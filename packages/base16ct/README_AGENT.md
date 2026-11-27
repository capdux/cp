# base16ct@1.0.0 (Agent Quickref)

> Pure CP zero-allocation constant-time (table-free) Base16 / Hex encoder and strict decoder (RustCrypto base16ct parity) | License: MIT
> Import: `import { ... } from "base16ct";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodedLen(n: usize): usize;`
- `export fn encodeLower(src: []u8, out: []u8): usize;`
- `export fn encodeUpper(src: []u8, out: []u8): usize;`
- `export fn decodeLower(hex: string, out: []u8): HexDecodeResult;`
- `export fn decodeUpper(hex: string, out: []u8): HexDecodeResult;`
- `export fn decodeMixed(hex: string, out: []u8): HexDecodeResult;`
