# hmac@1.0.0 (Agent Quickref)

> Zero-heap RFC 2104 streaming HMAC-SHA256 MAC with constant-time verify and hex output (crates.io hmac equivalent, reuses @cap/cookie-signature) | License: MIT
> Import: `import { ... } from "hmac";` | Full types: `src/index.d.cp`
> Deps: `cookie-signature`

## Functions
- `export fn encodeHex32(digest: []u8, out64: []u8): usize;`
- `export fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize;`
