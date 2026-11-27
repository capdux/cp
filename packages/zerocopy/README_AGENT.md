# zerocopy@1.0.0 (Agent Quickref)

> Zero-heap zero-copy prefix/suffix byte slice cursor readers & writers with layout validation (crates.io zerocopy equivalent, reuses @cap/bytemuck-crate) | License: MIT
> Import: `import { ... } from "zerocopy";` | Full types: `src/index.d.cp`
> Deps: `bytemuck`

## Functions
- `export fn isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool;`
- `export fn readFromPrefix(buf: &[]u8, n: usize): SplitBytes?;`
- `export fn readFromSuffix(buf: &[]u8, n: usize): SplitBytes?;`
- `export fn readU16LeFromPrefix(buf: &[]u8): PrefixU16?;`
- `export fn readU16BeFromPrefix(buf: &[]u8): PrefixU16?;`
- `export fn readU32LeFromPrefix(buf: &[]u8): PrefixU32?;`
- `export fn readU32BeFromPrefix(buf: &[]u8): PrefixU32?;`
- `export fn readU64LeFromPrefix(buf: &[]u8): PrefixU64?;`
- `export fn readU64BeFromPrefix(buf: &[]u8): PrefixU64?;`
- `export fn readU16LeFromSuffix(buf: &[]u8): PrefixU16?;`
- `export fn readU32LeFromSuffix(buf: &[]u8): PrefixU32?;`
- `export fn readU64LeFromSuffix(buf: &[]u8): PrefixU64?;`
- `export fn writeToPrefix(dst: []u8, src: &[]u8): ([]u8)?;`
- `export fn writeToSuffix(dst: []u8, src: &[]u8): ([]u8)?;`
- `export fn refFromBytesU16(buf: &[]u8): u16?;`
- `export fn refFromBytesU32(buf: &[]u8): u32?;`
- `export fn refFromBytesU64(buf: &[]u8): u64?;`
- `export fn refFromBytesSliceU32(buf: &[]u8): (&[]u32)?;`
- `export fn sliceAsBytesU16(src: &[]u16): &[]u8;`
- `export fn sliceAsBytesU32(src: &[]u32): &[]u8;`
- `export fn sliceAsBytesU64(src: &[]u64): &[]u8;`
