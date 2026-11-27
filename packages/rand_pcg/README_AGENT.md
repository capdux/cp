# rand_pcg — AI Agent Quick Ref

- **Import**: `import { Pcg32, Pcg64Mcg, U128Pair, wrapAdd64, wrapMul64, mul64Full, wrapMul128, rotr32, rotr64, splitmix64Next } from "rand_pcg";`
- **Source**: crates.io `rand_pcg` (`v1.0.0`, zero-heap, non-cryptographic PRNG)

## Core Signatures
```cp
class Pcg32 {
    state: u64,
    inc: u64,
    static init(seed: u64, stream_id: u64): Pcg32;
    static new(state: u64, stream: u64): Pcg32;
    static seedFromU64(seed: u64): Pcg32;
    nextU32(): u32;
    nextU64(): u64;
    nextBoundedU32(bound: u32): u32;   // unbiased [0, bound)
    nextF32(): f32;                    // [0.0, 1.0)
    nextF64(): f64;                    // [0.0, 1.0)
    nextBool(): bool;
    fillBytes(dst: []u8);
    advance(delta_in: u64);            // O(log N) jump-ahead
}

class Pcg64Mcg {
    state_hi: u64,
    state_lo: u64,
    static new(state_hi: u64, state_lo: u64): Pcg64Mcg;
    static seedFromU64(seed: u64): Pcg64Mcg;
    nextU64(): u64;
    nextU32(): u32;
    nextF64(): f64;
    fillBytes(dst: []u8);
    advance(delta_in: u64);
}

class U128Pair { hi: u64, lo: u64 }
wrapAdd64(a: u64, b: u64): u64
wrapMul64(a: u64, b: u64): u64
mul64Full(a: u64, b: u64): U128Pair
wrapMul128(a_hi: u64, a_lo: u64, b_hi: u64, b_lo: u64): U128Pair
rotr32(x: u32, r: u32): u32
rotr64(x: u64, r: u32): u64
splitmix64Next(x_in: u64): u64
```

## Pitfalls
- All `next*` / `fillBytes` / `advance` methods mutate state: declare `let mut rng = Pcg32.seedFromU64(...)`.
- Not cryptographically secure (CSPRNG); use `cap:crypto` for security tokens or keys.
- `nextBoundedU32(0)` should be avoided; ensure `bound > 0`.
- `Pcg64Mcg.new` enforces an odd 128-bit state internally (`state_lo | 1`).
