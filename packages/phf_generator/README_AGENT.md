# phf_generator@1.0.0 (Agent Quickref)

> Zero-allocation CHD Displace-Hash-Compress perfect hash table builder and O(1) lookup for CP | License: MIT
> Import: `import { ... } from "phf_generator";` | Full types: `src/index.d.cp`

## Functions
- `export fn add64(a: u64, b: u64): u64;`
- `export fn mul64(a: u64, b: u64): u64;`
- `export fn mix64(z_in: u64): u64;`
- `export fn strEq(a: string, b: string): bool;`
- `export fn hashKey(key: string, seed: u64): PhfHashes;`
- `export fn displace(f1: u32, f2: u32, d1: u32, d2: u32, len: usize): usize;`
- `export fn buildPhfWithSeed(keys: &[]string, start_seed: u64, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;`
- `export fn buildPhf(keys: &[]string, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;`
- `export fn lookupIndex(key: string, seed: u64, keys: &[]string, disps_d1: &[]u32, disps_d2: &[]u32, slot_to_key: &[]i32): i64;`
