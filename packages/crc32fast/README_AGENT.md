# crc32fast — AI Agent Quick Ref

Zero-heap IEEE 802.3 CRC-32 streaming Hasher with GF(2) combine, reset, and one-shot hash (crates.io crc32fast equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Crc32Hasher,
    crcByteStep,
    crcBuildTables,
    crcGf2MatrixTimes,
    crcGf2MatrixSquare,
    combineCrc32,
    hashBytes,
    hashStr,
} from "crc32fast";
```

## Core Types & Constants

```cp
class Crc32Hasher { state: u32, amount: u64 } // methods: new(...) +6
```

## API Signatures

```cp
fn crcByteStep(crc_in: u32, b: u8): u32;
fn crcBuildTables(): [4][256]u32;
fn crcGf2MatrixTimes(mat: [32]u32, vec: u32): u32;
fn crcGf2MatrixSquare(src: [32]u32): [32]u32;
fn combineCrc32(crc1: u32, crc2: u32, len2: u64): u32;
fn hashBytes(buf: &[]u8): u32;
fn hashStr(s: string): u32;
```

## Minimal Usage

```cp
import { hashStr } from "crc32fast";

pub fn main() {
    let result = hashStr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crc32fast"`; install via `cap add crc32fast`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
