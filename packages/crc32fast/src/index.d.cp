// Type definitions for CP

export fn crcByteStep(crc_in: u32, b: u8): u32;
export fn crcBuildTables(): [4][256]u32;
export fn crcGf2MatrixTimes(mat: [32]u32, vec: u32): u32;
export fn crcGf2MatrixSquare(src: [32]u32): [32]u32;
export fn combineCrc32(crc1: u32, crc2: u32, len2: u64): u32;
export class Crc32Hasher {
    state: u32,
    amount: u64,
    fn new(): Crc32Hasher;
    fn newWithInitial(init_crc: u32): Crc32Hasher;
    fn update(_: auto, buf: &[]u8);
    fn updateStr(_: auto, s: string);
    fn finalize(): u32;
    fn reset(_: auto);
    fn combine(_: auto, other: Crc32Hasher);
}

export fn hashBytes(buf: &[]u8): u32;
export fn hashStr(s: string): u32;
