# lz4_flex — AI Agent Quick Ref

Zero-allocation LZ4 Raw Block lossless compressor & decompressor with fixed 4096-entry hash table, overlapping RLE match copy, and size-prefixed framing (Rust lz4_flex / LZ4 Block Format parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    maxCompressedLen,
    hash4,
    match4,
    writeExtLen,
    compressBlock,
    decompressBlock,
    compressWithSize,
    decompressWithSize,
} from "lz4_flex";
```

## API Signatures

```cp
fn maxCompressedLen(input_len: usize): usize;
fn hash4(buf: &[]u8, pos: usize): usize;
fn match4(buf: &[]u8, a: usize, b: usize): bool;
fn writeExtLen(dst: []u8, pos: usize, extra: usize): i64;
fn compressBlock(src: &[]u8, dst: []u8): i64;
fn decompressBlock(src: &[]u8, dst: []u8): i64;
fn compressWithSize(src: &[]u8, dst: []u8): i64;
fn decompressWithSize(src: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { hash4 } from "lz4_flex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hash4(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lz4_flex"`; install via `cap add lz4_flex`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
