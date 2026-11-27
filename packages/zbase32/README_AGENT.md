# zbase32@1.0.0 (Agent Quickref)

> Zero-allocation Zooko human-oriented z-base-32 encoder, decoder, and validator in pure CP | License: MIT
> Import: `import { ... } from "zbase32";` | Full types: `src/index.d.cp`

## Functions
- `export fn decodeChar(b: u8): i32;`
- `export fn encodedLen(byte_len: usize): usize;`
- `export fn encode(data: &[]u8, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
- `export fn isValidZBase32(input: string): bool;`
