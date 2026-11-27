# @web/audit — AI Agent Quick Ref

Official enterprise compliance write-operation audit log plugin with 32-slot ring buffer and sensitive header masking for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class AuditConfig { status_path: string, actor_header: string, mask_sensitive: bool }
class AuditRecord { seq: u64, method: string, path: string, status: u16, /* +2 fields */ }
class AudRingSlot { seq: u64, status: u16, method_buf: [AUD_MAX_METHOD_LEN]u8, method_len: usize, /* +7 fields */ }
```

## API Signatures

```cp
fn audAsciiLower(c: u8): u8;
fn audAsciiEqlIgnoreCase(a: string, b: string): bool;
fn audTrimSpaces(s: string): string;
fn audCopyInto(dst: []u8, src: string): usize;
fn audIsWriteMethod(m: AppMethod): bool;
fn audResolveActor(ctx: &mut Context): string;
fn audResolveClientIp(ctx: &mut Context): string;
fn audWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn audWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn audWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn audWriteJsonStr(out_buf: []u8, pos: &mut usize, s: string): bool;
fn auditMaskHeaderValue(header_name: string, val: string): string;
fn auditCount(): usize;
fn auditTotalSeq(): u64;
fn auditReset();
fn auditRecordAt(chronological_idx: usize): AuditRecord?;
fn audAfterHandle(ctx: &mut Context, resp: &mut Response);
fn audHandleStatus(ctx: &mut Context): Response;
fn audCreatePlugin(cfg: AuditConfig): Plugin;
fn registerAudit(app: &mut App, cfg: AuditConfig): &mut App;
```

## Minimal Usage

```cp
import { audAsciiEqlIgnoreCase } from "@web/audit";

pub fn main() {
    let result = audAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/audit"`; install via `cap add @web/audit`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
