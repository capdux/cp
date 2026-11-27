// Type definitions for CP

export fn makeU64(hi: u32, lo: u32): u64;
export fn addWrap64(a: u64, b: u64): u64;
export fn foldedMultiply(x: u64, y: u64): u64;
export fn byteShift64(idx: u8): u64;
export fn readU64Le(data: &[]u8, offset: usize): u64;
export fn initFoldSeed(seed: u64): u64;
export fn initAccum(seed: u64): u64;
export class FoldHasher {
    accum: u64,
    sponge: u64,
    sponge_len: u8,
    fold_seed: u64,
    quality: bool,
    total_len: u64,
    fn initFast(seed: u64): FoldHasher;
    fn initQuality(seed: u64): FoldHasher;
    fn writeU8(_: auto, v: u8);
    fn writeU32(_: auto, v: u32);
    fn writeU64(_: auto, v: u64);
    fn writeBytes(_: auto, data: &[]u8);
    fn finish(): u64;
}

export fn hashBytesFast(data: &[]u8, seed: u64): u64;
export fn hashBytesQuality(data: &[]u8, seed: u64): u64;
export fn hashU64(v: u64, seed: u64): u64;
