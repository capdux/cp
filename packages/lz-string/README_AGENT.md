# lz-string@1.0.0 (Agent Quickref)

> Zero-heap LZW byte/string compression with Base64 and URI-safe encoding for CP (clean-room MIT implementation of npm lz-string). | License: MIT
> Import: `import { ... } from "lz-string";` | Full types: `src/index.d.cp`

## Functions
- `export fn lzsHash(key: u32): usize;`
- `export fn lzsClearTable(ht_key: []u32);`
- `export fn lzsFind(ht_key: &[]u32, ht_val: &[]u16, key: u32): i32;`
- `export fn lzsInsert(ht_key: []u32, ht_val: []u16, key: u32, code: u16);`
- `export fn compressBytes(input: &[]u8, out: []u8): usize;`
- `export fn decompressBytes(compressed: &[]u8, out: []u8): i64;`
- `export fn lzsEncode64(raw: &[]u8, alphabet: string, pad: bool, out: []u8): usize;`
- `export fn lzsDecodeChar(ch: u8, uri_mode: bool): i32;`
- `export fn lzsDecode64(text: string, uri_mode: bool, out: []u8): i64;`
- `export fn compressToBase64(input: string, out: []u8): usize;`
- `export fn decompressFromBase64(b64: string, out: []u8): i64;`
- `export fn compressToEncodedURIComponent(input: string, out: []u8): usize;`
- `export fn decompressFromEncodedURIComponent(uri: string, out: []u8): i64;`
