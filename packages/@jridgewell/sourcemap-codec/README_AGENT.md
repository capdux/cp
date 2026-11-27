# @jridgewell/sourcemap-codec@1.0.0 (Agent Quickref)

> Zero-heap Source Map v3 Base64 VLQ (Variable-Length Quantity) signed integer sequence encoder and decoder. | License: MIT
> Import: `import { ... } from "@jridgewell/sourcemap-codec";` | Full types: `src/index.d.cp`

## Functions
- `export fn b64Char(v: u8): u8;`
- `export fn b64Val(c: u8): i32;`
- `export fn encodeOne(val: i64, out: []u8): usize;`
- `export fn decodeOne(input: string, start: usize): DecodeResult;`
- `export fn encodeSlice(values: &[]i64, out: []u8): usize;`
- `export fn decodeSlice(input: string, out: []i64): i64;`
