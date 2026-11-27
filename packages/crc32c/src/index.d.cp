// Type definitions for CP

export fn c32cByteStep(crc_in: u32, b: u8): u32;
export fn c32cGf2MatrixTimes(mat: [32]u32, vec: u32): u32;
export fn c32cGf2MatrixSquare(src: [32]u32): [32]u32;
export fn crc32cAppend(crc: u32, data: []u8): u32;
export fn crc32c(data: []u8): u32;
export fn crc32cStr(s: string): u32;
export fn crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32;
export fn maskCrc32c(crc: u32): u32;
export fn unmaskCrc32c(masked: u32): u32;
export class Crc32cHasher {
    state: u32,
    amount: u64,
    fn new(): Crc32cHasher;
    fn newWithInitial(init_crc: u32): Crc32cHasher;
    fn update(_: auto, buf: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(): u32;
    fn reset(_: auto);
    fn combine(_: auto, other: Crc32cHasher);
}

