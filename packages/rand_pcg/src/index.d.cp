// Type definitions for CP

export fn wrapAdd64(a: u64, b: u64): u64;
export fn wrapMul64(a: u64, b: u64): u64;
export class U128Pair {
    hi: u64,
    lo: u64,
}

export fn mul64Full(a: u64, b: u64): U128Pair;
export fn wrapMul128(a_hi: u64, a_lo: u64, b_hi: u64, b_lo: u64): U128Pair;
export fn rotr32(x: u32, r: u32): u32;
export fn rotr64(x: u64, r: u32): u64;
export fn splitmix64Next(x_in: u64): u64;
export class Pcg32 {
    state: u64,
    inc: u64,
    fn init(seed: u64, stream_id: u64): Pcg32;
    fn new(state: u64, stream: u64): Pcg32;
    fn seedFromU64(seed: u64): Pcg32;
    fn nextU32(_: auto): u32;
    fn nextU64(_: auto): u64;
    fn nextBoundedU32(_: auto, bound: u32): u32;
    fn nextF32(_: auto): f32;
    fn nextF64(_: auto): f64;
    fn nextBool(_: auto): bool;
    fn fillBytes(_: auto, dst: []u8);
    fn advance(_: auto, delta_in: u64);
}

export class Pcg64Mcg {
    state_hi: u64,
    state_lo: u64,
    fn new(state_hi: u64, state_lo: u64): Pcg64Mcg;
    fn seedFromU64(seed: u64): Pcg64Mcg;
    fn nextU64(_: auto): u64;
    fn nextU32(_: auto): u32;
    fn nextF64(_: auto): f64;
    fn fillBytes(_: auto, dst: []u8);
    fn advance(_: auto, delta_in: u64);
}

