# @web/sse — AI Agent Quick Ref

Official Server-Sent Events (SSE) plugin with Last-Event-ID replay buffer & AI streaming helpers for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class SseConfig { endpoint_path: string, retry_ms: u32, cors_allow_origin: string }
class SseMessage { id: u64, event_name: string, data: string }
```

## API Signatures

```cp
fn sseWriteRaw(dst: []u8, pos: &mut usize, s: string): bool;
fn sseWriteU64(dst: []u8, pos: &mut usize, val: u64): bool;
fn sseParseU64(s: string): u64;
fn encodeSseEvent(id: u64, event_name: string, data: string, out_buf: []u8, pos: &mut usize): bool;
fn encodeSseHeartbeat(out_buf: []u8, pos: &mut usize): bool;
fn sseResetBroker();
fn ssePublish(event_name: string, data: string): u64;
fn buildSseReplayResponse(last_event_id: u64): Response;
fn handleSseEndpoint(ctx: &mut Context): Response;
fn createSsePlugin(cfg: SseConfig): Plugin;
fn registerSse(app: &mut App, cfg: SseConfig): &mut App;
```

## Minimal Usage

```cp
import { encodeSseEvent } from "@web/sse";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeSseEvent(0, "example", "example", out_buf[0..256], /* pos: &mut usize */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/sse"`; install via `cap add @web/sse`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
