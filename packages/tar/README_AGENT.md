# tar@1.0.0 (Agent Quickref)

> Zero-heap POSIX USTAR 512-byte tar archive header codec and streaming cursor (crates.io tar equivalent) | License: MIT
> Import: `import { ... } from "tar";` | Full types: `src/index.d.cp`

## Functions
- `export fn tarEmptyHeader(): TarHeader;`
- `export fn tarWriteOctal(out: []u8, offset: usize, digits: usize, val: u64): bool;`
- `export fn tarParseOctal(buf: []u8, offset: usize, width: usize, require_digit: bool): u64?;`
- `export fn tarComputeChecksum(block512: []u8): u32;`
- `export fn tarIsUstarMagic(block512: []u8): bool;`
- `export fn isZeroBlock(block512: []u8): bool;`
- `export fn tarPaddedSize(raw_size: u64): u64;`
- `export fn encodeTarHeader(name: string, size: u64, mode: u32, mtime: u64, typeflag: u8, out512: []u8): bool;`
- `export fn parseTarHeader(block512: []u8): TarHeader;`
