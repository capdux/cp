# crc32c — AI Agent Quick Ref

Zero-heap pure-CP CRC-32C (Castagnoli) checksum with streaming append, GF(2) combine, and LevelDB/RocksDB/GCS masking (crates.io crc32c equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Crc32cHasher,
    c32cByteStep,
    c32cGf2MatrixTimes,
    c32cGf2MatrixSquare,
    crc32cAppend,
    crc32c,
    crc32cStr,
    crc32cCombine,
} from "crc32c";
```

## Core Types & Constants

```cp
class Crc32cHasher { state: u32, amount: u64 } // methods: new(...) +6
```

## API Signatures

```cp
fn c32cByteStep(crc_in: u32, b: u8): u32;
fn c32cGf2MatrixTimes(mat: [32]u32, vec: u32): u32;
fn c32cGf2MatrixSquare(src: [32]u32): [32]u32;
fn crc32cAppend(crc: u32, data: []u8): u32;
fn crc32c(data: []u8): u32;
fn crc32cStr(s: string): u32;
fn crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32;
fn maskCrc32c(crc: u32): u32;
fn unmaskCrc32c(masked: u32): u32;
```

## Minimal Usage

```cp
import { crc32cStr } from "crc32c";

pub fn main() {
    let result = crc32cStr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crc32c"`; install via `cap add crc32c`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
