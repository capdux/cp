# lz4_flex@1.0.0 — AI Agent Quick Ref

- **Import**: `import { maxCompressedLen, compressBlock, decompressBlock, compressWithSize, decompressWithSize } from "lz4_flex";`
- **Provenance**: crates.io `lz4_flex` (v0.11) / LZ4 Raw Block Format parity | Zero-heap | License: MIT
- **Perf (ReleaseFast, 4 KB JSON)**: `compressBlock` = **2.10 GB/s** (1.86 µs/op); `decompressBlock` = **3.12 GB/s** (1.25 µs/op, `@memcpy`)
- **Types**: `src/index.d.cp`

## Core Functions
```cp
export fn maxCompressedLen(input_len: usize): usize;
export fn compressBlock(src: &[]u8, dst: []u8): i64;
export fn decompressBlock(src: &[]u8, dst: []u8): i64;
export fn compressWithSize(src: &[]u8, dst: []u8): i64;
export fn decompressWithSize(src: &[]u8, dst: []u8): i64;
```
- Low-level helpers (internal/building blocks): `hash4(buf, pos)`, `match4(buf, a, b)`, `writeExtLen(dst, pos, extra)`.

## Return Semantics
- All compress/decompress functions return `i64`:
  - `>= 0`: exact number of bytes written to `dst`
  - `-1`: error (destination buffer too small, truncated input, invalid offset `0`, or declared size mismatch in `decompressWithSize`)

## Minimal Example
```cp
import { compressWithSize, decompressWithSize, maxCompressedLen } from "lz4_flex";

const src = "repeated-log-line repeated-log-line repeated-log-line";
let comp: [256]u8 = undefined;
const cn = compressWithSize(src, comp[0..]);
if (cn < 0) { return; }

let out: [256]u8 = undefined;
const dn = decompressWithSize(comp[0..@intCast(cn)], out[0..]);
if (dn < 0) { return; }
// out[0..@intCast(dn)] == src
```

## Gotchas
1. Always check `n < 0` before casting the returned `i64` to `usize`.
2. For `compressBlock`, ensure `dst.len >= maxCompressedLen(src.len)` to handle incompressible payloads.
3. For `compressWithSize`, ensure `dst.len >= maxCompressedLen(src.len) + 4` (4-byte little-endian uncompressed size prefix).
4. `decompressBlock` requires the caller to provide a `dst` slice large enough for the uncompressed size; if uncompressed size is unknown, use `compressWithSize` / `decompressWithSize`.
5. Wire format is byte-for-byte compatible with Rust `lz4_flex::block::compress` / `compress_prepend_size` (not LZ4 Frame `.lz4` container format).
