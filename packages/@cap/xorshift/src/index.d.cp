// Type definitions for CP

export fn xsU64FromHalves(hi: u32, lo: u32): u64;
export fn xsAddWrap64(a: u64, b: u64): u64;
export fn xsMulWrap64(a: u64, b: u64): u64;
export fn xsSplitMixNext(st: &mut u64): u64;
export fn xsReadU32Le(b: [16]u8, off: usize): u32;
export class XorShift128 {
    x: u32,
    y: u32,
    z: u32,
    w: u32,
    fn fromSeed(seed: [16]u8): XorShift128;
    fn fromU64(seed: u64): XorShift128;
    fn nextU32(_: auto): u32;
    fn nextU64(_: auto): u64;
    fn nextF64(_: auto): f64;
    fn fillBytes(_: auto, out: []u8);
}

export class XorShift64Star {
    state: u64,
    fn new(seed: u64): XorShift64Star;
    fn nextU64(_: auto): u64;
    fn nextU32(_: auto): u32;
}

export class XorShift32 {
    state: u32,
    fn new(seed: u32): XorShift32;
    fn nextU32(_: auto): u32;
}

