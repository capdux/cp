[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# oorandom

Zero-heap deterministic PCG32 (`Rand32`) and dual-stream PCG64 (`Rand64`) PRNG with signed/unsigned range sampling, float generation, byte filling, state snapshotting, and O(log N) jump-ahead. Modeled after Rust `oorandom`.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `oorandom` (`~icefox/oorandom`) |
| **CP Package** | `oorandom` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: oorandom)
use oorandom::Rand32;
let mut rng = Rand32::new(42);
let roll = rng.rand_range(1..7);
```

```typescript
// CP (same package name: "oorandom")
import { Rand32 } from "oorandom";
let mut rng = Rand32.new(42);
let roll = rng.randRange(1, 7);
```

## Installation

```bash
cap add oorandom
```

## Quick Start & Practical Examples

### 1. `Rand32` — 32-Bit Deterministic PRNG

```typescript
import { Rand32 } from "oorandom";

fn demoRand32(): void {
    let mut rng = Rand32.new(42);
    let mut with_stream = Rand32.newInc(42, 7);

    let u = rng.randU32();
    let i = rng.randI32();
    let f = rng.randFloat();             // f32 in [0.0, 1.0)
    let b = rng.randBool();
    let dice = rng.randRange(1, 7);      // u32 in [1, 7)
    let temp = rng.randRangeI32(-10, 11); // i32 in [-10, 11)

    let mut buf: [16]u8 = undefined;
    rng.fillBytes(buf[0..16]);

    // Save & restore exact generator state
    let pair = rng.statePair();          // [st, inc]
    let mut restored = Rand32.fromState(pair[0], pair[1]);

    // O(log N) jump-ahead
    with_stream.advance(5000);
}
```

### 2. `Rand64` — Dual-Stream 64-Bit PRNG

```typescript
import { Rand64 } from "oorandom";

fn demoRand64(): void {
    let mut rng = Rand64.new(123456789);
    let mut custom = Rand64.newInc(1, 2, 3, 4);

    let u = rng.randU64();
    let i = rng.randI64();
    let f = rng.randFloat();              // f64 in [0.0, 1.0)
    let b = rng.randBool();
    let slot = rng.randRange(100, 200);   // u64 in [100, 200)
    let delta = rng.randRangeI64(-50, 50);

    let mut buf: [32]u8 = undefined;
    rng.fillBytes(buf[0..32]);

    rng.advance(100000);
    let st = rng.state(); // [lo.st, hi.st]
}
```

### 3. Low-Level PCG Primitives

```typescript
import { oorStep, oorOutput, oorAdvanceState } from "oorandom";

fn manualStep(): u32 {
    let st0: u64 = 12345;
    let inc: u64 = 1442695040888963407;
    let st1 = oorStep(st0, inc);
    let st100 = oorAdvanceState(st0, inc, 100);
    return oorOutput(st100);
}
```

## API Reference

### `Rand32`
- `Rand32.new(seed: u64): Rand32`, `Rand32.newInc(seed: u64, inc: u64): Rand32`, `Rand32.fromState(st: u64, inc: u64): Rand32`
- `randU32(): u32`, `randI32(): i32`, `randFloat(): f32`, `randBool(): bool`
- `randRange(start: u32, end: u32): u32`, `randRangeI32(start: i32, end: i32): i32`
- `fillBytes(dst: []u8)`, `advance(delta: u64)`
- `state(): u64`, `statePair(): [2]u64`

### `Rand64`
- `Rand64.new(seed: u64): Rand64`, `Rand64.newInc(seed_lo: u64, seed_hi: u64, inc_lo: u64, inc_hi: u64): Rand64`
- `randU64(): u64`, `randI64(): i64`, `randFloat(): f64`, `randBool(): bool`
- `randRange(start: u64, end: u64): u64`, `randRangeI64(start: i64, end: i64): i64`
- `fillBytes(dst: []u8)`, `advance(delta: u64)`, `state(): [2]u64`

### Primitives
- `oorStep(st: u64, inc: u64): u64`, `oorOutput(oldstate: u64): u32`, `oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64`
