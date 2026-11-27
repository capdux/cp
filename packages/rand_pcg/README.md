[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rand_pcg

Zero-heap deterministic `Pcg32` (PCG-XSH-RR 64/32) and `Pcg64Mcg` (128-bit MCG PCG-XSL-RR) pseudorandom number generators with independent streams and $O(\log N)$ jump-ahead in pure CP.

- **Upstream Alignment**: Inspired by Rust [`rand_pcg`](https://github.com/rust-random/rand) (`crates.io`, `MIT`).
- **Package Version**: `1.0.0`

## Installation & Import

```bash
cap add rand_pcg
```

In `cap.json`:

```json
{
  "dependencies": {
    "rand_pcg": "^1.0.0"
  }
}
```

```typescript
import { Pcg32, Pcg64Mcg, U128Pair, splitmix64Next } from "rand_pcg";
```

## Core API Reference

### `class Pcg32` (PCG-XSH-RR 64/32)

Fields: `state: u64`, `inc: u64`.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `init` | `Pcg32.init(seed: u64, stream_id: u64): Pcg32` | Standard PCG32 seeding with initial state + stream selector |
| `new` | `Pcg32.new(state: u64, stream: u64): Pcg32` | Rust `Pcg32::new(state, stream)` compatible constructor |
| `seedFromU64` | `Pcg32.seedFromU64(seed: u64): Pcg32` | Seed via `SplitMix64` expansion |
| `nextU32` / `nextU64` | `nextU32(*_): u32` / `nextU64(*_): u64` | Generate next uniform 32-bit or 64-bit unsigned integer |
| `nextBoundedU32` | `nextBoundedU32(*_, bound: u32): u32` | Unbiased rejection-sampled integer in `[0, bound)` |
| `nextF32` / `nextF64` | `nextF32(*_): f32` / `nextF64(*_): f64` | Uniform float in `[0.0, 1.0)` |
| `nextBool` | `nextBool(*_): bool` | Uniform random boolean |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | Fill caller-owned byte buffer `dst` with random bytes |
| `advance` | `advance(*_, delta_in: u64)` | Jump state forward by `delta_in` steps in $O(\log N)$ time |

### `class Pcg64Mcg` (128-bit MCG PCG-XSL-RR)

Fields: `state_hi: u64`, `state_lo: u64`.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `new` | `Pcg64Mcg.new(state_hi: u64, state_lo: u64): Pcg64Mcg` | Construct from 128-bit seed pair (forces odd state) |
| `seedFromU64` | `Pcg64Mcg.seedFromU64(seed: u64): Pcg64Mcg` | Seed 128-bit state via `SplitMix64` |
| `nextU64` / `nextU32` | `nextU64(*_): u64` / `nextU32(*_): u32` | Generate next 64-bit / 32-bit unsigned integer |
| `nextF64` | `nextF64(*_): f64` | Uniform `f64` in `[0.0, 1.0)` |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | Fill `dst` with random bytes |
| `advance` | `advance(*_, delta_in: u64)` | Jump 128-bit MCG state forward by `delta_in` steps in $O(\log N)$ |

### Low-Level Arithmetic Helpers

Also exported: `U128Pair`, `wrapAdd64`, `wrapMul64`, `mul64Full`, `wrapMul128`, `rotr32`, `rotr64`, `splitmix64Next`.

## Examples

### 1. Deterministic Simulation with Independent Streams & Jump-Ahead

```typescript
import { Pcg32 } from "rand_pcg";

fn monteCarlo(seed: u64): f64 {
    let mut rng = Pcg32.new(seed, 0xa02bdbf7bb3c0a7);
    let mut hits: u64 = 0;
    let total: u64 = 1000;
    let mut i: u64 = 0;
    while (i < total) {
        let x = rng.nextF64();
        let y = rng.nextF64();
        if (x * x + y * y <= 1.0) { hits += 1; }
        i += 1;
    }
    return 4.0 * (hits as f64) / (total as f64);
}
```

### 2. High-Throughput 64-Bit Randomness with `Pcg64Mcg`

```typescript
import { Pcg64Mcg } from "rand_pcg";

fn fillNonce(seed: u64, out: []u8) {
    let mut rng = Pcg64Mcg.seedFromU64(seed);
    rng.advance(1024);
    rng.fillBytes(out);
}
```

## Differences from Upstream & CP Highlights

- **Zero-Heap Stack State**: `Pcg32` occupies 16 bytes (`state`, `inc`) and `Pcg64Mcg` occupies 16 bytes (`state_hi`, `state_lo`) with zero dependencies.
- **$O(\log N)$ Parallel Substreams**: Both `Pcg32.advance` and `Pcg64Mcg.advance` let parallel CP actors partition one deterministic sequence without overlap.
- **Not Cryptographic**: Like upstream `rand_pcg`, these PRNGs are designed for simulations, games, and randomized algorithms—not cryptographic keys or tokens.
