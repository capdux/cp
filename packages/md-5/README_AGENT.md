# md-5@1.0.0 (Agent Quickref)

> Zero-allocation RFC 1321 MD5 cryptographic hash streaming implementation (Rust md-5/md5 crate parity) for CP | License: MIT
> Import: `import { ... } from "md-5";` | Full types: `src/index.d.cp`

## Functions
- `export fn md5Add32(a: u32, b: u32): u32;`
- `export fn md5Add4(a: u32, b: u32, c: u32, d: u32): u32;`
- `export fn md5Rotl32(x: u32, bits: u32): u32;`
- `export fn md5ShiftAt(idx: usize): u32;`
- `export fn md5ConstAt(idx: usize): u32;`
- `export fn md5ReadLe32(b: []u8, off: usize): u32;`
- `export fn md5WriteLe32(out: []u8, off: usize, v: u32);`
- `export fn md5HexDigit(nibble: u8): u8;`
- `export fn md5Bytes(data: []u8, out16: []u8);`
- `export fn md5Hex(s: string, out32: []u8): usize;`
