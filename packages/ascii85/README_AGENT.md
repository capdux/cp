# ascii85@1.0.0 (Agent Quickref)

> Zero-allocation Adobe Ascii85 (PostScript / PDF base-85 with 'z' zero compression and <~...~> delimiters) encoder and decoder for CP. | License: MIT
> Import: `import { ... } from "ascii85";` | Full types: `src/index.d.cp`

## Functions
- `export fn maxEncodedLen(byte_len: usize, with_delimiters: bool): usize;`
- `export fn encode(data: &[]u8, with_delimiters: bool, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
