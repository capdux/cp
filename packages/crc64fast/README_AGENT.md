# crc64fast@1.0.0 (Agent Quickref)

> Zero-allocation 64-bit CRC checksum engine supporting ECMA-182, CRC-64/XZ, and CRC-64/GO-ISO with 256-entry lookup tables and streaming hasher (Rust crc / crc64fast parity) for CP | License: MIT
> Import: `import { ... } from "crc64fast";` | Full types: `src/index.d.cp`

## Functions
- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn crc64Ecma(data: &[]u8): u64;`
- `export fn crc64Xz(data: &[]u8): u64;`
- `export fn crc64Iso(data: &[]u8): u64;`
