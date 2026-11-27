[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/session

Official zero-heap tamper-proof cookie session plugin for cap:web (pure-CP keyed MAC + fixed-slot KV store)

| Metadata | Value |
| --- | --- |
| Package | `@web/session` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/session
```

```cp
import {
    SessionConfig,
    SessionSlot,
    sessMonotonicNowSec,
    sessNowSec,
    sessFnv1a64Update,
    sessSplitMix64,
    sessWriteHexU64,
    sessComputeMacHex,
} from "@web/session";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SessionConfig` | `class` | Fields: `secret: string, cookie_name: string, ttl_secs: u64, secure: bool, http_only: bool, same_site: string` |
| `SessionSlot` | `class` | Fields: `used: bool, id_buf: [SESS_MAX_ID_LEN]u8, id_len: usize, val_buf: [SESS_MAX_VAL_LEN]u8, val_len: usize, created_at: u64, +1 more` · Methods: `idSlice, valSlice, matchesId` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sessMonotonicNowSec` | `(): u64` | Invokes `sessMonotonicNowSec` and returns `u64`. |
| `sessNowSec` | `(): u64` | Invokes `sessNowSec` and returns `u64`. |
| `sessFnv1a64Update` | `(seed: u64, data: &[]u8): u64` | Invokes `sessFnv1a64Update` and returns `u64`. |
| `sessSplitMix64` | `(x: u64): u64` | Invokes `sessSplitMix64` and returns `u64`. |
| `sessWriteHexU64` | `(val: u64, out_buf: []u8, offset: usize)` | Executes `sessWriteHexU64` in-place. |
| `sessComputeMacHex` | `(secret: string, id: string, out_hex: &mut [SESS_SIG_HEX_LEN]u8)` | Executes `sessComputeMacHex` in-place. |
| `sessConstTimeEql` | `(a: string, b: string): bool` | Invokes `sessConstTimeEql` and returns `bool`. |
| `sessWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `sessWriteRaw` and returns `bool`. |
| `sessWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `sessWriteU64` and returns `bool`. |
| `sessVerifySignedCookie` | `(signed_cookie: string): string?` | Invokes `sessVerifySignedCookie` and returns `string?`. |
| `sessFindValidSlotIndex` | `(id: string, now_sec: u64): usize?` | Invokes `sessFindValidSlotIndex` and returns `usize?`. |
| `setSessionTestClock` | `(now_sec: u64)` | Mutates or appends state via `setSessionTestClock`. |
| `resetSessionStore` | `()` | Executes `resetSessionStore` in-place. |
| `sessionCreate` | `(user_val: string, now_sec: u64, out_cookie_val: []u8): usize?` | Invokes `sessionCreate` and returns `usize?`. |
| `sessionGet` | `(signed_cookie: string, now_sec: u64): string?` | Invokes `sessionGet` and returns `string?`. |
| `sessionDestroy` | `(signed_cookie: string): bool` | Invokes `sessionDestroy` and returns `bool`. |
| `formatSessionSetCookie` | `(cfg: SessionConfig, signed_cookie: string, out_buf: []u8): usize?` | Serializes/formats data via `formatSessionSetCookie` returning `usize?`. |
| `sessionHook` | `(ctx: &mut Context): bool` | Invokes `sessionHook` and returns `bool`. |
| `createSessionPlugin` | `(cfg: SessionConfig): Plugin` | Constructs/initializes an instance via `createSessionPlugin`. |
| `registerSession` | `(app: &mut App, cfg: SessionConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { formatSessionSetCookie } from "@web/session";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatSessionSetCookie(/* cfg: SessionConfig */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
