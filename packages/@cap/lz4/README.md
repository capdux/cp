[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/lz4

Extremely fast lossless compression (LZ4 & LZ4-HC) native C binding for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/lz4` |
| Version | `1.10.0` |
| License | `BSD-2-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/lz4
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `Lz4Error` | `InputTooLarge, OutputTooSmall, CompressFailed, DecompressFailed, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Lz4Buffer` | `class` | Fields: `data: []u8, allocator: std.mem.Allocator` · Methods: `compress, compressHC, decompress, slice, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `LZ4_versionNumber` | `(): i32` | Invokes `LZ4_versionNumber` and returns `i32`. |
| `LZ4_compressBound` | `(inputSize: i32): i32` | Invokes `LZ4_compressBound` and returns `i32`. |
| `LZ4_compress_default` | `(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32): i32` | Invokes `LZ4_compress_default` and returns `i32`. |
| `LZ4_compress_fast` | `(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, acceleration: i32): i32` | Invokes `LZ4_compress_fast` and returns `i32`. |
| `LZ4_compress_HC` | `(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, compressionLevel: i32): i32` | Invokes `LZ4_compress_HC` and returns `i32`. |
| `LZ4_decompress_safe` | `(src: [*]&u8!, dst: [*]u8!, compressedSize: i32, dstCapacity: i32): i32` | Invokes `LZ4_decompress_safe` and returns `i32`. |
| `versionNumber` | `(): i32` | Invokes `versionNumber` and returns `i32`. |
| `compressBound` | `(input_len: usize): !usize` | Invokes `compressBound` and returns `!usize`. |
| `compressInto` | `(src: &[]u8, dst: []u8): !&[]u8` | Invokes `compressInto` and returns `!&[]u8`. |
| `compressFastInto` | `(src: &[]u8, dst: []u8, acceleration: i32): !&[]u8` | Invokes `compressFastInto` and returns `!&[]u8`. |
| `compressHCInto` | `(src: &[]u8, dst: []u8, level: i32): !&[]u8` | Invokes `compressHCInto` and returns `!&[]u8`. |
| `decompressInto` | `(src: &[]u8, dst: []u8): !&[]u8` | Invokes `decompressInto` and returns `!&[]u8`. |

## Minimal Example

```cp
import { versionNumber } from "@cap/lz4";

pub fn main() {
    let result = versionNumber();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
