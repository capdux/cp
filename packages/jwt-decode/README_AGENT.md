# jwt-decode — AI Agent Quick Ref

Zero-allocation RFC 7519 JWT segment splitter, Base64URL decoder, exp/nbf leeway validator, and alg:none guard in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    JwtParts,
    invalidParts,
    b64UrlVal,
    splitToken,
    decodeBase64Url,
    decodeHeader,
    decodePayload,
    isExpired,
} from "jwt-decode";
```

## Core Types & Constants

```cp
class JwtParts { header_b64: string, payload_b64: string, signature_b64: string, signing_input: string, /* +1 fields */ }
```

## API Signatures

```cp
fn invalidParts(): JwtParts;
fn b64UrlVal(b: u8): i32;
fn splitToken(token: string): JwtParts;
fn decodeBase64Url(input: string, out: []u8): i64;
fn decodeHeader(token: string, out: []u8): i64;
fn decodePayload(token: string, out: []u8): i64;
fn isExpired(exp_unix: i64, now_unix: i64, leeway_secs: i64): bool;
fn isNotYetValid(nbf_unix: i64, now_unix: i64, leeway_secs: i64): bool;
fn isJsonSpace(b: u8): bool;
fn isAlgNone(header_json: string): bool;
```

## Minimal Usage

```cp
import { decodeBase64Url } from "jwt-decode";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeBase64Url("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jwt-decode"`; install via `cap add jwt-decode`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
