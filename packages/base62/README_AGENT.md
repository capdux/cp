# base62 — AI Agent Quick Ref

Zero-heap URL-safe Base62 (0-9A-Za-z) u64 and arbitrary byte-slice encoder/decoder in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DecodeU64Result,
    charToVal,
    isValidBase62,
    encodeU64,
    encodeU64Padded,
    decodeU64,
    encodeBytes,
    decodeBytes,
} from "base62";
```

## Core Types & Constants

```cp
class DecodeU64Result { value: u64, ok: bool }
```

## API Signatures

```cp
fn charToVal(c: u8): i32;
fn isValidBase62(src: &[]u8): bool;
fn encodeU64(value: u64, dst: []u8): usize;
fn encodeU64Padded(value: u64, width: usize, dst: []u8): usize;
fn decodeU64(src: &[]u8): DecodeU64Result;
fn encodeBytes(src: &[]u8, dst: []u8): i64;
fn decodeBytes(src: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isValidBase62 } from "base62";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidBase62(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"base62"`; install via `cap add base62`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
