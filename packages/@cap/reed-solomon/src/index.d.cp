// Type definitions for CP

export fn gfMul(a: u8, b: u8): u8;
export fn gfPow(a: u8, n: u32): u8;
export fn gfInv(a: u8): u8;
export class RsCodec {
    data_count: usize,
    parity_count: usize,
    parity_matrix: [128]u8,
    fn init(data_shards: usize, parity_shards: usize): RsCodec;
    fn dataShards(): usize;
    fn parityShards(): usize;
    fn totalShards(): usize;
    fn encodeByte(data: &[]u8, parity_out: []u8): bool;
    fn verifyByte(data: &[]u8, parity: &[]u8): bool;
    fn reconstructByte(shards: []u8, present: &[]bool): bool;
}

