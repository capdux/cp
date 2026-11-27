# murmur3@1.0.0 (Agent Quickref)

> Zero-heap MurmurHash3 (32-bit x86 & 128-bit x64) deterministic non-cryptographic hash functions for sharding, Kafka/Cassandra/ClickHouse partitioning | License: MIT
> Import: `import { ... } from "murmur3";` | Full types: `src/index.d.cp`

## Functions
- `export fn rotl32(x: u32, r: u5): u32;`
- `export fn rotl64(x: u64, r: u6): u64;`
- `export fn fmix32(h_in: u32): u32;`
- `export fn fmix64(k_in: u64): u64;`
- `export fn murmur3_32(data: string, seed: u32): u32;`
- `export fn murmur3_128(data: string, seed: u64): Hash128;`
