[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/audit

Official enterprise compliance write-operation audit log plugin with 32-slot ring buffer and sensitive header masking for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/audit` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AuditConfig` | `class` | Fields: `status_path: string, actor_header: string, mask_sensitive: bool` |
| `AuditRecord` | `class` | Fields: `seq: u64, method: string, path: string, status: u16, actor: string, client_ip: string` |
| `AudRingSlot` | `class` | Fields: `seq: u64, status: u16, method_buf: [AUD_MAX_METHOD_LEN]u8, method_len: usize, path_buf: [AUD_MAX_PATH_LEN]u8, path_len: usize, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `audAsciiLower` | `(c: u8): u8` | Invokes `audAsciiLower` and returns `u8`. |
| `audAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `audAsciiEqlIgnoreCase` and returns `bool`. |
| `audTrimSpaces` | `(s: string): string` | Invokes `audTrimSpaces` and returns `string`. |
| `audCopyInto` | `(dst: []u8, src: string): usize` | Invokes `audCopyInto` and returns `usize`. |
| `audIsWriteMethod` | `(m: AppMethod): bool` | Invokes `audIsWriteMethod` and returns `bool`. |
| `audResolveActor` | `(ctx: &mut Context): string` | Invokes `audResolveActor` and returns `string`. |
| `audResolveClientIp` | `(ctx: &mut Context): string` | Invokes `audResolveClientIp` and returns `string`. |
| `audWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `audWriteRaw` and returns `bool`. |
| `audWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `audWriteByte` and returns `bool`. |
| `audWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `audWriteU64` and returns `bool`. |
| `audWriteJsonStr` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `audWriteJsonStr` and returns `bool`. |
| `auditMaskHeaderValue` | `(header_name: string, val: string): string` | Invokes `auditMaskHeaderValue` and returns `string`. |
| `auditCount` | `(): usize` | Invokes `auditCount` and returns `usize`. |
| `auditTotalSeq` | `(): u64` | Invokes `auditTotalSeq` and returns `u64`. |
| `auditReset` | `()` | Executes `auditReset` in-place. |
| `auditRecordAt` | `(chronological_idx: usize): AuditRecord?` | Invokes `auditRecordAt` and returns `AuditRecord?`. |
| `audAfterHandle` | `(ctx: &mut Context, resp: &mut Response)` | Executes `audAfterHandle` in-place. |
| `audHandleStatus` | `(ctx: &mut Context): Response` | Invokes `audHandleStatus` and returns `Response`. |
| `audCreatePlugin` | `(cfg: AuditConfig): Plugin` | Invokes `audCreatePlugin` and returns `Plugin`. |
| `registerAudit` | `(app: &mut App, cfg: AuditConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { audAsciiEqlIgnoreCase } from "@web/audit";

pub fn main() {
    let result = audAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
