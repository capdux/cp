# sha1 — AI Agent Quick Ref

Zero-allocation FIPS 180-4 SHA-1 cryptographic hash streaming implementation (Rust sha1 crate parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Sha1Hasher,
    sha1Add32,
    sha1Add5,
    sha1Rotl1,
    sha1Rotl5,
    sha1Rotl30,
    sha1ReadBe32,
    sha1WriteBe32,
} from "sha1";
```

## Core Types & Constants

```cp
class Sha1Hasher { h0: u32, h1: u32, h2: u32, h3: u32, /* +4 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn sha1Add32(a: u32, b: u32): u32;
fn sha1Add5(a: u32, b: u32, c: u32, d: u32, e: u32): u32;
fn sha1Rotl1(x: u32): u32;
fn sha1Rotl5(x: u32): u32;
fn sha1Rotl30(x: u32): u32;
fn sha1ReadBe32(b: []u8, off: usize): u32;
fn sha1WriteBe32(out: []u8, off: usize, v: u32);
fn sha1HexDigit(nibble: u8): u8;
fn sha1Bytes(data: []u8, out20: []u8);
fn sha1Hex(s: string, out40: []u8): usize;
```

## Minimal Usage

```cp
import { sha1Hex } from "sha1";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sha1Hex("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sha1"`; install via `cap add sha1`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
