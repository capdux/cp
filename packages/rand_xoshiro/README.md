[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rand_xoshiro

Zero-allocation Blackman & Vigna xoshiro256** PRNG with SplitMix64 seeding, unbiased nextRange, 2^128 jump, and Fisher-Yates shuffle in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_xoshiro` ([https://github.com/rust-random/rngs](https://github.com/rust-random/rngs))（原库名 `rand_xoshiro` → CP 包名 `rand_xoshiro`）

| Metadata | Value |
| --- | --- |
| Package | `rand_xoshiro` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rand_xoshiro
```

```cp
import {
    Xoshiro256,
    makeU64,
    rotl7,
    rotl45,
    splitmix64Next,
    seedFromU64,
    nextU64,
    nextF64,
} from "rand_xoshiro";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Xoshiro256` | `class` | Fields: `s: [4]u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `rotl7` | `(x: u64): u64` | Invokes `rotl7` and returns `u64`. |
| `rotl45` | `(x: u64): u64` | Invokes `rotl45` and returns `u64`. |
| `splitmix64Next` | `(state: &mut u64): u64` | Invokes `splitmix64Next` and returns `u64`. |
| `seedFromU64` | `(seed: u64): Xoshiro256` | Invokes `seedFromU64` and returns `Xoshiro256`. |
| `nextU64` | `(rng: &mut Xoshiro256): u64` | Invokes `nextU64` and returns `u64`. |
| `nextF64` | `(rng: &mut Xoshiro256): f64` | Invokes `nextF64` and returns `f64`. |
| `nextRange` | `(rng: &mut Xoshiro256, upper_exclusive: u64): u64` | Invokes `nextRange` and returns `u64`. |
| `jump` | `(rng: &mut Xoshiro256)` | Executes `jump` in-place. |
| `shuffleU32` | `(rng: &mut Xoshiro256, data: []u32)` | Executes `shuffleU32` in-place. |

## Minimal Example

```cp
import { makeU64 } from "rand_xoshiro";

pub fn main() {
    let result = makeU64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
