[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/snappy

Google Snappy 1.2.1 fast compression/decompression library (Kafka/Parquet/LevelDB/RocksDB) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/snappy` |
| Version | `1.2.1` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/snappy
```

```cp
import {
    SnappyError,
    snappy_compress,
    snappy_uncompress,
    snappy_max_compressed_length,
    snappy_uncompressed_length,
    snappy_validate_compressed_buffer,
    maxCompressedLength,
    uncompressedLength,
} from "@cap/snappy";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `SnappyError` | `InvalidInput, BufferTooSmall` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `snappy_compress` | `(input: [*]&u8!, input_length: usize, compressed: [*]u8!, compressed_length: *usize!): i32` | Invokes `snappy_compress` and returns `i32`. |
| `snappy_uncompress` | `(compressed: [*]&u8!, compressed_length: usize, uncompressed: [*]u8!, uncompressed_length: *usize!): i32` | Invokes `snappy_uncompress` and returns `i32`. |
| `snappy_max_compressed_length` | `(source_length: usize): usize` | Invokes `snappy_max_compressed_length` and returns `usize`. |
| `snappy_uncompressed_length` | `(compressed: [*]&u8!, compressed_length: usize, result: *usize!): i32` | Invokes `snappy_uncompressed_length` and returns `i32`. |
| `snappy_validate_compressed_buffer` | `(compressed: [*]&u8!, compressed_length: usize): i32` | Invokes `snappy_validate_compressed_buffer` and returns `i32`. |
| `maxCompressedLength` | `(source_len: usize): usize` | Invokes `maxCompressedLength` and returns `usize`. |
| `uncompressedLength` | `(compressed: &[]u8): !usize` | Invokes `uncompressedLength` and returns `!usize`. |
| `compressInto` | `(src: &[]u8, dst: []u8): !&[]u8` | Invokes `compressInto` and returns `!&[]u8`. |
| `uncompressInto` | `(compressed: &[]u8, dst: []u8): !&[]u8` | Invokes `uncompressInto` and returns `!&[]u8`. |
| `validateCompressedBuffer` | `(compressed: &[]u8): bool` | Validates input with `validateCompressedBuffer` returning `bool`. |

## Minimal Example

```cp
import { validateCompressedBuffer } from "@cap/snappy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = validateCompressedBuffer(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
