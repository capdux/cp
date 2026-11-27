# base64ct@1.0.0 (Agent Quickref)

> Pure CP zero-allocation constant-time (table-free) Base64 encoder and strict canonical decoder (RustCrypto base64ct parity) | License: MIT
> Import: `import { ... } from "base64ct";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodedLen(n: usize, padded: bool): usize;`
- `export fn encodeStd(src: []u8, out: []u8): usize;`
- `export fn encodeStdUnpadded(src: []u8, out: []u8): usize;`
- `export fn encodeUrl(src: []u8, out: []u8): usize;`
- `export fn encodeUrlUnpadded(src: []u8, out: []u8): usize;`
- `export fn decodeStd(src: string, out: []u8): DecodeResult;`
- `export fn decodeUrlUnpadded(src: string, out: []u8): DecodeResult;`
