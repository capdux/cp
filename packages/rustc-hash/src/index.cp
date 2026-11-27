// @cap/rustc-hash — Zero-heap pure-CP fast non-cryptographic FxHash (64-bit & 32-bit)
// Clean-room MIT implementation of Rust compiler's FxHasher.

import { readU16LE, readU32LE, readU64LE } from "cap:bin";

pub const FX_SEED_64: u64 = ((0x517cc1b7 as u64) << 32) | (0x27220a95 as u64);
pub const FX_SEED_32: u32 = 2654435769; // 0x9e3779b9

fn fxhMul64(a: u64, b: u64): u64 {
    let al: u64 = a & 0xFFFFFFFF;
    let ah: u64 = a >> 32;
    let bl: u64 = b & 0xFFFFFFFF;
    let bh: u64 = b >> 32;
    let p0: u64 = al * bl;
    let p1: u64 = (al * bh) & 0xFFFFFFFF;
    let p2: u64 = (ah * bl) & 0xFFFFFFFF;
    let hi: u64 = ((p0 >> 32) + p1 + p2) & 0xFFFFFFFF;
    return (hi << 32) | (p0 & 0xFFFFFFFF);
}

fn fxhMul32(a: u32, b: u32): u32 {
    return (((a as u64) * (b as u64)) & 0xFFFFFFFF) as u32;
}

fn fxhRotl64By5(x: u64): u64 {
    return (x << 5) | (x >> 59);
}

fn fxhRotl32By5(x: u32): u32 {
    return (x << 5) | (x >> 27);
}

fn fxhAddWord64(hash: u64, word: u64): u64 {
    return fxhMul64(fxhRotl64By5(hash) ^ word, FX_SEED_64);
}

fn fxhAddWord32(hash: u32, word: u32): u32 {
    return fxhMul32(fxhRotl32By5(hash) ^ word, FX_SEED_32);
}

fn fxhHashSlice64(hash: u64, data: []const u8): u64 {
    let h: u64 = hash;
    let off: usize = 0;
    let len: usize = data.len;
    while (off + 8 <= len) {
        h = fxhAddWord64(h, readU64LE(data, off) ?? 0);
        off = off + 8;
    }
    if (off + 4 <= len) {
        h = fxhAddWord64(h, (readU32LE(data, off) ?? 0) as u64);
        off = off + 4;
    }
    if (off + 2 <= len) {
        h = fxhAddWord64(h, (readU16LE(data, off) ?? 0) as u64);
        off = off + 2;
    }
    if (off < len) {
        h = fxhAddWord64(h, data[off] as u64);
    }
    return h;
}

fn fxhHashSlice32(hash: u32, data: []const u8): u32 {
    let h: u32 = hash;
    let off: usize = 0;
    let len: usize = data.len;
    while (off + 4 <= len) {
        h = fxhAddWord32(h, readU32LE(data, off) ?? 0);
        off = off + 4;
    }
    if (off + 2 <= len) {
        h = fxhAddWord32(h, (readU16LE(data, off) ?? 0) as u32);
        off = off + 2;
    }
    if (off < len) {
        h = fxhAddWord32(h, data[off] as u32);
    }
    return h;
}

pub struct FxHasher64 {
    pub hash: u64,

    pub static fn new(): FxHasher64 {
        return FxHasher64 { hash: 0 };
    }

    pub static fn withSeed(seed: u64): FxHasher64 {
        return FxHasher64 { hash: seed };
    }

    pub fn writeUsize(*_, v: usize) {
        _.hash = fxhAddWord64(_.hash, v as u64);
    }

    pub fn writeU64(*_, v: u64) {
        _.hash = fxhAddWord64(_.hash, v);
    }

    pub fn writeU32(*_, v: u32) {
        _.hash = fxhAddWord64(_.hash, v as u64);
    }

    pub fn writeBytes(*_, data: []u8) {
        _.hash = fxhHashSlice64(_.hash, data);
    }

    pub fn writeStr(*_, s: string) {
        _.hash = fxhHashSlice64(_.hash, s);
    }

    pub fn finish(): u64 {
        return _.hash;
    }

    pub fn reset(*_) {
        _.hash = 0;
    }
}

pub struct FxHasher32 {
    pub hash: u32,

    pub static fn new(): FxHasher32 {
        return FxHasher32 { hash: 0 };
    }

    pub static fn withSeed(seed: u32): FxHasher32 {
        return FxHasher32 { hash: seed };
    }

    pub fn writeU32(*_, v: u32) {
        _.hash = fxhAddWord32(_.hash, v);
    }

    pub fn writeBytes(*_, data: []u8) {
        _.hash = fxhHashSlice32(_.hash, data);
    }

    pub fn writeStr(*_, s: string) {
        _.hash = fxhHashSlice32(_.hash, s);
    }

    pub fn finish(): u32 {
        return _.hash;
    }

    pub fn reset(*_) {
        _.hash = 0;
    }
}

pub fn fxHash64(data: []u8): u64 {
    let h = FxHasher64.new();
    h.writeBytes(data);
    return h.finish();
}

pub fn fxHash64Str(s: string): u64 {
    let h = FxHasher64.new();
    h.writeStr(s);
    return h.finish();
}

pub fn fxHash32(data: []u8): u32 {
    let h = FxHasher32.new();
    h.writeBytes(data);
    return h.finish();
}
