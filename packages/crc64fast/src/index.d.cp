// Type definitions for CP

export fn makeU64(hi: u32, lo: u32): u64;
export class Crc64Hasher {
    table: [256]u64,
    crc: u64,
    init_val: u64,
    xorout: u64,
    reflected: bool,
    fn initCustom(poly: u64, init_val: u64, xorout: u64, reflected: bool): Crc64Hasher;
    fn initEcma(): Crc64Hasher;
    fn initXz(): Crc64Hasher;
    fn initIso(): Crc64Hasher;
    fn update(_: auto, data: &[]u8);
    fn finalize(): u64;
    fn reset(_: auto);
}

export fn crc64Ecma(data: &[]u8): u64;
export fn crc64Xz(data: &[]u8): u64;
export fn crc64Iso(data: &[]u8): u64;
