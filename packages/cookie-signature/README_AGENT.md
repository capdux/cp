# cookie-signature@1.0.0 (Agent Quickref)

> Zero-heap RFC 6265 Cookie parser, Set-Cookie header formatter, and built-in HMAC-SHA256 constant-time cookie signing/verification. | License: MIT
> Import: `import { ... } from "cookie-signature";` | Full types: `src/index.d.cp`

## Functions
- `export fn add32(a: u32, b: u32): u32;`
- `export fn rotr32(x: u32, n: u32): u32;`
- `export fn sha256K(i: usize): u32;`
- `export fn hmacSha256(key: &[]u8, msg: &[]u8, out: []u8);`
- `export fn b64UrlChar(v: u8): u8;`
- `export fn encodeBase64Url32(mac: &[]u8, dst: []u8);`
- `export fn isWs(c: u8): bool;`
- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn getCookie(header: &[]u8, name: &[]u8): CookieSlice;`
- `export fn sign(val: &[]u8, secret: &[]u8, dst: []u8): i64;`
- `export fn unsign(signed_val: &[]u8, secret: &[]u8): CookieSlice;`
- `export fn appendBytes(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn appendU64(dst: []u8, pos: usize, val: u64): i64;`
- `export fn formatSetCookie(name: &[]u8, value: &[]u8, opts: CookieOptions, dst: []u8): i64;`
