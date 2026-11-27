# @web/oauth2 — AI Agent Quick Ref

Official OAuth 2.0 + RFC 7636 PKCE authorization plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    OAuth2Config,
    OaStateSlot,
    oaBytesEql,
    oaWriteByte,
    oaWriteRaw,
    oaHexUpper,
    oaHexLower,
    oaIsUnreserved,
} from "@web/oauth2";
```

## Core Types & Constants

```cp
class OAuth2Config { client_id: string, authorize_url: string, token_url: string, redirect_uri: string, /* +3 fields */ }
class OaStateSlot { buf: [OA_MAX_STATE_LEN]u8, len: usize, active: bool }
```

## API Signatures

```cp
fn oaBytesEql(a: string, b: string): bool;
fn oaWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn oaWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn oaHexUpper(n: u8): u8;
fn oaHexLower(n: u8): u8;
fn oaIsUnreserved(c: u8): bool;
fn oaWriteUrlEncoded(out_buf: []u8, pos: &mut usize, s: string): bool;
fn oaWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
fn oaWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn oaHasQueryMark(s: string): bool;
fn oauth2BuildAuthorizeUrl(cfg: OAuth2Config, state: string, code_challenge: string, out_buf: []u8): usize?;
fn oauth2RegisterState(state: string);
fn oauth2ConsumeState(state: string): bool;
fn oauth2ResetStates();
fn oauth2BuildTokenFormBody(cfg: OAuth2Config, code: string, code_verifier: string, out_buf: []u8): usize?;
fn oaLoginHandler(ctx: &mut Context): Response;
fn oaCallbackHandler(ctx: &mut Context): Response;
fn registerOAuth2(app: &mut App, cfg: OAuth2Config): &mut App;
```

## Minimal Usage

```cp
import { oaBytesEql } from "@web/oauth2";

pub fn main() {
    let result = oaBytesEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/oauth2"`; install via `cap add @web/oauth2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
