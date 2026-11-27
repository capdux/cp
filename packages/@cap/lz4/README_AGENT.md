# @cap/lz4 — AI Agent Quick Ref

Extremely fast lossless compression (LZ4 & LZ4-HC) native C binding for CP (`v1.10.0`, `BSD-2-Clause`)

## Import

```cp
import {
    Lz4Error,
    Lz4Buffer,
    LZ4_versionNumber,
    LZ4_compressBound,
    LZ4_compress_default,
    LZ4_compress_fast,
    LZ4_compress_HC,
    LZ4_decompress_safe,
} from "@cap/lz4";
```

## Core Types & Constants

```cp
error Lz4Error { InputTooLarge, OutputTooSmall, CompressFailed, DecompressFailed, OutOfMemory }
class Lz4Buffer { data: []u8, allocator: std.mem.Allocator } // methods: compress(...) +4
```

## API Signatures

```cp
fn LZ4_versionNumber(): i32;
fn LZ4_compressBound(inputSize: i32): i32;
fn LZ4_compress_default(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32): i32;
fn LZ4_compress_fast(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, acceleration: i32): i32;
fn LZ4_compress_HC(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, compressionLevel: i32): i32;
fn LZ4_decompress_safe(src: [*]&u8!, dst: [*]u8!, compressedSize: i32, dstCapacity: i32): i32;
fn versionNumber(): i32;
fn compressBound(input_len: usize): !usize;
fn compressInto(src: &[]u8, dst: []u8): !&[]u8;
fn compressFastInto(src: &[]u8, dst: []u8, acceleration: i32): !&[]u8;
fn compressHCInto(src: &[]u8, dst: []u8, level: i32): !&[]u8;
fn decompressInto(src: &[]u8, dst: []u8): !&[]u8;
```

## Minimal Usage

```cp
import { versionNumber } from "@cap/lz4";

pub fn main() {
    let result = versionNumber();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/lz4"`; install via `cap add @cap/lz4`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
