# murmur3 — AI Agent Quick Ref

Zero-heap MurmurHash3 (32-bit x86 & 128-bit x64) deterministic non-cryptographic hash functions for sharding, Kafka/Cassandra/ClickHouse partitioning (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Hash128,
    rotl32,
    rotl64,
    fmix32,
    fmix64,
    murmur3_32,
    murmur3_128,
} from "murmur3";
```

## Core Types & Constants

```cp
class Hash128 { lo: u64, hi: u64 }
```

## API Signatures

```cp
fn rotl32(x: u32, r: u5): u32;
fn rotl64(x: u64, r: u6): u64;
fn fmix32(h_in: u32): u32;
fn fmix64(k_in: u64): u64;
fn murmur3_32(data: string, seed: u32): u32;
fn murmur3_128(data: string, seed: u64): Hash128;
```

## Minimal Usage

```cp
import { murmur3_32 } from "murmur3";

pub fn main() {
    let result = murmur3_32("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"murmur3"`; install via `cap add murmur3`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
