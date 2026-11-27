# hamming@1.0.0 (Agent Quickref)

> Zero-allocation Hamming distance, byte popcount weight, 64-bit Charikar SimHash, and Hamming(7,4) error-correcting code in pure CP | License: MIT
> Import: `import { ... } from "hamming";` | Full types: `src/index.d.cp`

## Functions
- `export fn popcountU32(x: u32): u32;`
- `export fn popcountU8(b: u8): u32;`
- `export fn popcountU64(x: u64): u32;`
- `export fn hammingU64(a: u64, b: u64): u32;`
- `export fn weightBytes(data: &[]u8): u64;`
- `export fn distanceBytes(a: &[]u8, b: &[]u8): i64;`
- `export fn isTokenByte(b: u8): bool;`
- `export fn toLowerAscii(b: u8): u8;`
- `export fn hashToken64(text: &[]u8, start: usize, end: usize): u64;`
- `export fn simhash64(text: &[]u8): u64;`
- `export fn hamming74Encode(nibble: u8): u8;`
- `export fn hamming74Decode(codeword: u8): Hamming74Decoded;`
