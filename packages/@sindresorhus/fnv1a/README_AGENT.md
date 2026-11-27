# @sindresorhus/fnv1a@1.0.0 (Agent Quickref)

> Zero-heap FNV-1 and FNV-1a 32-bit and 64-bit streaming non-cryptographic hash functions for CP (compatible with Rust fnv & npm @sindresorhus/fnv1a) | License: MIT
> Import: `import { ... } from "@sindresorhus/fnv1a";` | Full types: `src/index.d.cp`

## Functions
- `export fn fnv64OffsetBasis(): u64;`
- `export fn fnv64Prime(): u64;`
- `export fn mulWrap32(a: u32, b: u32): u32;`
- `export fn mulFnv64Prime(h: u64): u64;`
- `export fn fnv1a32Update(state: u32, data: &[]u8): u32;`
- `export fn fnv1a32(data: &[]u8): u32;`
- `export fn fnv132(data: &[]u8): u32;`
- `export fn fnv1a64Update(state: u64, data: &[]u8): u64;`
- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn fnv164(data: &[]u8): u64;`
- `export fn formatHex64(h: u64, out: []u8): usize;`
