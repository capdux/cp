# blake2@1.0.0 (Agent Quickref)

> Zero-allocation RFC 7693 BLAKE2s-256 streaming cryptographic hash & keyed MAC (Rust blake2 crate parity) for CP | License: MIT
> Import: `import { ... } from "blake2";` | Full types: `src/index.d.cp`

## Functions
- `export fn bl2sAdd2(a: u32, b: u32): u32;`
- `export fn bl2sAdd3(a: u32, b: u32, c: u32): u32;`
- `export fn bl2sRotr16(x: u32): u32;`
- `export fn bl2sRotr12(x: u32): u32;`
- `export fn bl2sRotr8(x: u32): u32;`
- `export fn bl2sRotr7(x: u32): u32;`
- `export fn bl2sReadLe32(b: []u8, off: usize): u32;`
- `export fn bl2sWriteLe32(out: []u8, off: usize, v: u32);`
- `export fn bl2sHexDigit(nibble: u8): u8;`
- `export fn bl2sIv(idx: usize): u32;`
- `export fn bl2sMix(v: []u32, ai: usize, bi: usize, ci: usize, di: usize, x: u32, y: u32);`
- `export fn bl2sCompress(h: []u32, block: []u8, t: u64, is_last: bool);`
- `export fn blake2s256Bytes(data: []u8, out32: []u8);`
- `export fn blake2s256Hex(s: string, out64: []u8): usize;`
