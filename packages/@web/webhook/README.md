[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/webhook

Official Stripe / GitHub / Standard Webhooks HMAC-SHA256 verification and replay protection plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/webhook` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/webhook
```

```cp
import {
    WebhookConfig,
    WhEventSlot,
    WhSha256Hasher,
    whRotr32,
    whSha256Compress,
    whSha256,
    whHmacSha256,
    whAsciiLower,
} from "@web/webhook";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WebhookConfig` | `class` | Fields: `secret: string, endpoint_path: string, sig_header: string, ts_header: string, id_header: string, max_skew_secs: u64` |
| `WhEventSlot` | `class` | Fields: `id_buf: [WH_MAX_ID_LEN]u8, id_len: usize, used: bool` |
| `WhSha256Hasher` | `class` | Fields: `state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` · Methods: `init, update, finalize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `whRotr32` | `(x: u32, n: u5): u32` | Invokes `whRotr32` and returns `u32`. |
| `whSha256Compress` | `(state: &mut [8]u32, block: &[64]u8)` | Executes `whSha256Compress` in-place. |
| `whSha256` | `(data: &[]u8, out: &mut [32]u8)` | Executes `whSha256` in-place. |
| `whHmacSha256` | `(key: string, msg: &[]u8, out: &mut [32]u8)` | Executes `whHmacSha256` in-place. |
| `whAsciiLower` | `(c: u8): u8` | Invokes `whAsciiLower` and returns `u8`. |
| `whStrEql` | `(a: string, b: string): bool` | Invokes `whStrEql` and returns `bool`. |
| `whAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `whAsciiEqlIgnoreCase` and returns `bool`. |
| `whConstTimeHexEql` | `(expected_lower: &[]u8, candidate: string): bool` | Invokes `whConstTimeHexEql` and returns `bool`. |
| `whParseU64` | `(s: string): u64?` | Invokes `whParseU64` and returns `u64?`. |
| `whNowSec` | `(): u64` | Invokes `whNowSec` and returns `u64`. |
| `whHasSeenEvent` | `(evt_id: string): bool` | Invokes `whHasSeenEvent` and returns `bool`. |
| `whRecordEvent` | `(evt_id: string)` | Executes `whRecordEvent` in-place. |
| `webhookSignHex` | `(secret: string, payload: string, out_hex: []u8): usize?` | Invokes `webhookSignHex` and returns `usize?`. |
| `webhookVerifySignature` | `(secret: string, payload: string, header_sig: string): bool` | Invokes `webhookVerifySignature` and returns `bool`. |
| `setWebhookTestClock` | `(now_sec: u64)` | Mutates or appends state via `setWebhookTestClock`. |
| `resetWebhookStore` | `()` | Executes `resetWebhookStore` in-place. |
| `whSeenCount` | `(): usize` | Invokes `whSeenCount` and returns `usize`. |
| `whBeforeHandle` | `(ctx: &mut Context): bool` | Invokes `whBeforeHandle` and returns `bool`. |
| `whDefaultHandler` | `(ctx: &mut Context): Response` | Invokes `whDefaultHandler` and returns `Response`. |
| `whCreatePlugin` | `(cfg: WebhookConfig): Plugin` | Invokes `whCreatePlugin` and returns `Plugin`. |
| `registerWebhook` | `(app: &mut App, cfg: WebhookConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { whStrEql } from "@web/webhook";

pub fn main() {
    let result = whStrEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
