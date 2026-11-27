# @web/webhook — AI Agent Quick Ref

Official Stripe / GitHub / Standard Webhooks HMAC-SHA256 verification and replay protection plugin for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class WebhookConfig { secret: string, endpoint_path: string, sig_header: string, ts_header: string, /* +2 fields */ }
class WhEventSlot { id_buf: [WH_MAX_ID_LEN]u8, id_len: usize, used: bool }
class WhSha256Hasher { state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64 } // methods: init(...) +2
```

## API Signatures

```cp
fn whRotr32(x: u32, n: u5): u32;
fn whSha256Compress(state: &mut [8]u32, block: &[64]u8);
fn whSha256(data: &[]u8, out: &mut [32]u8);
fn whHmacSha256(key: string, msg: &[]u8, out: &mut [32]u8);
fn whAsciiLower(c: u8): u8;
fn whStrEql(a: string, b: string): bool;
fn whAsciiEqlIgnoreCase(a: string, b: string): bool;
fn whConstTimeHexEql(expected_lower: &[]u8, candidate: string): bool;
fn whParseU64(s: string): u64?;
fn whNowSec(): u64;
fn whHasSeenEvent(evt_id: string): bool;
fn whRecordEvent(evt_id: string);
fn webhookSignHex(secret: string, payload: string, out_hex: []u8): usize?;
fn webhookVerifySignature(secret: string, payload: string, header_sig: string): bool;
fn setWebhookTestClock(now_sec: u64);
fn resetWebhookStore();
fn whSeenCount(): usize;
fn whBeforeHandle(ctx: &mut Context): bool;
fn whDefaultHandler(ctx: &mut Context): Response;
fn whCreatePlugin(cfg: WebhookConfig): Plugin;
fn registerWebhook(app: &mut App, cfg: WebhookConfig): &mut App;
```

## Minimal Usage

```cp
import { whStrEql } from "@web/webhook";

pub fn main() {
    let result = whStrEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/webhook"`; install via `cap add @web/webhook`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
