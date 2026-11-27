# simd-adler32@1.0.0 (Agent Quickref)

> Zero-allocation IEEE CRC-32, Castagnoli CRC-32C, Adler-32, and O(log N) CRC-32 combine engine (Rust crc32fast parity) for CP | License: MIT
> Import: `import { ... } from "simd-adler32";` | Full types: `src/index.d.cp`

## Functions
- `export fn crc32Ieee(data: string): u32;`
- `export fn crc32c(data: string): u32;`
- `export fn adler32(data: string): u32;`
- `export fn combineCrc32Ieee(crc1: u32, crc2: u32, len2: usize): u32;`
