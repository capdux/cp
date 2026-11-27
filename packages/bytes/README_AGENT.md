# bytes@1.0.0 (Agent Quickref)

> Unified zero-heap byte size parser/formatter (IEC KiB/MiB/GiB & SI KB/MB/GB) and zero-copy byte buffer cursors (BytesReader/BytesMut) for CP (superset of npm bytes & crates.io bytes) | License: MIT
> Import: `import { ... } from "bytes";` | Full types: `src/index.d.cp`

## Functions
- `export fn bytToLower(c: u8): u8;`
- `export fn bytIsSpace(c: u8): bool;`
- `export fn bytIsDigit(c: u8): bool;`
- `export fn bytUnitMultiplier(unit: string, binary_default: bool): u64;`
- `export fn bytU64Len(val: u64): usize;`
- `export fn parseBytes(input: string, binary_default: bool): i64;`
- `export fn parse(input: string): i64;`
- `export fn formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize;`
- `export fn formatIec(bytes: u64, out: []u8): usize;`
- `export fn formatSi(bytes: u64, out: []u8): usize;`
- `export fn bytbCopyBytes(dst: []u8, src: &[]u8, n: usize);`
