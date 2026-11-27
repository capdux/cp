# snap@1.0.0 (Agent Quickref)

> Zero-allocation Google Snappy Raw Block lossless compressor & decompressor with varint32 header, Literal/Copy-1/Copy-2/Copy-4 tags, and overlapping RLE support (Rust snap parity) for CP | License: MIT
> Import: `import { ... } from "snap";` | Full types: `src/index.d.cp`

## Functions
- `export fn maxCompressedLen(input_len: usize): usize;`
- `export fn decodeVarint32(src: &[]u8): VarintRes;`
- `export fn encodeVarint32(val: u32, dst: []u8, start_pos: usize): i64;`
- `export fn uncompressedLen(src: &[]u8): i64;`
- `export fn hash4(buf: &[]u8, pos: usize): usize;`
- `export fn match4(buf: &[]u8, a: usize, b: usize): bool;`
- `export fn emitLiteral(src: &[]u8, start: usize, len: usize, dst: []u8, start_pos: usize): i64;`
- `export fn emitCopyChunk(offset: usize, chunk_len: usize, dst: []u8, start_pos: usize): i64;`
- `export fn emitCopy(offset: usize, total_len: usize, dst: []u8, start_pos: usize): i64;`
- `export fn compress(src: &[]u8, dst: []u8): i64;`
- `export fn decompress(src: &[]u8, dst: []u8): i64;`
