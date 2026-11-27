# @web/basic-auth — AI Agent Quick Ref

Official HTTP Basic Auth (RFC 7617) and X-API-Key constant-time authentication plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BasicAuthConfig,
    baConstTimeEql,
    baDecodeVal,
    basicAuthDecodeBase64,
    baStartsWithIgnoreCase,
    basicAuthHook,
    createBasicAuthPlugin,
    registerBasicAuth,
} from "@web/basic-auth";
```

## Core Types & Constants

```cp
class BasicAuthConfig { username: string, password: string, api_key: string, api_key_header: string, /* +2 fields */ }
```

## API Signatures

```cp
fn baConstTimeEql(a: string, b: string): bool;
fn baDecodeVal(c: u8): u8?;
fn basicAuthDecodeBase64(input: string, out_buf: []u8): usize?;
fn baStartsWithIgnoreCase(s: string, prefix: string): bool;
fn basicAuthHook(ctx: &mut Context): bool;
fn createBasicAuthPlugin(cfg: BasicAuthConfig): Plugin;
fn registerBasicAuth(app: &mut App, cfg: BasicAuthConfig): &mut App;
```

## Minimal Usage

```cp
import { baConstTimeEql } from "@web/basic-auth";

pub fn main() {
    let result = baConstTimeEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/basic-auth"`; install via `cap add @web/basic-auth`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
