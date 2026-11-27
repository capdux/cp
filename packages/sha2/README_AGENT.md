# sha2@1.0.0 (Agent Quickref)

> Zero-heap FIPS 180-4 SHA-256 & SHA-224 streaming hashers with hex output (crates.io sha2 equivalent, reuses cookie-signature) | License: MIT
> Import: `import { ... } from "sha2";` | Full types: `src/index.d.cp`
> Deps: `cookie-signature`

## Functions
- `export fn sh2HexDigit(n: u8): u8;`
- `export fn sh2EncodeHex(data: []u8, out: []u8): usize;`
- `export fn sha256Hex(s: string, out64: []u8): usize;`
- `export fn sha224Hex(s: string, out56: []u8): usize;`
