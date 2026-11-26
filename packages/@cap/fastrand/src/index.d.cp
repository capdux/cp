// Type definitions for CP

export fn addWrap64(a: u64, b: u64): u64;
export fn mulWide64Xor(a: u64, b: u64): u64;
export fn i64ToOrderedU64(v: i64): u64;
export fn orderedU64ToI64(u: u64): i64;
export class Rng {
    state: u64,
    fn withSeed(seed: u64): Rng;
    fn nextU64(_: auto): u64;
    fn nextU32(_: auto): u32;
    fn nextBool(_: auto): bool;
    fn fork(_: auto): Rng;
    fn u64Below(_: auto, bound: u64): u64;
    fn nextChance(_: auto, numerator: u32, denominator: u32): bool;
    fn u64Range(_: auto, lo: u64, hi_exclusive: u64): u64;
    fn i64Range(_: auto, lo: i64, hi_inclusive: i64): i64;
    fn usizeRange(_: auto, lo: usize, hi_exclusive: usize): usize;
    fn f64Unit(_: auto): f64;
    fn fillBytes(_: auto, buf: []u8);
    fn shuffleU64(_: auto, slice: []u64);
    fn alphanumeric(_: auto, out: []u8): string;
}

