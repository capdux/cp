[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/logger

Official production observability plugin for cap:web (X-Request-Id, W3C traceparent, JSON access logs, OpenMetrics /metrics)

| Metadata | Value |
| --- | --- |
| Package | `@web/logger` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/logger
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LoggerConfig` | `class` | Fields: `request_id_header: string, metrics_path: string, enable_trace: bool` |
| `MetricsSnapshot` | `class` | Fields: `requests_total: u64, responses_2xx_total: u64, responses_4xx_total: u64, responses_5xx_total: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `logWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `logWriteByte` and returns `bool`. |
| `logWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `logWriteRaw` and returns `bool`. |
| `logHexDigitLower` | `(n: u8): u8` | Invokes `logHexDigitLower` and returns `u8`. |
| `logWriteU64Decimal` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `logWriteU64Decimal` and returns `bool`. |
| `logWriteU64HexFixed16` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `logWriteU64HexFixed16` and returns `bool`. |
| `logWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `logWriteJsonEscaped` and returns `bool`. |
| `formatAccessLogJson` | `(method: string, path: string, status: u16, req_id: string, out_buf: []u8): usize?` | Serializes/formats data via `formatAccessLogJson` returning `usize?`. |
| `formatTraceparent` | `(seq: u64, out_buf: []u8): usize?` | Serializes/formats data via `formatTraceparent` returning `usize?`. |
| `formatOpenMetrics` | `(snap: MetricsSnapshot, out_buf: []u8): usize?` | Serializes/formats data via `formatOpenMetrics` returning `usize?`. |
| `resetState` | `()` | Executes `resetState` in-place. |
| `getMetrics` | `(): MetricsSnapshot` | Queries/retrieves data via `getMetrics` returning `MetricsSnapshot`. |
| `lastAccessLog` | `(): string` | Invokes `lastAccessLog` and returns `string`. |
| `onRequestHook` | `(ctx: &mut Context): bool` | Invokes `onRequestHook` and returns `bool`. |
| `onAfterHook` | `(ctx: &mut Context, resp: &mut Response)` | Executes `onAfterHook` in-place. |
| `metricsHandler` | `(ctx: &mut Context): Response` | Invokes `metricsHandler` and returns `Response`. |
| `createLoggerPlugin` | `(cfg: LoggerConfig): Plugin` | Constructs/initializes an instance via `createLoggerPlugin`. |
| `registerLogger` | `(app: &mut App, cfg: LoggerConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { formatAccessLogJson } from "@web/logger";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatAccessLogJson("example", "example", 0, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
