# simd-adler32 — AI Agent Quick Ref

Zero-allocation IEEE CRC-32, Castagnoli CRC-32C, Adler-32, and O(log N) CRC-32 combine engine (Rust crc32fast parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Crc32Hasher,
    Crc32cHasher,
    buildCrcTable,
    crc32Ieee,
    crc32c,
    adler32,
    gf2MatrixTimes,
    gf2MatrixSquare,
} from "simd-adler32";
```

## Core Types & Constants

```cp
class Crc32Hasher { state: u32, table: [256]u32 } // methods: init(...) +3
class Crc32cHasher { state: u32, table: [256]u32 } // methods: init(...) +3
```

## API Signatures

```cp
fn buildCrcTable(poly: u32): [256]u32;
fn crc32Ieee(data: string): u32;
fn crc32c(data: string): u32;
fn adler32(data: string): u32;
fn gf2MatrixTimes(mat: &mut [32]u32, vec_in: u32): u32;
fn gf2MatrixSquare(square: &mut [32]u32, mat: &mut [32]u32);
fn combineCrc32Ieee(crc1: u32, crc2: u32, len2: usize): u32;
```

## Minimal Usage

```cp
import { crc32Ieee } from "simd-adler32";

pub fn main() {
    let result = crc32Ieee("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"simd-adler32"`; install via `cap add simd-adler32`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
