# base62@1.0.0 (Agent Quickref)

> Zero-heap URL-safe Base62 (0-9A-Za-z) u64 and arbitrary byte-slice encoder/decoder in pure CP | License: MIT
> Import: `import { ... } from "base62";` | Full types: `src/index.d.cp`

## Functions
- `export fn charToVal(c: u8): i32;`
- `export fn isValidBase62(src: &[]u8): bool;`
- `export fn encodeU64(value: u64, dst: []u8): usize;`
- `export fn encodeU64Padded(value: u64, width: usize, dst: []u8): usize;`
- `export fn decodeU64(src: &[]u8): DecodeU64Result;`
- `export fn encodeBytes(src: &[]u8, dst: []u8): i64;`
- `export fn decodeBytes(src: &[]u8, dst: []u8): i64;`
