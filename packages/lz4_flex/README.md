[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `lz4_flex`

Zero-allocation LZ4 Raw Block lossless compressor and decompressor for CP — clean-room parity with **crates.io `lz4_flex`** and the official LZ4 Block Format, featuring a fixed 4096-entry stack hash table, 4-byte match acceleration, overlapping RLE match copy, and little-endian size-prefixed framing.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `crates.io:lz4_flex` ([PSeitz/lz4_flex](https://github.com/PSeitz/lz4_flex), v0.11 parity) |
| **CP Package** | `lz4_flex@1.0.0` (`MIT`) |
| **Dependencies** | None (stdlib `cap:bin`, `cap:fmt` only) |

Keep the exact original package name — no renaming required:

```rust
// Rust (crates.io lz4_flex)
use lz4_flex::block::{compress_prepend_size, decompress_size_prepended};
let compressed = compress_prepend_size(input);
let restored = decompress_size_prepended(&compressed)?;
```

```cp
// CP — identical package name, zero-heap caller buffers
import { compressWithSize, decompressWithSize, maxCompressedLen } from "lz4_flex";

let comp_buf: [4352]u8 = undefined;
const comp_len = compressWithSize(input, comp_buf[0..]);
let out_buf: [4096]u8 = undefined;
const orig_len = decompressWithSize(comp_buf[0..@intCast(comp_len)], out_buf[0..]);
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Bandwidth | Architecture |
| :--- | :--- | --: | --: | --: | :--- |
| `compressBlock` | 4 KB JSON-log text (27.7× ratio) | **1,862.81 ns/op** | 536,822 ops/s | **2,096.96 MB/s (2.10 GB/s)** | 4096-entry stack hash table + 4B match |
| `decompressBlock` | 4 KB (from 148 B compressed) | **1,251.47 ns/op** | 799,060 ops/s | **3,121.33 MB/s (3.12 GB/s)** | `@memcpy` literal/match fast copy |

- **Zero heap allocations**: Hash table lives on stack; both compression and decompression write directly into caller-owned slices.
- **Native `@memcpy` decompression**: Non-overlapping literals and matches copy via LLVM `@memcpy`; overlapping RLE matches (`offset < match_len`) fall back cleanly to byte-wise copy.

## Examples

### 1. Raw Block Compression & Decompression

```cp
import { compressBlock, decompressBlock, maxCompressedLen } from "lz4_flex";

fn demoRawBlock() {
    const src = "hello lz4_flex! hello lz4_flex! hello lz4_flex! hello lz4_flex!";
    const worst = maxCompressedLen(src.len);

    let comp_buf: [256]u8 = undefined;
    if (comp_buf.len < worst) { return; }

    const comp_n = compressBlock(src, comp_buf[0..]);
    if (comp_n < 0) { return; }

    let decomp_buf: [256]u8 = undefined;
    const orig_n = decompressBlock(comp_buf[0..@intCast(comp_n)], decomp_buf[0..]);
    if (orig_n < 0) { return; }
    // decomp_buf[0..@intCast(orig_n)] == src
}
```

### 2. Size-Prefixed Framing (`compressWithSize` / `decompressWithSize`)

```cp
import { compressWithSize, decompressWithSize } from "lz4_flex";

fn demoSizePrefixed() {
    const payload = "{\"event\":\"metric\",\"service\":\"gateway\",\"status\":200}";

    // Prepends 4-byte little-endian uncompressed length (compatible with lz4_flex::compress_prepend_size)
    let wire: [256]u8 = undefined;
    const wire_len = compressWithSize(payload, wire[0..]);
    if (wire_len < 0) { return; }

    let restored: [256]u8 = undefined;
    const out_len = decompressWithSize(wire[0..@intCast(wire_len)], restored[0..]);
    if (out_len < 0) { return; }
    // restored[0..@intCast(out_len)] == payload
}
```

### 3. Edge Cases: Buffer Sizing & Corrupted Stream Detection

```cp
import { compressBlock, decompressBlock, decompressWithSize, maxCompressedLen } from "lz4_flex";

fn demoEdgeCases() {
    // 1. Always size dst >= maxCompressedLen(src.len) for incompressible data
    //    For compressWithSize(), add +4 bytes for the LE u32 size header.
    const needed = maxCompressedLen(4096) + 4;
    _ = needed;

    // 2. Undersized destination returns -1 cleanly (no buffer overrun)
    let tiny: [4]u8 = undefined;
    const fail_comp = compressBlock("incompressible-sample-bytes-0123456789", tiny[0..]); // -1

    // 3. Truncated or malformed compressed input returns -1
    const bad_frame = [_]u8{ 0xFF, 0x00, 0x00, 0x00, 0xF0 };
    let dst: [64]u8 = undefined;
    const fail_dec = decompressWithSize(bad_frame[0..], dst[0..]); // -1
    const fail_raw = decompressBlock(bad_frame[0..], dst[0..]);    // -1
    _ = fail_comp; _ = fail_dec; _ = fail_raw;
}
```

## API Overview

- `maxCompressedLen(input_len: usize): usize` — worst-case raw block output size
- `compressBlock(src: &[]u8, dst: []u8): i64` — raw LZ4 block compress (`-1` on error)
- `decompressBlock(src: &[]u8, dst: []u8): i64` — raw LZ4 block decompress (`-1` on error)
- `compressWithSize(src: &[]u8, dst: []u8): i64` — 4-byte LE size prefix + raw block (`-1` on error)
- `decompressWithSize(src: &[]u8, dst: []u8): i64` — validates 4-byte LE size prefix & decompresses (`-1` on error)

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
