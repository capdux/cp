# @web/logger — AI Agent Quick Ref

Official production observability plugin for cap:web (X-Request-Id, W3C traceparent, JSON access logs, OpenMetrics /metrics) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LoggerConfig,
    MetricsSnapshot,
    logWriteByte,
    logWriteRaw,
    logHexDigitLower,
    logWriteU64Decimal,
    logWriteU64HexFixed16,
    logWriteJsonEscaped,
} from "@web/logger";
```

## Core Types & Constants

```cp
class LoggerConfig { request_id_header: string, metrics_path: string, enable_trace: bool }
class MetricsSnapshot { requests_total: u64, responses_2xx_total: u64, responses_4xx_total: u64, responses_5xx_total: u64 }
```

## API Signatures

```cp
fn logWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn logWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn logHexDigitLower(n: u8): u8;
fn logWriteU64Decimal(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn logWriteU64HexFixed16(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn logWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
fn formatAccessLogJson(method: string, path: string, status: u16, req_id: string, out_buf: []u8): usize?;
fn formatTraceparent(seq: u64, out_buf: []u8): usize?;
fn formatOpenMetrics(snap: MetricsSnapshot, out_buf: []u8): usize?;
fn resetState();
fn getMetrics(): MetricsSnapshot;
fn lastAccessLog(): string;
fn onRequestHook(ctx: &mut Context): bool;
fn onAfterHook(ctx: &mut Context, resp: &mut Response);
fn metricsHandler(ctx: &mut Context): Response;
fn createLoggerPlugin(cfg: LoggerConfig): Plugin;
fn registerLogger(app: &mut App, cfg: LoggerConfig): &mut App;
```

## Minimal Usage

```cp
import { formatAccessLogJson } from "@web/logger";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatAccessLogJson("example", "example", 0, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/logger"`; install via `cap add @web/logger`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
