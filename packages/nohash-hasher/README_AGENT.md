# nohash-hasher@0.2.0 (Agent Quickref)

> Zero-overhead identity integer hasher and Fibonacci/SplitMix64 integer key mixers for hash maps | License: Apache-2.0
> Import: `import { ... } from "nohash-hasher";` | Full types: `src/index.d.cp`

## Functions
- `export fn newIntHasher(): IntHasher;`
- `export fn nohashU64(key: u64): u64;`
- `export fn nohashU32(key: u32): u64;`
- `export fn fibBucketIndex(key: u64, bits: u6): usize;`
- `export fn mixU64(x: u64): u64;`
