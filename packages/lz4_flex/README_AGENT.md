# lz4_flex@1.0.0 (Agent Quickref)

> Zero-allocation LZ4 Raw Block lossless compressor & decompressor with fixed 4096-entry hash table, overlapping RLE match copy, and size-prefixed framing (Rust lz4_flex / LZ4 Block Format parity) for CP | License: MIT
> Import: `import { ... } from "lz4_flex";` | Full types: `src/index.d.cp`

## Functions
- `export fn maxCompressedLen(input_len: usize): usize;`
- `export fn readU32LE(buf: &[]u8, pos: usize): u32;`
- `export fn hash4(buf: &[]u8, pos: usize): usize;`
- `export fn match4(buf: &[]u8, a: usize, b: usize): bool;`
- `export fn writeExtLen(dst: []u8, pos: usize, extra: usize): i64;`
- `export fn compressBlock(src: &[]u8, dst: []u8): i64;`
- `export fn decompressBlock(src: &[]u8, dst: []u8): i64;`
- `export fn compressWithSize(src: &[]u8, dst: []u8): i64;`
- `export fn decompressWithSize(src: &[]u8, dst: []u8): i64;`
