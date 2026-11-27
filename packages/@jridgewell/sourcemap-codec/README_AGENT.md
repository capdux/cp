# @jridgewell/sourcemap-codec — AI Agent Quick Ref

Zero-heap Source Map v3 Base64 VLQ (Variable-Length Quantity) signed integer sequence encoder and decoder. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DecodeResult,
    b64Char,
    b64Val,
    encodeOne,
    decodeOne,
    encodeSlice,
    decodeSlice,
} from "@jridgewell/sourcemap-codec";
```

## Core Types & Constants

```cp
class DecodeResult { value: i64, consumed: usize, valid: bool }
```

## API Signatures

```cp
fn b64Char(v: u8): u8;
fn b64Val(c: u8): i32;
fn encodeOne(val: i64, out: []u8): usize;
fn decodeOne(input: string, start: usize): DecodeResult;
fn encodeSlice(values: &[]i64, out: []u8): usize;
fn decodeSlice(input: string, out: []i64): i64;
```

## Minimal Usage

```cp
import { decodeOne } from "@jridgewell/sourcemap-codec";

pub fn main() {
    let result = decodeOne("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@jridgewell/sourcemap-codec"`; install via `cap add @jridgewell/sourcemap-codec`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
