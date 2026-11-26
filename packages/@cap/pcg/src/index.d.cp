// Type definitions for CP

export fn wrapAdd64(a: u64, b: u64): u64;
export fn wrapMul64(a: u64, b: u64): u64;
export fn rotr32(x: u32, r: u32): u32;
export class Pcg32 {
    state: u64,
    inc: u64,
    fn init(seed: u64, stream_id: u64): Pcg32;
    fn nextU32(_: auto): u32;
    fn nextU64(_: auto): u64;
    fn nextBoundedU32(_: auto, bound: u32): u32;
    fn nextF64(_: auto): f64;
    fn advance(_: auto, delta_in: u64);
}

