# rand_xoshiro@1.0.0 (Agent Quickref)

> Zero-allocation Blackman & Vigna xoshiro256** PRNG with SplitMix64 seeding, unbiased nextRange, 2^128 jump, and Fisher-Yates shuffle in pure CP | License: MIT
> Import: `import { ... } from "rand_xoshiro";` | Full types: `src/index.d.cp`

## Functions
- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn rotl7(x: u64): u64;`
- `export fn rotl45(x: u64): u64;`
- `export fn splitmix64Next(state: &mut u64): u64;`
- `export fn seedFromU64(seed: u64): Xoshiro256;`
- `export fn nextU64(rng: &mut Xoshiro256): u64;`
- `export fn nextF64(rng: &mut Xoshiro256): f64;`
- `export fn nextRange(rng: &mut Xoshiro256, upper_exclusive: u64): u64;`
- `export fn jump(rng: &mut Xoshiro256);`
- `export fn shuffleU32(rng: &mut Xoshiro256, data: []u32);`
