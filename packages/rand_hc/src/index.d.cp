// Type definitions for CP

export fn rhcRotr32(x: u32, n: usize): u32;
export fn rhcRotl32(x: u32, n: usize): u32;
export fn rhcF1(x: u32): u32;
export fn rhcF2(x: u32): u32;
export fn rhcG1(x: u32, y: u32, z: u32): u32;
export fn rhcG2(x: u32, y: u32, z: u32): u32;
export fn rhcReadU32Le(seed: [32]u8, offset: usize): u32;
export class Hc128Rng {
    p: [512]u32,
    q: [512]u32,
    counter: usize,
    fn h1(x: u32): u32;
    fn h2(x: u32): u32;
    fn fromSeed(seed: [32]u8): Hc128Rng;
    fn nextU32(_: auto): u32;
    fn nextU64(_: auto): u64;
    fn fillBytes(_: auto, out: []u8);
}

