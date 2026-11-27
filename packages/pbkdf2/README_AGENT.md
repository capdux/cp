# pbkdf2 — AI Agent Quick Ref

Pure CP zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256 password-based key derivation and constant-time verification (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Pbkdf2Pads,
    pbkdf2InitPads,
    pbkdf2HmacSaltBlock,
    pbkdf2Hmac32,
    pbkdf2HmacSha256,
    pbkdf2Verify,
    pbkdf2HmacSha256Str,
} from "pbkdf2";
```

## Core Types & Constants

```cp
class Pbkdf2Pads { inner_base: Sha256State, outer_base: Sha256State }
```

## API Signatures

```cp
fn pbkdf2InitPads(password: &[]u8): Pbkdf2Pads;
fn pbkdf2HmacSaltBlock(pads: Pbkdf2Pads, salt: &[]u8, block_idx: u32, out32: []u8);
fn pbkdf2Hmac32(pads: Pbkdf2Pads, msg32: []u8, out32: []u8);
fn pbkdf2HmacSha256(password: &[]u8, salt: &[]u8, iterations: u32, out_dk: []u8): bool;
fn pbkdf2Verify(password: &[]u8, salt: &[]u8, iterations: u32, expected_dk: &[]u8): bool;
fn pbkdf2HmacSha256Str(password: string, salt: string, iterations: u32, out_dk: []u8): bool;
```

## Minimal Usage

```cp
import { pbkdf2HmacSha256Str } from "pbkdf2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pbkdf2HmacSha256Str("example", "example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pbkdf2"`; install via `cap add pbkdf2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
