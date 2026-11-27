[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/audit

Official enterprise compliance write-operation audit log plugin with 32-slot ring buffer and sensitive header masking for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/audit` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/audit
```

```cp
import {
    AuditConfig,
    AuditRecord,
    AudRingSlot,
    audAsciiLower,
    audAsciiEqlIgnoreCase,
    audTrimSpaces,
    audCopyInto,
    audIsWriteMethod,
} from "@web/audit";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AuditConfig` | `class` | 字段: `status_path: string, actor_header: string, mask_sensitive: bool` |
| `AuditRecord` | `class` | 字段: `seq: u64, method: string, path: string, status: u16, actor: string, client_ip: string` |
| `AudRingSlot` | `class` | 字段: `seq: u64, status: u16, method_buf: [AUD_MAX_METHOD_LEN]u8, method_len: usize, path_buf: [AUD_MAX_PATH_LEN]u8, path_len: usize, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `audAsciiLower` | `(c: u8): u8` | 调用 `audAsciiLower` 并返回 `u8`。 |
| `audAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `audAsciiEqlIgnoreCase` 并返回 `bool`。 |
| `audTrimSpaces` | `(s: string): string` | 调用 `audTrimSpaces` 并返回 `string`。 |
| `audCopyInto` | `(dst: []u8, src: string): usize` | 调用 `audCopyInto` 并返回 `usize`。 |
| `audIsWriteMethod` | `(m: AppMethod): bool` | 调用 `audIsWriteMethod` 并返回 `bool`。 |
| `audResolveActor` | `(ctx: &mut Context): string` | 调用 `audResolveActor` 并返回 `string`。 |
| `audResolveClientIp` | `(ctx: &mut Context): string` | 调用 `audResolveClientIp` 并返回 `string`。 |
| `audWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `audWriteRaw` 并返回 `bool`。 |
| `audWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `audWriteByte` 并返回 `bool`。 |
| `audWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `audWriteU64` 并返回 `bool`。 |
| `audWriteJsonStr` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `audWriteJsonStr` 并返回 `bool`。 |
| `auditMaskHeaderValue` | `(header_name: string, val: string): string` | 调用 `auditMaskHeaderValue` 并返回 `string`。 |
| `auditCount` | `(): usize` | 调用 `auditCount` 并返回 `usize`。 |
| `auditTotalSeq` | `(): u64` | 调用 `auditTotalSeq` 并返回 `u64`。 |
| `auditReset` | `()` | 就地执行 `auditReset` 操作。 |
| `auditRecordAt` | `(chronological_idx: usize): AuditRecord?` | 调用 `auditRecordAt` 并返回 `AuditRecord?`。 |
| `audAfterHandle` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `audAfterHandle` 操作。 |
| `audHandleStatus` | `(ctx: &mut Context): Response` | 调用 `audHandleStatus` 并返回 `Response`。 |
| `audCreatePlugin` | `(cfg: AuditConfig): Plugin` | 调用 `audCreatePlugin` 并返回 `Plugin`。 |
| `registerAudit` | `(app: &mut App, cfg: AuditConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { audAsciiEqlIgnoreCase } from "@web/audit";

pub fn main() {
    let result = audAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
