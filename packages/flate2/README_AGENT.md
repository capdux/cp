# flate2@1.0.0 (Agent Quickref)

> Zero-heap RFC 1952 Gzip & RFC 1950 Zlib stored-deflate framing codec with strict CRC-32/ISIZE/Adler-32 verification (crates.io flate2 equivalent) | License: MIT
> Import: `import { ... } from "flate2";` | Full types: `src/index.d.cp`
> Deps: `adler2`, `bytemuck`, `crc32fast`

## Functions
- `export fn fl2EmptyGzipHeader(): GzipHeader;`
- `export fn fl2StoredEncodedLen(payload_len: usize): usize;`
- `export fn fl2EncodeStoredBlocks(payload: []u8, out: []u8, start: usize): usize;`
- `export fn fl2DecodeStoredBlocks(stream: []u8, out: []u8): usize?;`
- `export fn parseGzipHeader(frame: []u8): GzipHeader;`
- `export fn encodeGzipStored(payload: []u8, mtime: u32, out: []u8): usize?;`
- `export fn decodeGzipStored(frame: []u8, out: []u8): usize?;`
- `export fn encodeZlibStored(payload: []u8, out: []u8): usize?;`
- `export fn decodeZlibStored(frame: []u8, out: []u8): usize?;`
