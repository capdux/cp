[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# oorandom

Zero-heap deterministic `Rand32` (PCG32 XSH-RR 64/32) and dual-stream `Rand64` PRNG in pure CP with state snapshotting, unbiased ranges, and $O(\log N)$ advance.

- **Upstream Alignment**: Modeled after Rust [`oorandom`](https://sr.ht/~icefox/oorandom/) (`crates.io`, `MIT`).
- **Package Version**: `1.0.0`

## Installation & Import

```bash
cap add oorandom
```

In `cap.json`:

```json
{
  "dependencies": {
    "oorandom": "^1.0.0"
  }
}
```

```typescript
import { Rand32, Rand64, oorStep, oorOutput, oorAdvanceState } from "oorandom";
```

## Core API Reference

### `class Rand32` (PCG32 XSH-RR 64/32)

Fields: `st: u64`, `inc: u64`.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `new` | `Rand32.new(seed: u64): Rand32` | Create with default increment stream |
| `newInc` | `Rand32.newInc(seed: u64, inc: u64): Rand32` | Create with explicit stream increment `inc` |
| `fromState` | `Rand32.fromState(st: u64, inc: u64): Rand32` | Restore generator directly from saved `(st, inc)` pair |
| `randU32` / `randI32` | `randU32(*_): u32` / `randI32(*_): i32` | Generate uniform 32-bit unsigned / signed integer |
| `randFloat` | `randFloat(*_): f32` | Generate uniform `f32` in `[0.0, 1.0)` |
| `randBool` | `randBool(*_): bool` | Generate uniform random boolean |
| `randRange` / `randRangeI32` | `randRange(*_, start: u32, end: u32): u32` / `randRangeI32(*_, start: i32, end: i32): i32` | Unbiased half-open range `[start, end)` |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | Fill caller-owned byte buffer `dst` |
| `advance` | `advance(*_, delta: u64)` | Jump forward `delta` steps in $O(\log N)$ |
| `state` / `statePair` | `state(): u64` / `statePair(): [2]u64` | Snapshot internal `st` or `[st, inc]` for deterministic replay |

### `class Rand64` (Dual-Stream PCG64)

Fields: `lo: Rand32`, `hi: Rand32`.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `new` | `Rand64.new(seed: u64): Rand64` | Create 64-bit generator from single `u64` seed |
| `newInc` | `Rand64.newInc(seed_lo: u64, seed_hi: u64, inc_lo: u64, inc_hi: u64): Rand64` | Create from explicit dual seeds and stream increments |
| `randU64` / `randI64` | `randU64(*_): u64` / `randI64(*_): i64` | Generate uniform 64-bit unsigned / signed integer |
| `randFloat` | `randFloat(*_): f64` | Generate uniform `f64` in `[0.0, 1.0)` |
| `randBool` | `randBool(*_): bool` | Generate uniform random boolean |
| `randRange` / `randRangeI64` | `randRange(*_, start: u64, end: u64): u64` / `randRangeI64(*_, start: i64, end: i64): i64` | Unbiased half-open 64-bit range `[start, end)` |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | Fill caller-owned byte buffer `dst` |
| `advance` | `advance(*_, delta: u64)` | Jump both streams forward `delta` steps in $O(\log N)$ |
| `state` | `state(): [2]u64` | Snapshot `[lo.st, hi.st]` |

### Low-Level Functions

`oorStep(st: u64, inc: u64): u64`, `oorOutput(oldstate: u64): u32`, `oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64`.

## Examples

### 1. Deterministic Gameplay Dice & State Snapshotting

```typescript
import { Rand32 } from "oorandom";

fn rollAndReplay(): bool {
    let mut rng = Rand32.new(12345);
    let snap = rng.statePair();
    let d20 = rng.randRange(1, 21);

    let mut replay = Rand32.fromState(snap[0], snap[1]);
    return replay.randRange(1, 21) == d20;
}
```

### 2. 64-Bit Range Sampling & Byte Filling

```typescript
import { Rand64 } from "oorandom";

fn sampleWorld(seed: u64, out: []u8): i64 {
    let mut rng = Rand64.new(seed);
    rng.fillBytes(out);
    return rng.randRangeI64(-1000, 1000);
}
```

## Differences from Upstream & CP Highlights

- **Zero-Dependency Minimalism**: Preserves `oorandom`'s tiny, self-contained philosophy—just `Rand32` and `Rand64` with no trait hierarchy or heap allocation.
- **Built-in State Snapshotting & Jump-Ahead**: Exposes `fromState`, `statePair`, `fillBytes`, and `advance` for deterministic game replays and parallel simulations.
- **Not Cryptographic**: Designed for simulations, procedural generation, and games—not security-sensitive randomness.
