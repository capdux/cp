# @web/logger@1.0.0 (Agent Quickref)

> Official production observability plugin for cap:web (X-Request-Id, W3C traceparent, JSON access logs, OpenMetrics /metrics) | License: MIT
> Import: `import { ... } from "@web/logger";` | Full types: `src/index.d.cp`

## Functions
- `export fn logWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn logWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn logHexDigitLower(n: u8): u8;`
- `export fn logWriteU64Decimal(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn logWriteU64HexFixed16(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn logWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn formatAccessLogJson(method: string, path: string, status: u16, req_id: string, out_buf: []u8): usize?;`
- `export fn formatTraceparent(seq: u64, out_buf: []u8): usize?;`
- `export fn formatOpenMetrics(snap: MetricsSnapshot, out_buf: []u8): usize?;`
- `export fn resetState();`
- `export fn getMetrics(): MetricsSnapshot;`
- `export fn lastAccessLog(): string;`
- `export fn onRequestHook(ctx: &mut Context): bool;`
- `export fn onAfterHook(ctx: &mut Context, resp: &mut Response);`
- `export fn metricsHandler(ctx: &mut Context): Response;`
- `export fn createLoggerPlugin(cfg: LoggerConfig): Plugin;`
- `export fn registerLogger(app: &mut App, cfg: LoggerConfig): &mut App;`
