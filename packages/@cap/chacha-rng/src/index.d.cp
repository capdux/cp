// Type definitions for CP

export fn crAdd32(a: u32, b: u32): u32;
export fn crRotl32(x: u32, bits: u32): u32;
export fn crReadLe32(data: &[]u8, offset: usize): u32;
export fn crWriteLe32(out: []u8, offset: usize, v: u32);
export fn crQuarterRound(st: []u32, a: usize, b: usize, c: usize, d: usize);
export fn chachaBlock(key: [8]u32, counter: u32, nonce: [3]u32, rounds: u32, out: []u8);
export class ChaCha20Rng {
    key: [8]u32,
    nonce: [3]u32,
    counter: u32,
    buf: [64]u8,
    pos: usize,
    fn fromSeed(seed: [32]u8): ChaCha20Rng;
    fn fromU64(seed: u64): ChaCha20Rng;
    fn setWordPos(_: auto, block_counter: u32);
    fn fillBytes(_: auto, out: []u8);
    fn nextU32(_: auto): u32;
    fn nextU64(_: auto): u64;
    fn nextF64(_: auto): f64;
}

