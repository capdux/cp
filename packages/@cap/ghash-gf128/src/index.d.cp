// Type definitions for CP

export fn bitToMask64(bit: u64): u64;
export fn readBeU64(buf: &[]u8, off: usize): u64;
export fn writeBeU64(val: u64, out: []u8, off: usize);
export class Block128 {
    hi: u64,
    lo: u64,
    fn zero(): Block128;
    fn one(): Block128;
    fn fromBytes(b: &[]u8): Block128;
    fn toBytes(out: []u8);
    fn xor(other: Block128): Block128;
    fn eq(other: Block128): bool;
}

export fn gf128Mul(x: Block128, y: Block128): Block128;
export class Ghash {
    h: Block128,
    y: Block128,
    fn new(hash_key_16: &[]u8): Ghash;
    fn reset(_: auto);
    fn updateBlock(_: auto, block_16: &[]u8);
    fn updatePadded(_: auto, data: &[]u8);
    fn updateLengths(_: auto, aad_bytes: u64, ct_bytes: u64);
    fn finalize(out_16: []u8);
}

