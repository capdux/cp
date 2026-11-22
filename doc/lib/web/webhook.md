# CP 标准库：Webhook 安全签名与幂等去重引擎 (`cap:webhook` / `cap.webhook`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

## 一、 架构总览与设计准则

`cap:webhook` 是 CP 语言官方内置的纯原生 Webhook 安全验签与幂等去重标准库，零外部依赖（0 `std.posix`、0 `@import("bun")`），底层直接使用 Zig 标准库 `std.crypto.auth.hmac.sha2.HmacSha256` 与 `std.crypto.timing_safe.eql` 实现恒定时间密码学比对，覆盖以下核心能力：

- **Standard Webhooks 规范**：`v1,<base64(HMAC-SHA256(secret, "{msg_id}.{ts}.{body}"))>` 签名生成、空格分隔多签名密钥轮换、恒定时间验证与重放窗口校验；
- **GitHub Webhook**：`X-Hub-Signature-256`（`sha256=<hex>`）签名生成与恒定时间验签；
- **Stripe Webhook**：`Stripe-Signature`（`t=<ts>,v1=<hex>`）签名生成、多 `v1=` 密钥轮换解析及时间戳防重放校验；
- **TTL 幂等去重存储**：`IdempotencyStore` 提供确定性时间窗口内的事件 ID 防重入、过期淘汰（`purgeExpired`）与显式所有权生命周期管理；
- **双通道接入**：支持 `import ... from "cap:web"` 与 `import cap from "cap"; cap.webhook.*` 自动动态挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/webhook/sign.cp` | `// Source: cp/lib/webhook/sign.cp (Standard Webhooks, GitHub & Stripe HMAC-SHA256 Verification)` | `WebhookError`、`StandardWebhook`、`GitHubWebhook`、`StripeWebhook` |
| `lib/webhook/idempotency.cp` | `// Source: cp/lib/webhook/idempotency.cp (TTL-Bounded Webhook Idempotency Deduplication Store)` | `IdempotencyStore` |
| `lib/webhook/index.cp` | `// Source: cp/lib/webhook/index.cp (Unified Exports for cap:webhook)` | `cap:webhook` 与 `cap.webhook` 对外统一导出入口 |

---

## 二、 核心 API 详解

### 1. 错误类型与签名验证引擎（`sign.cp`）

- **`WebhookError`**：
  - `InvalidSignature`：签名不匹配
  - `TimestampExpired`：时间戳早于允许窗口（`now_sec - ts_sec > tolerance_sec`）
  - `TimestampFuture`：时间戳超前于允许窗口（`ts_sec - now_sec > tolerance_sec`）
  - `MalformedHeader`：签名头格式缺失、非法时间戳或缺少有效签名字段
  - `OutOfMemory`：内存分配失败

- **`StandardWebhook`**：
  - `sign(alloc, secret, msg_id, ts_sec, payload): ([]u8) ! WebhookError`：生成 `v1,<base64>` 签名头，调用方负责释放返回切片；
  - `verify(secret, msg_id, ts_sec, payload, sig_header, now_sec, tolerance_sec): void ! WebhookError`：校验空格/制表符分隔的签名列表（支持多密钥轮换），校验时间窗口并执行恒定时间 HMAC-SHA256 验签。

- **`GitHubWebhook`**：
  - `signSha256(alloc, secret, payload): ([]u8) ! WebhookError`：生成 `"sha256=<lowercase_hex>"`；
  - `verifySha256(secret, payload, header): bool`：校验 `"sha256=<64 hex>"` 头部并执行恒定时间 32 字节摘要比对。

- **`StripeWebhook`**：
  - `signHeader(alloc, secret, ts_sec, payload): ([]u8) ! WebhookError`：生成 `"t=<ts_sec>,v1=<lowercase_hex>"`；
  - `verifyHeader(secret, payload, header, now_sec, tolerance_sec): void ! WebhookError`：解析 `t=` 与多个 `v1=` 字段，忽略辅助版本键（如 `v0=`），执行防重放窗口拦截与恒定时间多签名校验。

### 2. 幂等去重存储（`idempotency.cp`）

- **`IdempotencyStore`**：
  - `init(alloc, default_ttl_ms): IdempotencyStore`
  - `deinit(*_)`：释放所有内部事件 ID 拷贝与哈希表内存
  - `checkAndRecord(*_, event_id, now_ms): bool`：首次或已过期时写入并刷新 TTL 返回 `true`；TTL 窗口内重复到达返回 `false`
  - `checkAndRecordWithTtl(*_, event_id, ttl_ms, now_ms): bool`：按事件指定自定义 TTL
  - `isSeen(event_id, now_ms): bool`：只读查询事件是否处于有效去重窗口内
  - `purgeExpired(*_, now_ms): usize`：无堆分配批量清理所有已过期条目并返回清理数量
  - `remove(*_, event_id): bool`：主动移除指定事件
  - `len(): usize`：返回当前存储条目总数

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    WebhookError,
    StandardWebhook,
    GitHubWebhook,
    StripeWebhook,
    IdempotencyStore,
} from "cap:web";
const std = @import("std");

pub fn handleIncomingWebhook(payload: []const u8, now_sec: i64, now_ms: u64): !void {
    const alloc = std.heap.page_allocator;

    // 1. Standard Webhooks 签名与验签
    const std_sig = StandardWebhook.sign(alloc, "whsec_key", "msg_01", now_sec, payload)?;
    defer alloc.free(std_sig);
    StandardWebhook.verify("whsec_key", "msg_01", now_sec, payload, std_sig, now_sec, 300)?;

    // 2. GitHub SHA-256 验签
    const gh_sig = GitHubWebhook.signSha256(alloc, "gh_secret", payload)?;
    defer alloc.free(gh_sig);
    const gh_ok = GitHubWebhook.verifySha256("gh_secret", payload, gh_sig);

    // 3. Stripe 签名头验签
    const stripe_sig = StripeWebhook.signHeader(alloc, "whsec_stripe", now_sec, payload)?;
    defer alloc.free(stripe_sig);
    StripeWebhook.verifyHeader("whsec_stripe", payload, stripe_sig, now_sec, 300)?;

    // 4. 幂等去重存储（TTL 5 分钟 = 300_000 ms）
    let dedup = cap.webhook.IdempotencyStore.init(alloc, 300_000);
    defer dedup.deinit();
    if (dedup.checkAndRecord("msg_01", now_ms)) {
        // 首次投递：安全执行业务处理
    }
    * = gh_ok;
}
```
