# @web/jwt — AI Agent Quick Ref

Official zero-heap HS256 JWT authentication plugin for cap:web (pure-CP SHA-256, HMAC-SHA256, Base64URL) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Sha256Hasher,
    JwtConfig,
    rotr32,
    sha256Compress,
    sha256,
    hmacSha256,
    base64UrlEncode,
    base64UrlCharVal,
} from "@web/jwt";
```

## Core Types & Constants

```cp
class Sha256Hasher { state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64 } // methods: init(...) +2
class JwtConfig { secret: string, issuer: string, header_name: string, optional_auth: bool }
```

## API Signatures

```cp
fn rotr32(x: u32, n: u5): u32;
fn sha256Compress(state: &mut [8]u32, block: &[64]u8);
fn sha256(data: &[]u8, out: &mut [32]u8);
fn hmacSha256(key: string, msg: &[]u8, out: &mut [32]u8);
fn base64UrlEncode(input: &[]u8, out_buf: []u8): usize?;
fn base64UrlCharVal(ch: u8): u8?;
fn base64UrlDecode(input: string, out_buf: []u8): usize?;
fn jwtConstantTimeEql(a: &[]u8, b: &[]u8): bool;
fn jwtAsciiLower(c: u8): u8;
fn jwtAsciiEqlIgnoreCase(a: string, b: string): bool;
fn jwtTrimSpaces(s: string): string;
fn jwtContainsSubstr(haystack: string, needle: string): bool;
fn jwtWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn signHs256(secret: string, payload_json: string, out_buf: []u8): usize?;
fn verifyHs256(secret: string, token: string, out_payload: []u8): usize?;
fn jwtCheckIssuerClaim(payload: string, issuer: string): bool;
fn configure(cfg: JwtConfig);
fn authHook(ctx: &mut Context): bool;
fn createJwtPlugin(cfg: JwtConfig): Plugin;
fn registerJwt(app: &mut App, cfg: JwtConfig): &mut App;
```

## Minimal Usage

```cp
import { base64UrlDecode } from "@web/jwt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = base64UrlDecode("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/jwt"`; install via `cap add @web/jwt`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
