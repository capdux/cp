# leb128@1.0.0 (Agent Quickref)

> Zero-heap ULEB128 (unsigned) and SLEB128 (signed) variable-length integer encoder/decoder for WebAssembly, DWARF & binary formats (compatible with Rust leb128) | License: MIT
> Import: `import { ... } from "leb128";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodeUnsigned(val: u64, out: []u8): usize;`
- `export fn decodeUnsigned(data: &[]u8, start: usize): UlebResult;`
- `export fn encodeSigned(val: i64, out: []u8): usize;`
- `export fn decodeSigned(data: &[]u8, start: usize): SlebResult;`
- `export fn unsignedEncodedLen(val: u64): usize;`
