// Type definitions for CP

export fn makeU64(hi: u32, lo: u32): u64;
export fn addWrap64(a: u64, b: u64): u64;
export fn mulWrap64(a: u64, b: u64): u64;
export fn pow2_64(n: u32): u64;
export fn rotl64(x: u64, r_in: u32): u64;
export fn foldedMultiply(x: u64, y: u64): u64;
export fn multipleConst(): u64;
export fn pi0(): u64;
export fn pi1(): u64;
export fn pi2(): u64;
export fn pi3(): u64;
export fn readU64Le(data: &[]u8, offset: usize): u64;
export fn packTail(tail: [8]u8, tail_len: usize): u64;
export class AHasher {
    buffer: u64,
    pad: u64,
    extra_keys: [2]u64,
    tail: [8]u8,
    tail_len: usize,
    total_len: u64,
    fn init(k0: u64, k1: u64, k2: u64, k3: u64): AHasher;
    fn writeU8(_: auto, v: u8);
    fn writeU32(_: auto, v: u32);
    fn writeU64(_: auto, v: u64);
    fn writeBytes(_: auto, data: &[]u8);
    fn finish(): u64;
}

export class RandomState {
    k0: u64,
    k1: u64,
    k2: u64,
    k3: u64,
    fn withSeeds(k0: u64, k1: u64, k2: u64, k3: u64): RandomState;
    fn defaultState(): RandomState;
    fn buildHasher(): AHasher;
    fn hashOneBytes(data: &[]u8): u64;
    fn hashOneU64(v: u64): u64;
}

export fn hashBytes(data: &[]u8, k0: u64, k1: u64): u64;
export fn hashU64(v: u64, k0: u64, k1: u64): u64;
