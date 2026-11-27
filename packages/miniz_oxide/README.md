[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# miniz_oxide

High-performance lossless DEFLATE / zlib compression, checksums (CRC-32 & Adler-32), and single-entry ZIP archive engine for CP, modeled on Rust's [`miniz_oxide`](https://github.com/Frommi/miniz_oxide) crate and backed by native `miniz` 3.0.2 (MIT, CP v3.0.2).

Offers both RAII-managed dynamic buffers (`MinizBuf`, automatically freed on scope exit via `__cp_deinit`) and zero-heap slice-to-slice compression/decompression (`compressToSlice` / `decompressToSlice`), plus size-capped decompression (`decompressToVecWithLimit`) to guard against excessive expansion.

## Installation & Import

```bash
cap add miniz_oxide
```

```cp
import {
    MinizError,
    MinizBuf,
    compressToVec,
    compressToVecZlib,
    decompressToVec,
    decompressToVecZlib,
    decompressToVecWithLimit,
    compressToSlice,
    decompressToSlice,
    crc32,
    adler32,
} from "miniz_oxide";
```

## Core API Reference

### Errors & RAII Buffer

| Symbol | Signature / Variants | Description |
| :--- | :--- | :--- |
| `MinizError` | `ZipCreateFailed`, `ZipExtractFailed`, `OutOfMemory`, `CompressFailed`, `DecompressFailed`, `BufferTooSmall`, `InvalidLevel`, `OutputLimitExceeded` | Error set returned by all fallible (`!T`) functions. |
| `MinizBuf` | `raw: [*]u8!?`, `len: usize` | Owning C-heap buffer with automatic RAII release (`__cp_deinit`). |
| `MinizBuf.slice()` | `(): &[]u8` | Returns a read-only zero-copy slice view over the buffer bytes. |
| `MinizBuf.zipSingleFile` | `(entry_name: string, payload: &[]u8): !MinizBuf` | Creates an in-memory ZIP archive containing a single file entry. |
| `MinizBuf.extractFile` | `(zip_bytes: &[]u8, entry_name: string): !MinizBuf` | Extracts a named entry from an in-memory ZIP archive. |

### Compression & Decompression Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `compressToVec` / `compress_to_vec` | `(input: &[]u8, level: u8): !MinizBuf` | Raw DEFLATE compression (`level` in `0..=10`) into an RAII `MinizBuf`. |
| `compressToVecZlib` / `compress_to_vec_zlib` | `(input: &[]u8, level: u8): !MinizBuf` | Zlib-wrapped DEFLATE compression (`level` in `0..=10`). |
| `decompressToVec` / `decompress_to_vec` | `(input: &[]u8): !MinizBuf` | Raw DEFLATE decompression into an RAII `MinizBuf`. |
| `decompressToVecZlib` / `decompress_to_vec_zlib` | `(input: &[]u8): !MinizBuf` | Zlib-wrapped DEFLATE decompression. |
| `decompressToVecWithLimit` | `(input: &[]u8, max_size: usize): !MinizBuf` | Raw DEFLATE decompression capped at `max_size` bytes (`OutputLimitExceeded` if exceeded). |
| `decompressToVecZlibWithLimit` | `(input: &[]u8, max_size: usize): !MinizBuf` | Zlib decompression capped at `max_size` bytes. |
| `compressToSlice` / `compressToSliceZlib` | `(input: &[]u8, out: []u8, level: u8): !usize` | Zero-heap compression directly into caller-provided `out`, returning bytes written. |
| `decompressToSlice` / `decompressToSliceZlib` | `(input: &[]u8, out: []u8): !usize` | Zero-heap decompression directly into caller-provided `out`, returning bytes written. |

### Checksums & Utilities

| Function | Signature | Description |
| :--- | :--- | :--- |
| `versionString` | `(): string` | Returns underlying `miniz` library version string. |
| `compressBound` | `(source_len: usize): usize` | Computes worst-case compressed output size for `source_len` bytes. |
| `crc32` / `crc32Update` | `(&[]u8): u32` / `(u32, &[]u8): u32` | One-shot and incremental IEEE CRC-32 checksum. |
| `adler32` / `adler32Update` | `(&[]u8): u32` / `(u32, &[]u8): u32` | One-shot and incremental Adler-32 checksum. |
| `makeDeflateFlags` | `(level: u8, write_zlib_header: bool): !i32` | Validates `level` (`0..=10`) and builds `tdefl` compressor flags. |

Low-level C FFI symbols (`cp_miniz_*`, `tdefl_*`, `tinfl_*`, `mz_*`) are also exported for advanced custom streaming pipelines.

## Usage Examples

### 1. Zlib Compress & Size-Capped Decompress (RAII `MinizBuf`)

```cp
import { compressToVecZlib, decompressToVecZlibWithLimit } from "miniz_oxide";

fn zlibRoundTrip(data: &[]u8): !usize {
    let compressed = compressToVecZlib(data, 6)?;
    let restored = decompressToVecZlibWithLimit(compressed.slice(), 1024 * 1024)?;
    // Both compressed and restored are freed automatically at scope exit
    return restored.slice().len;
}
```

### 2. Zero-Heap Slice-to-Slice Compression

```cp
import { compressToSlice, decompressToSlice } from "miniz_oxide";

fn stackDeflate(input: &[]u8): !usize {
    let comp_buf: [512]u8 = undefined;
    let clen = compressToSlice(input, comp_buf[0..512], 6)?;

    let out_buf: [512]u8 = undefined;
    let dlen = decompressToSlice(&comp_buf[0..clen], out_buf[0..512])?;
    return dlen;
}
```

### 3. In-Memory Single-File ZIP Archive

```cp
import { MinizBuf, crc32 } from "miniz_oxide";

fn zipExample(payload: &[]u8): !u32 {
    let archive = MinizBuf.zipSingleFile("hello.txt", payload)?;
    let extracted = MinizBuf.extractFile(archive.slice(), "hello.txt")?;
    return crc32(extracted.slice());
}
```

## Differences from Upstream & Safety Highlights

- **Deterministic RAII Cleanup**: `MinizBuf` implements `__cp_deinit` to release native C memory automatically on scope exit or error propagation (`?`), preventing memory leaks without a GC.
- **Resource Exhaustion Guard**: `decompressToVecWithLimit` and `decompressToVecZlibWithLimit` enforce hard upper bounds on decompressed output size (`MinizError.OutputLimitExceeded`).
- **Dual Naming Ergonomics**: Provides idiomatic CP camelCase (`compressToVec`) alongside Rust `miniz_oxide` snake_case aliases (`compress_to_vec`).
