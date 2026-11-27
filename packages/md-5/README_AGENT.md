# md-5 — AI Agent Quick Ref

Zero-allocation RFC 1321 MD5 cryptographic hash streaming implementation (Rust md-5/md5 crate parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Md5Hasher,
    md5Add32,
    md5Add4,
    md5Rotl32,
    md5ShiftAt,
    md5ConstAt,
    md5ReadLe32,
    md5WriteLe32,
} from "md-5";
```

## Core Types & Constants

```cp
class Md5Hasher { a0: u32, b0: u32, c0: u32, d0: u32, /* +3 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn md5Add32(a: u32, b: u32): u32;
fn md5Add4(a: u32, b: u32, c: u32, d: u32): u32;
fn md5Rotl32(x: u32, bits: u32): u32;
fn md5ShiftAt(idx: usize): u32;
fn md5ConstAt(idx: usize): u32;
fn md5ReadLe32(b: []u8, off: usize): u32;
fn md5WriteLe32(out: []u8, off: usize, v: u32);
fn md5HexDigit(nibble: u8): u8;
fn md5Bytes(data: []u8, out16: []u8);
fn md5Hex(s: string, out32: []u8): usize;
```

## Minimal Usage

```cp
import { md5Hex } from "md-5";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = md5Hex("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"md-5"`; install via `cap add md-5`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
