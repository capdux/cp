[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/sse

Official Server-Sent Events (SSE) plugin with Last-Event-ID replay buffer & AI streaming helpers for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/sse` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/sse
```

```cp
import {
    SseConfig,
    SseMessage,
    sseWriteRaw,
    sseWriteU64,
    sseParseU64,
    encodeSseEvent,
    encodeSseHeartbeat,
    sseResetBroker,
} from "@web/sse";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SseConfig` | `class` | Fields: `endpoint_path: string, retry_ms: u32, cors_allow_origin: string` |
| `SseMessage` | `class` | Fields: `id: u64, event_name: string, data: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sseWriteRaw` | `(dst: []u8, pos: &mut usize, s: string): bool` | Invokes `sseWriteRaw` and returns `bool`. |
| `sseWriteU64` | `(dst: []u8, pos: &mut usize, val: u64): bool` | Invokes `sseWriteU64` and returns `bool`. |
| `sseParseU64` | `(s: string): u64` | Invokes `sseParseU64` and returns `u64`. |
| `encodeSseEvent` | `(id: u64, event_name: string, data: string, out_buf: []u8, pos: &mut usize): bool` | Serializes/formats data via `encodeSseEvent` returning `bool`. |
| `encodeSseHeartbeat` | `(out_buf: []u8, pos: &mut usize): bool` | Serializes/formats data via `encodeSseHeartbeat` returning `bool`. |
| `sseResetBroker` | `()` | Executes `sseResetBroker` in-place. |
| `ssePublish` | `(event_name: string, data: string): u64` | Invokes `ssePublish` and returns `u64`. |
| `buildSseReplayResponse` | `(last_event_id: u64): Response` | Invokes `buildSseReplayResponse` and returns `Response`. |
| `handleSseEndpoint` | `(ctx: &mut Context): Response` | Invokes `handleSseEndpoint` and returns `Response`. |
| `createSsePlugin` | `(cfg: SseConfig): Plugin` | Constructs/initializes an instance via `createSsePlugin`. |
| `registerSse` | `(app: &mut App, cfg: SseConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { encodeSseEvent } from "@web/sse";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeSseEvent(0, "example", "example", out_buf[0..256], /* pos: &mut usize */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
