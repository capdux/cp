# CP 标准库：防篡改加密哈希链审计日志手册 (`cap:audit`)

## 一、 架构总览与设计准则

`cap:audit` 是 CP 语言官方内置的纯 CP **防篡改加密哈希链审计日志（Tamper-Evident Hash-Chained Audit Log）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于 `cap:crypto` 的 SHA-256 为企业级后端、RBAC 鉴权与 AI Agent 工具调用轨迹提供可验证不可抵赖的审计链路。既可通过 `import { ... } from "cap:audit"` 导入，也支持 `import cap from "cap"; cap.audit.*` 动态懒挂载调用。

```text
┌──────────────┐     ┌───────────────────────┐     ┌───────────────────────┐
│ Entry #1     │     │ Entry #2              │     │ Entry #N              │
│ prev = 00..0 │ ──▶ │ prev = hash(#1)       │ ──▶ │ prev = hash(#N-1)     │
│ hash = H(#1) │     │ hash = H(#2)          │     │ hash = H(#N)          │
└──────────────┘     └───────────────────────┘     └───────────────────────┘
```

### 1. 哈希链规范编码 (`CP-AUDIT-V1`)

每条审计记录 `AuditEntry` 的 `entry_hash` 均按无歧义规范编码计算 SHA-256（64 位小写十六进制）：
- 域分隔前缀：`"CP-AUDIT-V1\x00"`
- 8 字节大端序 `seq`、`timestamp_ms`
- 8 字节大端序长度前缀 + 原始字节：`prev_hash`、`actor`、`action`、`resource`、`outcome.toString()`、`severity.toString()`、`metadata_json`
- 创世首条（`seq == 1`）固定绑定 `GENESIS_HASH`（64 个 `'0'`）。

### 2. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/audit/trail.cp` | `// Source: cp/lib/audit/trail.cp (Tamper-Evident Hash-Chained Audit Log)` | `GENESIS_HASH`、`AuditError`、`AuditOutcome`、`AuditSeverity`、`AuditEntry`、`ChainVerifyResult`、`AuditTrail` |
| `lib/audit/index.cp` | `// Source: cp/lib/audit/index.cp (Unified Exports for cap:audit)` | `cap:audit` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 常量、错误与枚举类型

- **`GENESIS_HASH: string`**：64 个 `'0'` 组成的创世前序哈希常量。
- **`AuditError`**：
  - `TamperedEntry`：记录载荷或 `entry_hash` 与重算摘要不匹配。
  - `BrokenChain`：`seq` 断档/重排或 `prev_hash` 与前序记录 `entry_hash` 断裂。
  - `EmptyLog`：`importJsonl` 遇到空日志或纯空白输入。
  - `InvalidJsonl`：JSONL 格式非法、缺少必填字段或枚举值无效。
  - `OutOfMemory`：内存分配失败。
- **`AuditOutcome`**：`Success`、`Denied`、`Error`（提供 `toString()` 与 `AuditOutcome.fromString(s)`）。
- **`AuditSeverity`**：`Info`、`Warn`、`Critical`（提供 `toString()` 与 `AuditSeverity.fromString(s)`）。

### 2. 审计记录与校验结果结构体

- **`AuditEntry`**：`seq: u64`、`timestamp_ms: u64`、`actor: string`、`action: string`、`resource: string`、`outcome: AuditOutcome`、`severity: AuditSeverity`、`metadata_json: string`、`prev_hash: string`、`entry_hash: string`。
- **`ChainVerifyResult`**：`valid: bool`、`checked_count: usize`、`first_corrupt_seq: u64`、`reason: string`。

### 3. `AuditTrail` 核心方法

- **`AuditTrail.init(alloc)` / `deinit()`**：初始化与确定性释放所有内部托管内存。
- **`append(timestamp_ms, actor, action, resource, outcome, severity, metadata_json): AuditEntry ! AuditError`**：自动递增 `seq`、绑定前条 `entry_hash`、计算规范 SHA-256 并追加记录。
- **`verifyChain(): ChainVerifyResult`**：逐条校验 `seq` 连续性、`prev_hash` 链式衔接与 `entry_hash` 摘要完整性；任意字段篡改、哈希伪造、重排或删行均精确定位 `first_corrupt_seq`。
- **`queryByActor(alloc, actor): []AuditEntry ! AuditError`**：按主体过滤（调用方使用 `alloc.free(res)` 释放结果切片，条目字符串借用自 `AuditTrail`）。
- **`queryByAction(alloc, action): []AuditEntry ! AuditError`**：按动作过滤。
- **`queryRange(alloc, start_ms, end_ms): []AuditEntry ! AuditError`**：按闭区间 `[start_ms, end_ms]` 时间窗过滤。
- **`exportJsonl(): string ! AuditError`**：导出为转义完备的规范 JSONL 文本（调用方使用 `alloc.free(text)` 释放）。
- **`AuditTrail.importJsonl(alloc, jsonl_text): AuditTrail ! AuditError`**：从 JSONL 文本反序列化重建 `AuditTrail`，配合 `verifyChain()` 立即完成完整性校验。
- **`exportCsv(): string ! AuditError`**：导出为符合 RFC 4180 规范的 CSV 文本（含表头）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { AuditTrail, AuditOutcome, AuditSeverity, GENESIS_HASH } from "cap:audit";
import cap from "cap";

pub fn demoAudit(): ! {
    const alloc = std.heap.page_allocator;
    let trail = cap.audit.AuditTrail.init(alloc);
    defer trail.deinit();

    const e1 = trail.append(1700000000000, "agent:planner", "tool.read", "fs:/etc/hosts", AuditOutcome.Success, AuditSeverity.Info, "{\"bytes\":128}")?;
    const e2 = trail.append(1700000001000, "user:alice", "rbac.grant", "role:admin", AuditOutcome.Denied, AuditSeverity.Critical, "{\"reason\":\"mfa\"}")?;

    const check = trail.verifyChain();
    if (!check.valid) {
        return;
    }

    const jsonl = trail.exportJsonl()?;
    defer alloc.free(jsonl);

    let restored = AuditTrail.importJsonl(alloc, jsonl)?;
    defer restored.deinit();
    const check2 = restored.verifyChain();
    * = e1;
    * = e2;
    * = check2;
}
```
