# crc32fast@1.0.0 (Agent Quickref)

> Zero-heap IEEE 802.3 CRC-32 streaming Hasher with GF(2) combine, reset, and one-shot hash (crates.io crc32fast equivalent) | License: MIT
> Import: `import { ... } from "crc32fast";` | Full types: `src/index.d.cp`

## Functions
- `export fn crcByteStep(crc_in: u32, b: u8): u32;`
- `export fn crcBuildTable(): [256]u32;`
- `export fn crcGf2MatrixTimes(mat: [32]u32, vec: u32): u32;`
- `export fn crcGf2MatrixSquare(src: [32]u32): [32]u32;`
- `export fn combineCrc32(crc1: u32, crc2: u32, len2: u64): u32;`
- `export fn hashBytes(buf: &[]u8): u32;`
- `export fn hashStr(s: string): u32;`
