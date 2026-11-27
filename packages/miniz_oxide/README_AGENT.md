# miniz_oxide — AI Agent Quick Ref

DEFLATE / zlib compression, CRC-32/Adler-32 checksums, and in-memory single-file ZIP engine backed by `miniz` 3.0.2 with RAII `MinizBuf` (Rust `miniz_oxide` parity).

## Import

```cp
import {
    MinizError, MinizBuf,
    compressToVec, compressToVecZlib,
    decompressToVec, decompressToVecZlib,
    decompressToVecWithLimit, decompressToVecZlibWithLimit,
    compressToSlice, compressToSliceZlib,
    decompressToSlice, decompressToSliceZlib,
    compressBound, crc32, crc32Update, adler32, adler32Update, versionString,
} from "miniz_oxide";
```

## Core API Signatures

```cp
error MinizError {
    ZipCreateFailed, ZipExtractFailed, OutOfMemory,
    CompressFailed, DecompressFailed, BufferTooSmall,
    InvalidLevel, OutputLimitExceeded,
}

class MinizBuf { raw: [*]u8!?, len: usize } // Auto-freed via __cp_deinit
MinizBuf.zipSingleFile(entry_name: string, payload: &[]u8): !MinizBuf
MinizBuf.extractFile(zip_bytes: &[]u8, entry_name: string): !MinizBuf
buf.slice(): &[]u8

// Heap-backed (RAII) — also aliased as compress_to_vec[_zlib], decompress_to_vec[_zlib]
compressToVec(input: &[]u8, level: u8): !MinizBuf
compressToVecZlib(input: &[]u8, level: u8): !MinizBuf
decompressToVec(input: &[]u8): !MinizBuf
decompressToVecZlib(input: &[]u8): !MinizBuf
decompressToVecWithLimit(input: &[]u8, max_size: usize): !MinizBuf
decompressToVecZlibWithLimit(input: &[]u8, max_size: usize): !MinizBuf

// Zero-heap slice APIs
compressToSlice(input: &[]u8, out: []u8, level: u8): !usize
compressToSliceZlib(input: &[]u8, out: []u8, level: u8): !usize
decompressToSlice(input: &[]u8, out: []u8): !usize
decompressToSliceZlib(input: &[]u8, out: []u8): !usize

// Checksums & utils
versionString(): string; compressBound(source_len: usize): usize
crc32(data: &[]u8): u32; crc32Update(prev: u32, data: &[]u8): u32
adler32(data: &[]u8): u32; adler32Update(prev: u32, data: &[]u8): u32
makeDeflateFlags(level: u8, write_zlib_header: bool): !i32
```

## Minimal Example

```cp
import { compressToVecZlib, decompressToVecZlibWithLimit } from "miniz_oxide";

fn demo(input: &[]u8): !usize {
    let comp = compressToVecZlib(input, 6)?;
    let raw = decompressToVecZlibWithLimit(comp.slice(), 65536)?;
    return raw.slice().len;
}
```

## Gotchas & Borrow Rules

- **Level range**: `level` must be `0..=10`; higher values return `MinizError.InvalidLevel`.
- **Format matching**: Do not mix raw DEFLATE (`compressToVec`/`decompressToVec`) with zlib (`*Zlib`).
- **RAII lifetime**: `MinizBuf` frees itself on scope exit; do not return `buf.slice()` after `buf` goes out of scope.
- **Untrusted input**: Always use `decompressToVecWithLimit` / `decompressToVecZlibWithLimit` (or fixed `decompressToSlice`) to prevent unbounded memory growth.
