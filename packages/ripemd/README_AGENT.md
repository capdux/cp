# ripemd — AI Agent Quick Ref

Zero-heap pure-CP RIPEMD-160 160-bit cryptographic hash function (crates.io ripemd equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Ripemd160,
    rmdAdd,
    rmdAdd3,
    rmdAdd4,
    rmdRol,
    rmdNot,
    rmdFunc,
    rmdKl,
} from "ripemd";
```

## Core Types & Constants

```cp
class Ripemd160 { state: [5]u32, buf: [64]u8, buf_len: usize, total_len: u64 } // methods: new(...) +5
```

## API Signatures

```cp
fn rmdAdd(a: u32, b: u32): u32;
fn rmdAdd3(a: u32, b: u32, c: u32): u32;
fn rmdAdd4(a: u32, b: u32, c: u32, d: u32): u32;
fn rmdRol(x: u32, n: u32): u32;
fn rmdNot(x: u32): u32;
fn rmdFunc(round: usize, x: u32, y: u32, z: u32): u32;
fn rmdKl(round: usize): u32;
fn rmdKr(round: usize): u32;
fn rmdCompress(state: [5]u32, block: []u8): [5]u32;
fn ripemd160Bytes(data: []u8, out20: []u8);
fn ripemd160Hex(s: string, out40: []u8): usize;
```

## Minimal Usage

```cp
import { ripemd160Hex } from "ripemd";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ripemd160Hex("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ripemd"`; install via `cap add ripemd`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
