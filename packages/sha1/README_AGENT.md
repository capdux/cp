# sha1@1.0.0 (Agent Quickref)

> Zero-allocation FIPS 180-4 SHA-1 cryptographic hash streaming implementation (Rust sha1 crate parity) for CP | License: MIT
> Import: `import { ... } from "sha1";` | Full types: `src/index.d.cp`

## Functions
- `export fn sha1Add32(a: u32, b: u32): u32;`
- `export fn sha1Add5(a: u32, b: u32, c: u32, d: u32, e: u32): u32;`
- `export fn sha1Rotl1(x: u32): u32;`
- `export fn sha1Rotl5(x: u32): u32;`
- `export fn sha1Rotl30(x: u32): u32;`
- `export fn sha1ReadBe32(b: []u8, off: usize): u32;`
- `export fn sha1WriteBe32(out: []u8, off: usize, v: u32);`
- `export fn sha1HexDigit(nibble: u8): u8;`
- `export fn sha1Bytes(data: []u8, out20: []u8);`
- `export fn sha1Hex(s: string, out40: []u8): usize;`
