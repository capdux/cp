[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/jwt

Official zero-heap HS256 JWT authentication plugin for cap:web (pure-CP SHA-256, HMAC-SHA256, Base64URL)

| Metadata | Value |
| --- | --- |
| Package | `@web/jwt` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/jwt
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Sha256Hasher` | `class` | Fields: `state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` · Methods: `init, update, finalize` |
| `JwtConfig` | `class` | Fields: `secret: string, issuer: string, header_name: string, optional_auth: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rotr32` | `(x: u32, n: u5): u32` | Invokes `rotr32` and returns `u32`. |
| `sha256Compress` | `(state: &mut [8]u32, block: &[64]u8)` | Executes `sha256Compress` in-place. |
| `sha256` | `(data: &[]u8, out: &mut [32]u8)` | Executes `sha256` in-place. |
| `hmacSha256` | `(key: string, msg: &[]u8, out: &mut [32]u8)` | Executes `hmacSha256` in-place. |
| `base64UrlEncode` | `(input: &[]u8, out_buf: []u8): usize?` | Invokes `base64UrlEncode` and returns `usize?`. |
| `base64UrlCharVal` | `(ch: u8): u8?` | Invokes `base64UrlCharVal` and returns `u8?`. |
| `base64UrlDecode` | `(input: string, out_buf: []u8): usize?` | Invokes `base64UrlDecode` and returns `usize?`. |
| `jwtConstantTimeEql` | `(a: &[]u8, b: &[]u8): bool` | Invokes `jwtConstantTimeEql` and returns `bool`. |
| `jwtAsciiLower` | `(c: u8): u8` | Invokes `jwtAsciiLower` and returns `u8`. |
| `jwtAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `jwtAsciiEqlIgnoreCase` and returns `bool`. |
| `jwtTrimSpaces` | `(s: string): string` | Invokes `jwtTrimSpaces` and returns `string`. |
| `jwtContainsSubstr` | `(haystack: string, needle: string): bool` | Invokes `jwtContainsSubstr` and returns `bool`. |
| `jwtWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `jwtWriteRaw` and returns `bool`. |
| `signHs256` | `(secret: string, payload_json: string, out_buf: []u8): usize?` | Invokes `signHs256` and returns `usize?`. |
| `verifyHs256` | `(secret: string, token: string, out_payload: []u8): usize?` | Validates input with `verifyHs256` returning `usize?`. |
| `jwtCheckIssuerClaim` | `(payload: string, issuer: string): bool` | Invokes `jwtCheckIssuerClaim` and returns `bool`. |
| `configure` | `(cfg: JwtConfig)` | Executes `configure` in-place. |
| `authHook` | `(ctx: &mut Context): bool` | Invokes `authHook` and returns `bool`. |
| `createJwtPlugin` | `(cfg: JwtConfig): Plugin` | Constructs/initializes an instance via `createJwtPlugin`. |
| `registerJwt` | `(app: &mut App, cfg: JwtConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { base64UrlDecode } from "@web/jwt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = base64UrlDecode("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
