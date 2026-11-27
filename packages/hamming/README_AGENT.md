# hamming — AI Agent Quick Ref

Zero-allocation Hamming distance, byte popcount weight, 64-bit Charikar SimHash, and Hamming(7,4) error-correcting code in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Hamming74Decoded,
    popcountU32,
    popcountU8,
    popcountU64,
    hammingU64,
    weightBytes,
    distanceBytes,
    isTokenByte,
} from "hamming";
```

## Core Types & Constants

```cp
class Hamming74Decoded { data: u8, corrected: bool, error_bit: u8 }
```

## API Signatures

```cp
fn popcountU32(x: u32): u32;
fn popcountU8(b: u8): u32;
fn popcountU64(x: u64): u32;
fn hammingU64(a: u64, b: u64): u32;
fn weightBytes(data: &[]u8): u64;
fn distanceBytes(a: &[]u8, b: &[]u8): i64;
fn isTokenByte(b: u8): bool;
fn hashToken64(text: &[]u8, start: usize, end: usize): u64;
fn simhash64(text: &[]u8): u64;
fn hamming74Encode(nibble: u8): u8;
fn hamming74Decode(codeword: u8): Hamming74Decoded;
```

## Minimal Usage

```cp
import { isTokenByte } from "hamming";

pub fn main() {
    let result = isTokenByte(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hamming"`; install via `cap add hamming`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
