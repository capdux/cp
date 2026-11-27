[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/webhook

Official Stripe / GitHub / Standard Webhooks HMAC-SHA256 verification and replay protection plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/webhook` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/webhook
```

```cp
import {
    WebhookConfig,
    WhEventSlot,
    WhSha256Hasher,
    whRotr32,
    whSha256Compress,
    whSha256,
    whHmacSha256,
    whAsciiLower,
} from "@web/webhook";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WebhookConfig` | `class` | 字段: `secret: string, endpoint_path: string, sig_header: string, ts_header: string, id_header: string, max_skew_secs: u64` |
| `WhEventSlot` | `class` | 字段: `id_buf: [WH_MAX_ID_LEN]u8, id_len: usize, used: bool` |
| `WhSha256Hasher` | `class` | 字段: `state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` · 方法: `init, update, finalize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `whRotr32` | `(x: u32, n: u5): u32` | 调用 `whRotr32` 并返回 `u32`。 |
| `whSha256Compress` | `(state: &mut [8]u32, block: &[64]u8)` | 就地执行 `whSha256Compress` 操作。 |
| `whSha256` | `(data: &[]u8, out: &mut [32]u8)` | 就地执行 `whSha256` 操作。 |
| `whHmacSha256` | `(key: string, msg: &[]u8, out: &mut [32]u8)` | 就地执行 `whHmacSha256` 操作。 |
| `whAsciiLower` | `(c: u8): u8` | 调用 `whAsciiLower` 并返回 `u8`。 |
| `whStrEql` | `(a: string, b: string): bool` | 调用 `whStrEql` 并返回 `bool`。 |
| `whAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `whAsciiEqlIgnoreCase` 并返回 `bool`。 |
| `whConstTimeHexEql` | `(expected_lower: &[]u8, candidate: string): bool` | 调用 `whConstTimeHexEql` 并返回 `bool`。 |
| `whParseU64` | `(s: string): u64?` | 调用 `whParseU64` 并返回 `u64?`。 |
| `whNowSec` | `(): u64` | 调用 `whNowSec` 并返回 `u64`。 |
| `whHasSeenEvent` | `(evt_id: string): bool` | 调用 `whHasSeenEvent` 并返回 `bool`。 |
| `whRecordEvent` | `(evt_id: string)` | 就地执行 `whRecordEvent` 操作。 |
| `webhookSignHex` | `(secret: string, payload: string, out_hex: []u8): usize?` | 调用 `webhookSignHex` 并返回 `usize?`。 |
| `webhookVerifySignature` | `(secret: string, payload: string, header_sig: string): bool` | 调用 `webhookVerifySignature` 并返回 `bool`。 |
| `setWebhookTestClock` | `(now_sec: u64)` | 写入或更新状态数据。 |
| `resetWebhookStore` | `()` | 就地执行 `resetWebhookStore` 操作。 |
| `whSeenCount` | `(): usize` | 调用 `whSeenCount` 并返回 `usize`。 |
| `whBeforeHandle` | `(ctx: &mut Context): bool` | 调用 `whBeforeHandle` 并返回 `bool`。 |
| `whDefaultHandler` | `(ctx: &mut Context): Response` | 调用 `whDefaultHandler` 并返回 `Response`。 |
| `whCreatePlugin` | `(cfg: WebhookConfig): Plugin` | 调用 `whCreatePlugin` 并返回 `Plugin`。 |
| `registerWebhook` | `(app: &mut App, cfg: WebhookConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { whStrEql } from "@web/webhook";

pub fn main() {
    let result = whStrEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
