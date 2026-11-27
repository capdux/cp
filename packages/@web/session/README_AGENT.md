# @web/session — AI Agent Quick Ref

Official zero-heap tamper-proof cookie session plugin for cap:web (pure-CP keyed MAC + fixed-slot KV store) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class SessionConfig { secret: string, cookie_name: string, ttl_secs: u64, secure: bool, /* +2 fields */ }
class SessionSlot { used: bool, id_buf: [SESS_MAX_ID_LEN]u8, id_len: usize, val_buf: [SESS_MAX_VAL_LEN]u8, /* +3 fields */ } // methods: idSlice(...) +2
```

## API Signatures

```cp
fn sessMonotonicNowSec(): u64;
fn sessNowSec(): u64;
fn sessFnv1a64Update(seed: u64, data: &[]u8): u64;
fn sessSplitMix64(x: u64): u64;
fn sessWriteHexU64(val: u64, out_buf: []u8, offset: usize);
fn sessComputeMacHex(secret: string, id: string, out_hex: &mut [SESS_SIG_HEX_LEN]u8);
fn sessConstTimeEql(a: string, b: string): bool;
fn sessWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn sessWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn sessVerifySignedCookie(signed_cookie: string): string?;
fn sessFindValidSlotIndex(id: string, now_sec: u64): usize?;
fn setSessionTestClock(now_sec: u64);
fn resetSessionStore();
fn sessionCreate(user_val: string, now_sec: u64, out_cookie_val: []u8): usize?;
fn sessionGet(signed_cookie: string, now_sec: u64): string?;
fn sessionDestroy(signed_cookie: string): bool;
fn formatSessionSetCookie(cfg: SessionConfig, signed_cookie: string, out_buf: []u8): usize?;
fn sessionHook(ctx: &mut Context): bool;
fn createSessionPlugin(cfg: SessionConfig): Plugin;
fn registerSession(app: &mut App, cfg: SessionConfig): &mut App;
```

## Minimal Usage

```cp
import { formatSessionSetCookie } from "@web/session";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatSessionSetCookie(/* cfg: SessionConfig */, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/session"`; install via `cap add @web/session`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
