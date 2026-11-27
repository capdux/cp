# weezl@1.0.0 (Agent Quickref)

> Zero-allocation GIF/TIFF/PDF LZW dictionary lossless compressor & decompressor with 9..12-bit variable code widths, Clear/EOI control codes, and KwKwK handling (Rust weezl parity) for CP | License: MIT
> Import: `import { ... } from "weezl";` | Full types: `src/index.d.cp`

## Functions
- `export fn hashPair(key: u32): usize;`
- `export fn encode(src: &[]u8, dst: []u8): i64;`
- `export fn decode(src: &[]u8, dst: []u8): i64;`
