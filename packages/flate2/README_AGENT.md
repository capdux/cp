# flate2 — AI Agent Quick Ref

- **Package**: `flate2` (`1.0.0`) | **Source**: crates.io `flate2` (RFC 1952 Gzip / RFC 1950 Zlib stored-DEFLATE)
- **Import**: `import { encodeGzipStored, decodeGzipStored, encodeZlibStored, decodeZlibStored, parseGzipHeader } from "flate2";`
- **Allocation**: Zero-heap; returns `usize?` (`null` on error or insufficient buffer).

## Constants & Types

```typescript
GZIP_ID1: u8 = 31; GZIP_ID2: u8 = 139; GZIP_CM_DEFLATE: u8 = 8;
ZLIB_CMF_DEFAULT: u8 = 120; ZLIB_FLG_NO_COMP: u8 = 1;

export class GzipHeader {
    method: u8, flags: u8, mtime: u32, os: u8, data_offset: usize, valid: bool,
}
```

## Core Signatures

```typescript
export fn parseGzipHeader(frame: []u8): GzipHeader;
export fn encodeGzipStored(payload: []u8, mtime: u32, out: []u8): usize?;
export fn decodeGzipStored(frame: []u8, out: []u8): usize?;
export fn encodeZlibStored(payload: []u8, out: []u8): usize?;
export fn decodeZlibStored(frame: []u8, out: []u8): usize?;
export fn fl2StoredEncodedLen(payload_len: usize): usize;
export fn fl2EncodeStoredBlocks(payload: []u8, out: []u8, start: usize): usize;
export fn fl2DecodeStoredBlocks(stream: []u8, out: []u8): usize?;
export fn fl2EmptyGzipHeader(): GzipHeader;
```

## Minimal Pattern

```typescript
import { encodeGzipStored, decodeGzipStored } from "flate2";

let enc_buf: [256]u8 = undefined;
const n = encodeGzipStored(payload, 0, enc_buf[0..]);
if (n != null) {
    let dec_buf: [256]u8 = undefined;
    const m = decodeGzipStored(enc_buf[0..n.?], dec_buf[0..]);
}
```

## Pitfalls

1. **Stored DEFLATE only**: `decodeGzipStored` / `decodeZlibStored` decode uncompressed stored-DEFLATE blocks (`BTYPE=00`); Huffman-compressed streams return `null`. For high-ratio compression use `zstd`.
2. **Buffer overhead**: Gzip adds `18 + 5 * ceil(len / 65535)` bytes (minimum 23 bytes for empty payload); Zlib adds `6 + 5 * ceil(len / 65535)` bytes.
3. **Optional return**: Always check `!= null` before unwrapping `.?`.
4. **Slice parameter type**: Signatures take `[]u8` (mutable slice view).
