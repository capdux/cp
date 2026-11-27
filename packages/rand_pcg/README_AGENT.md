# rand_pcg — AI Agent Quick Ref

Zero-heap deterministic `Pcg32` (PCG-XSH-RR 64/32) and `Pcg64Mcg` (128-bit MCG) PRNGs with independent streams and O(log N) jump-ahead.

## Import
```typescript
import { Pcg32, Pcg64Mcg, U128Pair, splitmix64Next } from "rand_pcg";
```

## API Signatures
```typescript
export class Pcg32 {
    state: u64, inc: u64,
    fn init(seed: u64, stream_id: u64): Pcg32;
    fn new(state: u64, stream: u64): Pcg32;
    fn seedFromU64(seed: u64): Pcg32;
    fn nextU32(*_): u32;
    fn nextU64(*_): u64;
    fn nextBoundedU32(*_, bound: u32): u32;
    fn nextF32(*_): f32;
    fn nextF64(*_): f64;
    fn nextBool(*_): bool;
    fn fillBytes(*_, dst: []u8);
    fn advance(*_, delta_in: u64);
}

export class Pcg64Mcg {
    state_hi: u64, state_lo: u64,
    fn new(state_hi: u64, state_lo: u64): Pcg64Mcg;
    fn seedFromU64(seed: u64): Pcg64Mcg;
    fn nextU64(*_): u64;
    fn nextU32(*_): u32;
    fn nextF64(*_): f64;
    fn fillBytes(*_, dst: []u8);
    fn advance(*_, delta_in: u64);
}
// Math helpers: U128Pair, wrapAdd64, wrapMul64, mul64Full, wrapMul128, rotr32, rotr64, splitmix64Next
```

## Minimal Example
```typescript
import { Pcg32, Pcg64Mcg } from "rand_pcg";

fn sample(): u64 {
    let mut r32 = Pcg32.seedFromU64(42);
    let mut r64 = Pcg64Mcg.seedFromU64(42);
    let a = r32.nextBoundedU32(100) as u64;
    let b = r64.nextU64();
    return a ^ b;
}
```

## Pitfalls & Rules
- All `next*`, `fillBytes`, and `advance` methods mutate generator state; bind instances with `let mut`.
- `nextF32` / `nextF64` produce values in `[0.0, 1.0)`; `nextBoundedU32(bound)` produces `[0, bound)`.
- `fillBytes` mutates caller-owned `dst: []u8` in place (zero allocation).
- Non-cryptographic PRNG: do NOT use for passwords, keys, or security nonces.
