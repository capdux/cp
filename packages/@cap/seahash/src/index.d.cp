// Type definitions for CP

export fn seaU64(hi: u32, lo: u32): u64;
export fn seaMul64(a: u64, b: u64): u64;
export fn seaReadU64Le(buf: []u8, off: usize): u64;
export fn seahashDiffuse(x_in: u64): u64;
export class SeaHasher {
    a: u64,
    b: u64,
    c: u64,
    d: u64,
    buf: [8]u8,
    buf_len: usize,
    written: u64,
    fn new(): SeaHasher;
    fn withSeeds(k1: u64, k2: u64, k3: u64, k4: u64): SeaHasher;
    fn writeU64(_: auto, word: u64);
    fn writeBytes(_: auto, buf: []u8);
    fn writeStr(_: auto, s: string);
    fn finish(): u64;
}

export fn seahashBytes(buf: []u8): u64;
export fn seahashSeeded(buf: []u8, k1: u64, k2: u64, k3: u64, k4: u64): u64;
