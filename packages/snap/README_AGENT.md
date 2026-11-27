# snap@1.0.0 — AI Agent Quick Ref

- **Source**: `crates.io:snap` (`BurntSushi/rust-snappy`, BSD-3-Clause) | **Deps**: none
- **Import**: `import { compress, decompress, maxCompressedLen, uncompressedLen } from "snap";`
- **Full Types**: `packages/snap/src/index.d.cp`

## Core Types & Functions
```cp
class VarintRes { ok: bool, value: u32, bytes_read: usize } // internal varint decode result

fn maxCompressedLen(input_len: usize): usize  // 32 + input_len + input_len / 6
fn uncompressedLen(src: &[]u8): i64           // -1 on malformed varint32 header
fn compress(src: &[]u8, dst: []u8): i64       // returns compressed bytes or -1
fn decompress(src: &[]u8, dst: []u8): i64     // returns uncompressed bytes or -1
```

## Minimal Pattern
```cp
import { compress, decompress, maxCompressedLen, uncompressedLen } from "snap";

const src = "abcabcabcabcabcabc";
let comp: [64]u8 = undefined; // ensure >= maxCompressedLen(src.len)
const c_len = compress(src, comp[0..]);
if (c_len >= 0) {
    const raw_len = uncompressedLen(comp[0..(c_len as usize)]);
    let out: [64]u8 = undefined;
    const d_len = decompress(comp[0..(c_len as usize)], out[0..]);
    * = raw_len; * = d_len;
}
```

## Migration Map (Rust `snap::raw` → CP `snap`)
- `max_compress_len(n)` → `maxCompressedLen(n)`
- `decompress_len(src)` → `uncompressedLen(src)`
- `Encoder::compress` / `Decoder::decompress` → `compress(src, dst)` / `decompress(src, dst)`

## Pitfalls & Notes
1. **Negative Return = Error**: `compress`, `decompress`, and `uncompressedLen` return `-1` (`i64`) on insufficient output buffer, malformed varint, out-of-bounds back-reference, or length mismatch. Always check `>= 0` before casting to `usize`.
2. **Buffer Sizing**: Allocate `dst` of at least `maxCompressedLen(src.len)` for `compress`, and `uncompressedLen(compressed)` for `decompress`.
3. **Raw Block Format Only**: Implements Snappy Raw Block format (not Snappy Framing / `.sz` stream format).
4. **Perf & Memory**: Stack `[4096]u16 = undefined` hash table (`MAX_OFFSET = 65535`), `@memcpy` fast path for literals & non-overlapping copies, byte-wise RLE for overlapping copies (`offset < len`).
5. **Internal Helpers**: `decodeVarint32`, `encodeVarint32`, `hash4`, `match4`, `emitLiteral`, `emitCopyChunk`, `emitCopy` are internal building blocks — use the 4 public functions above.
