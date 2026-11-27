// Type definitions for CP

export fn prRotl64(x: u64, k: u6): u64;
export fn prSplitMix64Step(state: u64): u64;
export class Xoroshiro128Plus {
    s0: u64,
    s1: u64,
    fn fromSeed(seed: u64): Xoroshiro128Plus;
    fn fromState(s0: u64, s1: u64): Xoroshiro128Plus;
    fn nextU64(): NextU64Result;
    fn nextI32(): NextI32Result;
    fn jump(): Xoroshiro128Plus;
}

export class NextU64Result {
    value: u64,
    next: Xoroshiro128Plus,
}

export class NextI32Result {
    value: i32,
    next: Xoroshiro128Plus,
}

export class NextIntRangeResult {
    value: i64,
    next: Xoroshiro128Plus,
}

export fn uniformIntDistribution(min: i64, max: i64, rng: Xoroshiro128Plus): NextIntRangeResult;
