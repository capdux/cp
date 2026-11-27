# twox-hash — AI Agent Quick Ref

Zero-heap pure-CP XXHash32 and XXHash64 fast non-cryptographic streaming hashers (crates.io twox-hash equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    XxHash32,
    XxHash64,
    xxhU64,
    xxhAdd32,
    xxhSub32,
    xxhMul32,
    xxhRol32,
    xxhAdd64,
} from "twox-hash";
```

## Core Types & Constants

```cp
class XxHash32 { seed: u32, v1: u32, v2: u32, v3: u32, /* +4 fields */ } // methods: withSeed(...) +4
class XxHash64 { seed: u64, v1: u64, v2: u64, v3: u64, /* +4 fields */ } // methods: withSeed(...) +4
```

## API Signatures

```cp
fn xxhU64(hi: u32, lo: u32): u64;
fn xxhAdd32(a: u32, b: u32): u32;
fn xxhSub32(a: u32, b: u32): u32;
fn xxhMul32(a: u32, b: u32): u32;
fn xxhRol32(x: u32, r: u32): u32;
fn xxhAdd64(a: u64, b: u64): u64;
fn xxhSub64(a: u64, b: u64): u64;
fn xxhMul64(a: u64, b: u64): u64;
fn xxhRol64(x: u64, r: u64): u64;
fn xxh32Round(acc: u32, lane: u32): u32;
fn xxh64Prime1(): u64;
fn xxh64Prime2(): u64;
fn xxh64Prime3(): u64;
fn xxh64Prime4(): u64;
fn xxh64Prime5(): u64;
fn xxh64Round(acc: u64, lane: u64): u64;
fn xxh64MergeRound(acc: u64, val: u64): u64;
```

## Minimal Usage

```cp
import { xxhU64 } from "twox-hash";

pub fn main() {
    let result = xxhU64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"twox-hash"`; install via `cap add twox-hash`.
