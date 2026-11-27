# blake2 — AI Agent Quick Ref

Zero-allocation RFC 7693 BLAKE2s-256 streaming cryptographic hash & keyed MAC (Rust blake2 crate parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Blake2s256,
    bl2sAdd2,
    bl2sAdd3,
    bl2sRotr16,
    bl2sRotr12,
    bl2sRotr8,
    bl2sRotr7,
    bl2sReadLe32,
} from "blake2";
```

## Core Types & Constants

```cp
class Blake2s256 { h: [8]u32, t: u64, buf: [64]u8, buf_len: usize } // methods: new(...) +5
```

## API Signatures

```cp
fn bl2sAdd2(a: u32, b: u32): u32;
fn bl2sAdd3(a: u32, b: u32, c: u32): u32;
fn bl2sRotr16(x: u32): u32;
fn bl2sRotr12(x: u32): u32;
fn bl2sRotr8(x: u32): u32;
fn bl2sRotr7(x: u32): u32;
fn bl2sReadLe32(b: []u8, off: usize): u32;
fn bl2sWriteLe32(out: []u8, off: usize, v: u32);
fn bl2sHexDigit(nibble: u8): u8;
fn bl2sIv(idx: usize): u32;
fn bl2sMix(v: []u32, ai: usize, bi: usize, ci: usize, di: usize, x: u32, y: u32);
fn bl2sCompress(h: []u32, block: []u8, t: u64, is_last: bool);
fn blake2s256Bytes(data: []u8, out32: []u8);
fn blake2s256Hex(s: string, out64: []u8): usize;
```

## Minimal Usage

```cp
import { blake2s256Hex } from "blake2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = blake2s256Hex("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"blake2"`; install via `cap add blake2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
