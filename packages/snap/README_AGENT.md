# snap — AI Agent Quick Ref

Zero-allocation Google Snappy Raw Block lossless compressor & decompressor with varint32 header, Literal/Copy-1/Copy-2/Copy-4 tags, and overlapping RLE support (Rust snap parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    VarintRes,
    maxCompressedLen,
    decodeVarint32,
    encodeVarint32,
    uncompressedLen,
    hash4,
    match4,
    emitLiteral,
} from "snap";
```

## Core Types & Constants

```cp
class VarintRes { ok: bool, value: u32, bytes_read: usize }
```

## API Signatures

```cp
fn maxCompressedLen(input_len: usize): usize;
fn decodeVarint32(src: &[]u8): VarintRes;
fn encodeVarint32(val: u32, dst: []u8, start_pos: usize): i64;
fn uncompressedLen(src: &[]u8): i64;
fn hash4(buf: &[]u8, pos: usize): usize;
fn match4(buf: &[]u8, a: usize, b: usize): bool;
fn emitLiteral(src: &[]u8, start: usize, len: usize, dst: []u8, start_pos: usize): i64;
fn emitCopyChunk(offset: usize, chunk_len: usize, dst: []u8, start_pos: usize): i64;
fn emitCopy(offset: usize, total_len: usize, dst: []u8, start_pos: usize): i64;
fn compress(src: &[]u8, dst: []u8): i64;
fn decompress(src: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { decodeVarint32 } from "snap";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeVarint32(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"snap"`; install via `cap add snap`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
