// Type definitions for CP

export fn buildCrcTable(poly: u32): [256]u32;
export class Crc32Hasher {
    state: u32,
    table: [256]u32,
    fn init(): Crc32Hasher;
    fn update(_: auto, data: string);
    fn finalize(): u32;
    fn reset(_: auto);
}

export class Crc32cHasher {
    state: u32,
    table: [256]u32,
    fn init(): Crc32cHasher;
    fn update(_: auto, data: string);
    fn finalize(): u32;
    fn reset(_: auto);
}

export fn crc32Ieee(data: string): u32;
export fn crc32c(data: string): u32;
export fn adler32(data: string): u32;
export fn gf2MatrixTimes(mat: &mut [32]u32, vec_in: u32): u32;
export fn gf2MatrixSquare(square: &mut [32]u32, mat: &mut [32]u32);
export fn combineCrc32Ieee(crc1: u32, crc2: u32, len2: usize): u32;
