# @web/audit@1.0.0 (Agent Quickref)

> Official enterprise compliance write-operation audit log plugin with 32-slot ring buffer and sensitive header masking for cap:web | License: MIT
> Import: `import { ... } from "@web/audit";` | Full types: `src/index.d.cp`

## Functions
- `export fn audAsciiLower(c: u8): u8;`
- `export fn audAsciiEqlIgnoreCase(a: string, b: string): bool;`
- `export fn audTrimSpaces(s: string): string;`
- `export fn audCopyInto(dst: []u8, src: string): usize;`
- `export fn audIsWriteMethod(m: AppMethod): bool;`
- `export fn audResolveActor(ctx: &mut Context): string;`
- `export fn audResolveClientIp(ctx: &mut Context): string;`
- `export fn audWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn audWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn audWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn audWriteJsonStr(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn auditMaskHeaderValue(header_name: string, val: string): string;`
- `export fn auditCount(): usize;`
- `export fn auditTotalSeq(): u64;`
- `export fn auditReset();`
- `export fn auditRecordAt(chronological_idx: usize): AuditRecord?;`
- `export fn audAfterHandle(ctx: &mut Context, resp: &mut Response);`
- `export fn audHandleStatus(ctx: &mut Context): Response;`
- `export fn audCreatePlugin(cfg: AuditConfig): Plugin;`
- `export fn registerAudit(app: &mut App, cfg: AuditConfig): &mut App;`
