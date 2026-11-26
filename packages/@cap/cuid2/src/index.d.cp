// Type definitions for CP

export fn makeU64(hi: u32, lo: u32): u64;
export fn addWrap64(a: u64, b: u64): u64;
export fn mulWrap64(a: u64, b: u64): u64;
export fn nextEntropy(state: &mut u64): u64;
export fn add32(a: u32, b: u32): u32;
export fn pow2_64(n: u32): u64;
export fn rotr32(x: u32, n: u32): u32;
export fn shr32(x: u32, n: u32): u32;
export fn bitNot32(x: u32): u32;
export fn sha256K(i: usize): u32;
export fn writeU64Be(buf: &mut [64]u8, off: usize, v: u64);
export fn sha256FourU64(v0: u64, v1: u64, v2: u64, v3: u64): [32]u8;
export fn divMod36(bytes: &mut [32]u8): u8;
export class Cuid2Generator {
    counter: u64,
    fingerprint: u64,
    rng_state: u64,
    id_len: usize,
    fn init(fingerprint: u64, seed: u64, id_len: usize): Cuid2Generator;
    fn next(_: auto, timestamp_ms: u64, dst: []u8): usize;
    fn counterValue(): u64;
}

export fn isCuid2(s: &[]u8): bool;
export fn isCuid2WithLen(s: &[]u8, expected_len: usize): bool;
