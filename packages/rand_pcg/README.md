[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# rand_pcg

Zero-heap PCG32 (XSH-RR) and PCG64Mcg (128-bit MCG) pseudorandom number generators with independent streams, unbiased bounded sampling, byte filling, and O(log N) jump-ahead (`advance`). Inspired by Rust `rand_pcg`.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `rand_pcg` (`rust-random/rand`) |
| **CP Package** | `rand_pcg` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: rand_pcg)
use rand_pcg::{Pcg32, Pcg64Mcg};
use rand_core::{RngCore, SeedableRng};
let mut rng = Pcg32::seed_from_u64(42);
let v = rng.next_u32();
```

```typescript
// CP (same package name: "rand_pcg")
import { Pcg32, Pcg64Mcg } from "rand_pcg";
let mut rng = Pcg32.seedFromU64(42);
let v = rng.nextU32();
```

## Installation

```bash
cap add rand_pcg
```

## Quick Start & Practical Examples

### 1. `Pcg32` — Independent Streams, Bounded Integers, Floats & Jump-Ahead

```typescript
import { Pcg32 } from "rand_pcg";

fn runPcg32(): void {
    // 3 ways to construct Pcg32:
    let mut r1 = Pcg32.seedFromU64(42);       // SplitMix64-expanded seed
    let mut r2 = Pcg32.init(12345, 1);        // (seed, stream_id)
    let mut r3 = Pcg32.new(0x1234, 0x5678);   // (raw state, raw stream)

    let u32_val = r1.nextU32();
    let u64_val = r1.nextU64();
    let die_roll = r1.nextBoundedU32(6);      // unbiased [0, 6)
    let f32_val = r1.nextF32();               // [0.0, 1.0)
    let f64_val = r1.nextF64();               // [0.0, 1.0)
    let coin = r1.nextBool();

    let mut bytes: [16]u8 = undefined;
    r1.fillBytes(bytes[0..16]);

    // O(log N) jump-ahead for parallel Monte Carlo workers
    r2.advance(1000000);
}
```

### 2. `Pcg64Mcg` — 128-Bit State High-Throughput 64-Bit Generator

```typescript
import { Pcg64Mcg } from "rand_pcg";

fn runPcg64Mcg(): void {
    let mut rng = Pcg64Mcg.seedFromU64(2026);
    let mut manual = Pcg64Mcg.new(0, 1);

    let x = rng.nextU64();
    let y = rng.nextU32();
    let p = rng.nextF64(); // [0.0, 1.0)

    let mut buf: [32]u8 = undefined;
    rng.fillBytes(buf[0..32]);

    // O(log N) jump-ahead in 128-bit MCG state space
    rng.advance(10000);
}
```

### 3. Low-Level Bit & Arithmetic Utilities

```typescript
import {
    wrapAdd64,
    wrapMul64,
    mul64Full,
    wrapMul128,
    rotr32,
    rotr64,
    splitmix64Next,
} from "rand_pcg";

fn lowLevelHelpers(): void {
    let mixed = splitmix64Next(42);
    let prod = mul64Full(0x100000000, 0x100000000); // U128Pair { hi, lo }
    let p128 = wrapMul128(prod.hi, prod.lo, 0, 3);
    let r = rotr32(0x12345678, 8);
    let r64 = rotr64(mixed, 13);
    let s = wrapAdd64(wrapMul64(mixed, 6364136223846793005), 1);
}
```

## API Reference

### `Pcg32`
- `Pcg32.init(seed: u64, stream_id: u64): Pcg32`
- `Pcg32.new(state: u64, stream: u64): Pcg32`
- `Pcg32.seedFromU64(seed: u64): Pcg32`
- `nextU32(): u32`, `nextU64(): u64`, `nextBoundedU32(bound: u32): u32`
- `nextF32(): f32`, `nextF64(): f64`, `nextBool(): bool`
- `fillBytes(dst: []u8)`, `advance(delta_in: u64)`

### `Pcg64Mcg`
- `Pcg64Mcg.new(state_hi: u64, state_lo: u64): Pcg64Mcg`
- `Pcg64Mcg.seedFromU64(seed: u64): Pcg64Mcg`
- `nextU64(): u64`, `nextU32(): u32`, `nextF64(): f64`
- `fillBytes(dst: []u8)`, `advance(delta_in: u64)`

### Low-Level Helpers
- `U128Pair { hi: u64, lo: u64 }`
- `wrapAdd64(a: u64, b: u64): u64`, `wrapMul64(a: u64, b: u64): u64`
- `mul64Full(a: u64, b: u64): U128Pair`, `wrapMul128(a_hi: u64, a_lo: u64, b_hi: u64, b_lo: u64): U128Pair`
- `rotr32(x: u32, r: u32): u32`, `rotr64(x: u64, r: u32): u64`, `splitmix64Next(x_in: u64): u64`
