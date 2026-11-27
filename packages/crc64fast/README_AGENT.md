# crc64fast — AI Agent Quick Ref

Zero-allocation 64-bit CRC checksum engine supporting ECMA-182, CRC-64/XZ, and CRC-64/GO-ISO with 256-entry lookup tables and streaming hasher (Rust crc / crc64fast parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Crc64Hasher,
    makeU64,
    buildSlice4Table,
    updateReflectedSlice4,
    updateNormalSlice4,
    crc64Ecma,
    crc64Xz,
    crc64Iso,
} from "crc64fast";
```

## Core Types & Constants

```cp
class Crc64Hasher { table: [256]u64, crc: u64, init_val: u64, xorout: u64, /* +2 fields */ } // methods: initCustom(...) +6
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn buildSlice4Table(poly: u64, reflected: bool): [4][256]u64;
fn updateReflectedSlice4(crc_in: u64, data: &[]u8, tbl: [4][256]u64): u64;
fn updateNormalSlice4(crc_in: u64, data: &[]u8, tbl: [4][256]u64): u64;
fn crc64Ecma(data: &[]u8): u64;
fn crc64Xz(data: &[]u8): u64;
fn crc64Iso(data: &[]u8): u64;
```

## Minimal Usage

```cp
import { makeU64 } from "crc64fast";

pub fn main() {
    let result = makeU64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crc64fast"`; install via `cap add crc64fast`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
