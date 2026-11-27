# @cap/lz4@1.10.0 (Agent Quickref)

> Extremely fast lossless compression (LZ4 & LZ4-HC) native C binding for CP | License: BSD-2-Clause
> Import: `import { ... } from "@cap/lz4";` | Full types: `src/index.d.cp`

## Functions
- `export fn LZ4_versionNumber(): i32;`
- `export fn LZ4_compressBound(inputSize: i32): i32;`
- `export fn LZ4_compress_default(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32): i32;`
- `export fn LZ4_compress_fast(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, acceleration: i32): i32;`
- `export fn LZ4_compress_HC(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, compressionLevel: i32): i32;`
- `export fn LZ4_decompress_safe(src: [*]&u8!, dst: [*]u8!, compressedSize: i32, dstCapacity: i32): i32;`
- `export fn versionNumber(): i32;`
- `export fn compressBound(input_len: usize): !usize;`
- `export fn compressInto(src: &[]u8, dst: []u8): !&[]u8;`
- `export fn compressFastInto(src: &[]u8, dst: []u8, acceleration: i32): !&[]u8;`
- `export fn compressHCInto(src: &[]u8, dst: []u8, level: i32): !&[]u8;`
- `export fn decompressInto(src: &[]u8, dst: []u8): !&[]u8;`
