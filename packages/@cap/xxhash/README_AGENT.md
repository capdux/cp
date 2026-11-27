# @cap/xxhash@0.8.3 (Agent Quickref)

> xxHash 0.8.3 extremely fast non-cryptographic hash algorithm (XXH32/XXH64/XXH3) for CP | License: BSD-2-Clause
> Import: `import { ... } from "@cap/xxhash";` | Full types: `src/index.d.cp`

## Functions
- `export fn XXH_versionNumber(): u32;`
- `export fn XXH32(input: [*]&u8!, length: usize, seed: u32): u32;`
- `export fn XXH64(input: [*]&u8!, length: usize, seed: u64): u64;`
- `export fn XXH3_64bits(input: [*]&u8!, length: usize): u64;`
- `export fn versionNumber(): u32;`
- `export fn xxh32(data: &[]u8, seed: u32): u32;`
- `export fn xxh64(data: &[]u8, seed: u64): u64;`
- `export fn xxh3_64bits(data: &[]u8): u64;`
