// Type definitions for CP

export fn pcgMult(): u64;
export fn defaultStream(): u64;
export fn add64(a: u64, b: u64): u64;
export fn mul64(a: u64, b: u64): u64;
export fn rotr32(x: u32, rot: u32): u32;
export class Pcg32 {
    state: u64,
    inc: u64,
    fn new(seed: u64, stream: u64): Pcg32;
    fn fromSeed(seed: u64): Pcg32;
    fn nextU32(_: auto): u32;
    fn nextU64(_: auto): u64;
    fn nextF64(_: auto): f64;
    fn nextBounded(_: auto, bound: u32): u32;
    fn advance(_: auto, delta: u64);
    fn fillBytes(_: auto, out: []u8);
}

