# crc32c@1.0.0 (Agent Quickref)

> Zero-heap pure-CP CRC-32C (Castagnoli) checksum with streaming append, GF(2) combine, and LevelDB/RocksDB/GCS masking (crates.io crc32c equivalent) | License: MIT
> Import: `import { ... } from "crc32c";` | Full types: `src/index.d.cp`

## Functions
- `export fn c32cByteStep(crc_in: u32, b: u8): u32;`
- `export fn c32cGf2MatrixTimes(mat: [32]u32, vec: u32): u32;`
- `export fn c32cGf2MatrixSquare(src: [32]u32): [32]u32;`
- `export fn crc32cAppend(crc: u32, data: []u8): u32;`
- `export fn crc32c(data: []u8): u32;`
- `export fn crc32cStr(s: string): u32;`
- `export fn crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32;`
- `export fn maskCrc32c(crc: u32): u32;`
- `export fn unmaskCrc32c(masked: u32): u32;`
