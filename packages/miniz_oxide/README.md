[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# miniz_oxide

Native lossless DEFLATE / zlib compression & decompression, bounded-output decompression (decompression-limit protection), CRC32/Adler32 checksums, and single-file ZIP archive creation/extraction backed by `miniz 3.0.2` (`-O3`) with automatic RAII cleanup (`MinizBuf`).

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [crates.io/crates/miniz_oxide](https://crates.io/crates/miniz_oxide) |
| **Upstream Package** | `miniz_oxide` (CP v3.0.2, backed by miniz 3.0.2) |
| **Upstream Repository** | https://github.com/Frommi/miniz_oxide |
| **License** | MIT |

Keep the exact package name when migrating from Rust to CP (both camelCase and snake_case aliases are exported):

```rust
// Rust
use miniz_oxide::deflate::compress_to_vec_zlib;
use miniz_oxide::inflate::decompress_to_vec_zlib;
```

```cp
// CP — same package name, native C (-O3) speed + RAII MinizBuf
import {
    compressToVecZlib, decompressToVecZlib,
    compress_to_vec_zlib, decompress_to_vec_zlib,
} from "miniz_oxide";
```

## Installation

```bash
cap add miniz_oxide
```

## Quick Start & Practical Examples

### 1. DEFLATE & Zlib Compression / Decompression (`*ToVec` & `*WithLimit`)

```cp
import {
    compressToVec, decompressToVec,
    compressToVecZlib, decompressToVecZlib,
    decompressToVecWithLimit, decompressToVecZlibWithLimit,
    MinizBuf,
} from "miniz_oxide";

fn demoZlib(payload: &[]u8): !void {
    // Compress with level 6 (valid range: 0..10); MinizBuf auto-frees via __cp_deinit
    let compressed: MinizBuf = compressToVecZlib(payload, 6)?;

    // Decompress with an explicit max output size guard against resource exhaustion
    let restored: MinizBuf = decompressToVecZlibWithLimit(compressed.slice(), 1024 * 1024)?;
    let raw_bytes: &[]u8 = restored.slice();

    // Raw DEFLATE (without zlib header/trailer)
    let raw_comp = compressToVec(payload, 6)?;
    let raw_back = decompressToVecWithLimit(raw_comp.slice(), 1024 * 1024)?;
}
```

### 2. Zero-Heap Slice-to-Slice Compression (`compressToSlice*` / `decompressToSlice*`)

```cp
import {
    compressBound, compressToSliceZlib, decompressToSliceZlib,
    crc32, crc32Update, adler32, adler32Update, versionString,
} from "miniz_oxide";

fn demoZeroHeapSlice(input: &[]u8): !void {
    let bound = compressBound(input.len);
    mut comp_buf: [1024]u8 = [0; 1024];
    mut out_buf: [1024]u8 = [0; 1024];

    let comp_len = compressToSliceZlib(input, &mut comp_buf, 6)?;
    let orig_len = decompressToSliceZlib(&comp_buf[0..comp_len], &mut out_buf)?;

    // Incremental checksums
    let c1 = crc32(input);
    let c2 = crc32Update(c1, input);
    let a1 = adler32(input);
    let a2 = adler32Update(a1, input);
    let ver = versionString();
}
```

### 3. Single-File ZIP Archive Creation & Extraction (`MinizBuf.zipSingleFile` / `extractFile`)

```cp
import { MinizBuf } from "miniz_oxide";

fn demoZip(content: &[]u8): !void {
    let archive = MinizBuf.zipSingleFile("hello.txt", content)?;
    let extracted = MinizBuf.extractFile(archive.slice(), "hello.txt")?;
    let data: &[]u8 = extracted.slice();
}
```

## Performance & Safety Highlights

- **Native C (`miniz 3.0.2`, `-O3`)**: Prebuilt native C engine delivering high-throughput DEFLATE/zlib/ZIP performance.
- **Deterministic RAII Cleanup**: `MinizBuf.__cp_deinit` automatically releases C-allocated buffers via `cp_miniz_free` on scope exit or error propagation.
- **Zero-Heap Slice Mode**: `compressToSlice*` and `decompressToSlice*` operate entirely inside caller-supplied stack buffers (`MinizError.BufferTooSmall` if insufficient).
- **Resource Exhaustion Guard**: `decompressToVecWithLimit` and `decompressToVecZlibWithLimit` return `MinizError.OutputLimitExceeded` if decompressed output exceeds `max_size`.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
