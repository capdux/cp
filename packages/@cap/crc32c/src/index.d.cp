// Type definitions for CP

export fn crc32cByte(crc_in: u32, byte: u8): u32;
export fn buildTable(): [256]u32;
export fn crc32c(data: &[]u8): u32;
export fn crc32cAppend(prev: u32, data: &[]u8): u32;
export fn mask(crc: u32): u32;
export fn unmask(masked: u32): u32;
export fn gf2MatrixTimes(mat: &mut [32]u32, vec_in: u32): u32;
export fn gf2MatrixSquare(square: &mut [32]u32, mat: &mut [32]u32);
export fn crc32cCombine(crc1_in: u32, crc2: u32, len2_in: u64): u32;
export class Crc32cHasher {
    state: u32,
    total_len: u64,
    table: [256]u32,
    fn init(): Crc32cHasher;
    fn update(_: auto, data: &[]u8);
    fn finalize(): u32;
    fn finalizeMasked(): u32;
    fn bytesHashed(): u64;
    fn reset(_: auto);
}

