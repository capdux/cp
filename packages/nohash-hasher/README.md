[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# nohash-hasher

Zero-overhead identity integer hasher, Fibonacci/SplitMix64 key mixers, and fixed-capacity stack hash containers (`IntMap64`, `IntSet64`) in pure CP.

- **Upstream Alignment**: `crates.io` `nohash-hasher` `v0.2.0` (Apache-2.0)
- **Package Version**: `0.2.0`
- **Memory Model**: Zero heap allocation — identity bit-cast hashing and fixed 64-slot open-addressed stack tables.

## Installation & Import

```bash
cap add nohash-hasher
```

```cp
import {
    INT_TABLE_CAP,
    IntHasher,
    NoHashHasher,
    BuildNoHashHasher,
    newIntHasher,
    newNoHashHasher,
    nohashU64,
    nohashI32,
    fibBucketIndex,
    mixU64,
    unmixU64,
    IntMap64,
    IntSet64,
} from "nohash-hasher";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `INT_TABLE_CAP` | `const INT_TABLE_CAP: usize` | Fixed slot capacity of `IntMap64` and `IntSet64` (`64`). |
| `IntHasher` / `NoHashHasher` | `class IntHasher { state: u64, ... }` | Identity hasher with `static fn new()`, `writeU8..writeUsize`, `writeI8..writeIsize`, `finish(): u64`, `reset()`, `clone()`. |
| `BuildNoHashHasher` | `class BuildNoHashHasher { seed: u64, ... }` | Hasher builder with `static fn new()` and `buildHasher(): IntHasher`. |
| `newIntHasher` / `newNoHashHasher` | `fn newIntHasher(): IntHasher` | Free-function constructors for `IntHasher` / `NoHashHasher`. |
| `nohashU8`..`nohashUsize` | `fn nohashU64(key: u64): u64` (and `U8`, `U16`, `U32`, `Usize`) | One-shot zero-cost unsigned integer identity hash functions. |
| `nohashI8`..`nohashIsize` | `fn nohashI64(key: i64): u64` (and `I8`, `I16`, `I32`, `Isize`) | One-shot signed integer identity hash functions via bit-cast. |
| `fibBucketIndex` | `fn fibBucketIndex(key: u64, bits: u6): usize` | Fibonacci multiplication bucket mapper into `0..(1 << bits)`. |
| `mixU64` / `unmixU64` | `fn mixU64(x: u64): u64` / `fn unmixU64(x: u64): u64` | Bijective SplitMix64 avalanche mixer and its exact inverse. |
| `IntMap64` | `class IntMap64 { ... }` | Stack-allocated 64-slot `u64 -> u64` map: `init()`, `len()`, `isEmpty()`, `clear()`, `insert(key, val): bool`, `get(key): u64?`, `containsKey(key): bool`, `remove(key): u64?`. |
| `IntSet64` | `class IntSet64 { ... }` | Stack-allocated 64-slot `u64` set: `init()`, `len()`, `isEmpty()`, `clear()`, `insert(key): bool`, `contains(key): bool`, `remove(key): bool`. |

## Examples

### 1. Identity Hashing & Reversible Mixing

```cp
import { IntHasher, nohashU64, mixU64, unmixU64, fibBucketIndex } from "nohash-hasher";

fn hashDemo(): void {
    let mut h = IntHasher.new();
    h.writeU64(42);
    const digest = h.finish(); // 42

    const direct = nohashU64(42); // 42
    const mixed = mixU64(42);
    const orig = unmixU64(mixed); // 42
    const bucket = fibBucketIndex(42, 6); // 0..63
}
```

### 2. Zero-Heap `IntMap64` and `IntSet64`

```cp
import { IntMap64, IntSet64 } from "nohash-hasher";

fn containerDemo(): void {
    let mut map = IntMap64.init();
    map.insert(1001, 99);
    const v = map.get(1001);       // Some(99)
    const old = map.remove(1001);  // Some(99)

    let mut set = IntSet64.init();
    set.insert(7);
    const has7 = set.contains(7);  // true
}
```

## Differences from Upstream & Highlights

- **Explicit Typed Methods**: Instead of Rust's `std::hash::Hasher` trait, `IntHasher` exposes explicit `writeU8..writeIsize` methods and standalone `nohash*` helpers for zero-abstraction inlining.
- **Built-in Stack Containers**: Includes ready-to-use `IntMap64` and `IntSet64` (`INT_TABLE_CAP == 64`) for ultra-fast, zero-heap integer lookup tables.
- **Invertible Mixing**: Adds `mixU64` / `unmixU64` (SplitMix64) and `fibBucketIndex` for cases where sequential keys need uniform bucket spread without cryptographic overhead.
