# rend@1.0.0 (Agent Quickref)

> Zero-allocation endian-aware primitive types (U16/U32/I32/U64/I64 Be/Le) and bounds-checked slice readers for zero-copy binary protocols in CP | License: MIT
> Import: `import { ... } from "rend";` | Full types: `src/index.d.cp`

## Functions
- `export fn rndPackU16Be(v: u16): [2]u8;`
- `export fn rndPackU16Le(v: u16): [2]u8;`
- `export fn rndUnpackU16Be(b: [2]u8): u16;`
- `export fn rndUnpackU16Le(b: [2]u8): u16;`
- `export fn rndPackU32Be(v: u32): [4]u8;`
- `export fn rndPackU32Le(v: u32): [4]u8;`
- `export fn rndUnpackU32Be(b: [4]u8): u32;`
- `export fn rndUnpackU32Le(b: [4]u8): u32;`
- `export fn rndPackU64Be(v: u64): [8]u8;`
- `export fn rndPackU64Le(v: u64): [8]u8;`
- `export fn rndUnpackU64Be(b: [8]u8): u64;`
- `export fn rndUnpackU64Le(b: [8]u8): u64;`
- `export fn readU16Be(buf: &[]u8, offset: usize): u16?;`
- `export fn readU16Le(buf: &[]u8, offset: usize): u16?;`
- `export fn readU32Be(buf: &[]u8, offset: usize): u32?;`
- `export fn readU32Le(buf: &[]u8, offset: usize): u32?;`
- `export fn readU64Be(buf: &[]u8, offset: usize): u64?;`
- `export fn readU64Le(buf: &[]u8, offset: usize): u64?;`
