# oorandom — AI Agent Quick Ref

Zero-heap deterministic `Rand32` (PCG32 XSH-RR) and dual-stream `Rand64` PRNG with state snapshots, unbiased ranges, and O(log N) advance.

## Import
```typescript
import { Rand32, Rand64, oorStep, oorOutput, oorAdvanceState } from "oorandom";
```

## API Signatures
```typescript
export fn oorStep(st: u64, inc: u64): u64;
export fn oorOutput(oldstate: u64): u32;
export fn oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64;

export class Rand32 {
    st: u64, inc: u64,
    fn new(seed: u64): Rand32;
    fn newInc(seed: u64, inc: u64): Rand32;
    fn fromState(st: u64, inc: u64): Rand32;
    fn randU32(*_): u32;
    fn randI32(*_): i32;
    fn randFloat(*_): f32;
    fn randBool(*_): bool;
    fn randRange(*_, start: u32, end: u32): u32;
    fn randRangeI32(*_, start: i32, end: i32): i32;
    fn fillBytes(*_, dst: []u8);
    fn advance(*_, delta: u64);
    fn state(): u64;
    fn statePair(): [2]u64;
}

export class Rand64 {
    lo: Rand32, hi: Rand32,
    fn new(seed: u64): Rand64;
    fn newInc(seed_lo: u64, seed_hi: u64, inc_lo: u64, inc_hi: u64): Rand64;
    fn randU64(*_): u64;
    fn randI64(*_): i64;
    fn randFloat(*_): f64;
    fn randBool(*_): bool;
    fn randRange(*_, start: u64, end: u64): u64;
    fn randRangeI64(*_, start: i64, end: i64): i64;
    fn fillBytes(*_, dst: []u8);
    fn advance(*_, delta: u64);
    fn state(): [2]u64;
}
```

## Minimal Example
```typescript
import { Rand32, Rand64 } from "oorandom";

fn demo(): u64 {
    let mut r32 = Rand32.new(7);
    let mut r64 = Rand64.new(7);
    return (r32.randRange(1, 7) as u64) + r64.randRange(10, 20);
}
```

## Pitfalls & Rules
- All `rand*`, `fillBytes`, and `advance` methods mutate state; bind with `let mut`.
- `randRange` / `randRangeI32` / `randRangeI64` are half-open `[start, end)`; ensure `start < end`.
- `Rand32.randFloat` returns `f32` in `[0.0, 1.0)`; `Rand64.randFloat` returns `f64` in `[0.0, 1.0)`.
- Use `statePair()` + `Rand32.fromState(s[0], s[1])` to checkpoint and replay deterministically.
- Non-cryptographic PRNG: do NOT use for security tokens or keys.
