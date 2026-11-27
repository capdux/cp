# oorandom — AI Agent Quick Ref

- **Import**: `import { Rand32, Rand64, oorStep, oorOutput, oorAdvanceState } from "oorandom";`
- **Source**: crates.io `oorandom` (`v1.0.0`, zero-heap deterministic PCG32 / dual-stream PCG64)

## Core Signatures
```cp
class Rand32 {
    st: u64, inc: u64,
    static new(seed: u64): Rand32;
    static newInc(seed: u64, inc: u64): Rand32;
    static fromState(st: u64, inc: u64): Rand32;
    randU32(): u32;
    randI32(): i32;
    randFloat(): f32;                      // [0.0, 1.0)
    randBool(): bool;
    randRange(start: u32, end: u32): u32;  // [start, end)
    randRangeI32(start: i32, end: i32): i32;
    fillBytes(dst: []u8);
    advance(delta: u64);
    state(): u64;
    statePair(): [2]u64;                   // [st, inc]
}

class Rand64 {
    lo: Rand32, hi: Rand32,
    static new(seed: u64): Rand64;
    static newInc(seed_lo: u64, seed_hi: u64, inc_lo: u64, inc_hi: u64): Rand64;
    randU64(): u64;
    randI64(): i64;
    randFloat(): f64;                      // [0.0, 1.0)
    randBool(): bool;
    randRange(start: u64, end: u64): u64;  // [start, end)
    randRangeI64(start: i64, end: i64): i64;
    fillBytes(dst: []u8);
    advance(delta: u64);
    state(): [2]u64;
}

oorStep(st: u64, inc: u64): u64
oorOutput(oldstate: u64): u32
oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64
```

## Pitfalls
- Declare `let mut rng = Rand32.new(seed)` — sampling and `advance` mutate state.
- `randRange(start, end)` is half-open `[start, end)`: ensure `start < end`.
- `Rand32.fromState` expects an odd `inc` (snapshot directly from `statePair()`).
- Not cryptographically secure; use `cap:crypto` for security-sensitive randomness.
