[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# zstd

High-performance **Zstandard (Zstd)** native compression and decompression for CP, backed by Facebook's official C `libzstd` with RAII-managed `Compressor` / `Decompressor` classes and both zero-allocation (`*Into`) and allocator-backed (`*Alloc`) APIs.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [crates.io](https://crates.io/crates/zstd) |
| **Upstream Package** | `zstd` (v0.13 parity, backed by official `libzstd`) |
| **Upstream Repo** | https://github.com/gyscos/zstd-rs |
| **CP Package** | `zstd` (`1.0.0`) |
| **License** | BSD-3-Clause / MIT |

Keep the exact same package name in CP — no renaming required:

```rust
// Rust (crates.io)
use zstd::bulk::{Compressor, Decompressor};
```

```typescript
// CP (cap)
import { Compressor, Decompressor, compressBound, versionNumber, ZstdError } from "zstd";
```

## Installation

```bash
cap add zstd
```

## Key Features & Performance Highlights

- **Native `libzstd` Speed**: Ships prebuilt native C `zstd` bindings compiled with `-O3` (`opt: "fast"`).
- **RAII Context Lifecycle**: `Compressor` and `Decompressor` implement `__cp_deinit`, automatically freeing `ZSTD_CCtx` / `ZSTD_DCtx` when going out of scope — zero context leaks.
- **Zero-Allocation Hot Path**: Reuse a single `Compressor` / `Decompressor` across thousands of frames with `compressInto` and `decompressInto` writing straight into caller buffers.
- **Dynamic Buffer Convenience**: Use `compressAlloc` / `decompressAlloc` when frame sizes are unknown at compile time.

## Quick Start

### Zero-Allocation Compress & Decompress (`compressInto` / `decompressInto`)

```typescript
import { Compressor, Decompressor, compressBound } from "zstd";

fn roundtripZeroAlloc(input: &[]u8): !void {
    let comp: Compressor = undefined;
    try comp.init(3); // compression level 3 (default balanced)

    let compressed: [4096]u8 = undefined;
    const comp_len = try comp.compressInto(input, compressed[0..]);

    let decomp: Decompressor = undefined;
    try decomp.init();

    let restored: [4096]u8 = undefined;
    const dec_len = try decomp.decompressInto(compressed[0..comp_len], restored[0..]);
    _ = dec_len;
}
```

### Allocator-Backed Compress & Decompress (`compressAlloc` / `decompressAlloc`)

```typescript
import { Compressor, Decompressor } from "zstd";
import * as std from "std";

fn roundtripAlloc(allocator: std.mem.Allocator, input: &[]u8): ![]u8 {
    let comp: Compressor = undefined;
    try comp.init(6);
    const compressed = try comp.compressAlloc(allocator, input);
    defer allocator.free(compressed);

    let decomp: Decompressor = undefined;
    try decomp.init();
    return try decomp.decompressAlloc(allocator, compressed);
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `versionNumber` | `() -> u32` | Runtime `libzstd` version number (e.g. `10506`) |
| `compressBound` | `(src_size: usize) -> usize` | Worst-case compressed output size in bytes |
| `Compressor.init` | `(level: i32) -> !void` | Allocate `ZSTD_CCtx` and set compression level (1..22) |
| `Compressor.compressInto` | `(src: &[]u8, dst: []u8) -> !usize` | Compress into caller buffer, returning written bytes |
| `Compressor.compressAlloc` | `(allocator: std.mem.Allocator, src: &[]u8) -> ![]u8` | Compress into a newly allocated buffer |
| `Decompressor.init` | `() -> !void` | Allocate `ZSTD_DCtx` |
| `Decompressor.decompressInto` | `(src: &[]u8, dst: []u8) -> !usize` | Decompress into caller buffer, returning written bytes |
| `Decompressor.decompressAlloc` | `(allocator: std.mem.Allocator, src: &[]u8) -> ![]u8` | Decompress using frame content size into allocated buffer |
| `ZstdError` | `AllocContextFailed \| InvalidContext \| CompressFailed \| DecompressFailed \| UnknownContentSize \| OutOfMemory` | Typed error union |
| `ZSTD_*` | Raw C FFI bindings | Direct access to `ZSTD_createCCtx`, `ZSTD_compressCCtx`, `ZSTD_getFrameContentSize`, etc. |

## License

BSD-3-Clause / MIT
