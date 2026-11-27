[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/oauth2

Official OAuth 2.0 + RFC 7636 PKCE authorization plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/oauth2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/oauth2
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OAuth2Config` | `class` | Fields: `client_id: string, authorize_url: string, token_url: string, redirect_uri: string, scope: string, login_path: string, +1 more` |
| `OaStateSlot` | `class` | Fields: `buf: [OA_MAX_STATE_LEN]u8, len: usize, active: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `oaBytesEql` | `(a: string, b: string): bool` | Invokes `oaBytesEql` and returns `bool`. |
| `oaWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `oaWriteByte` and returns `bool`. |
| `oaWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `oaWriteRaw` and returns `bool`. |
| `oaHexUpper` | `(n: u8): u8` | Invokes `oaHexUpper` and returns `u8`. |
| `oaHexLower` | `(n: u8): u8` | Invokes `oaHexLower` and returns `u8`. |
| `oaIsUnreserved` | `(c: u8): bool` | Invokes `oaIsUnreserved` and returns `bool`. |
| `oaWriteUrlEncoded` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `oaWriteUrlEncoded` and returns `bool`. |
| `oaWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `oaWriteJsonEscaped` and returns `bool`. |
| `oaWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `oaWriteU64` and returns `bool`. |
| `oaHasQueryMark` | `(s: string): bool` | Invokes `oaHasQueryMark` and returns `bool`. |
| `oauth2BuildAuthorizeUrl` | `(cfg: OAuth2Config, state: string, code_challenge: string, out_buf: []u8): usize?` | Invokes `oauth2BuildAuthorizeUrl` and returns `usize?`. |
| `oauth2RegisterState` | `(state: string)` | Executes `oauth2RegisterState` in-place. |
| `oauth2ConsumeState` | `(state: string): bool` | Invokes `oauth2ConsumeState` and returns `bool`. |
| `oauth2ResetStates` | `()` | Executes `oauth2ResetStates` in-place. |
| `oauth2BuildTokenFormBody` | `(cfg: OAuth2Config, code: string, code_verifier: string, out_buf: []u8): usize?` | Invokes `oauth2BuildTokenFormBody` and returns `usize?`. |
| `oaLoginHandler` | `(ctx: &mut Context): Response` | Invokes `oaLoginHandler` and returns `Response`. |
| `oaCallbackHandler` | `(ctx: &mut Context): Response` | Invokes `oaCallbackHandler` and returns `Response`. |
| `registerOAuth2` | `(app: &mut App, cfg: OAuth2Config): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { oaBytesEql } from "@web/oauth2";

pub fn main() {
    let result = oaBytesEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
