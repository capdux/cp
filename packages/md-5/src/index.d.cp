// Type definitions for CP

export fn md5Add32(a: u32, b: u32): u32;
export fn md5Add4(a: u32, b: u32, c: u32, d: u32): u32;
export fn md5Rotl32(x: u32, bits: u32): u32;
export fn md5ShiftAt(idx: usize): u32;
export fn md5ConstAt(idx: usize): u32;
export fn md5ReadLe32(b: []u8, off: usize): u32;
export fn md5WriteLe32(out: []u8, off: usize, v: u32);
export fn md5HexDigit(nibble: u8): u8;
export class Md5Hasher {
    a0: u32,
    b0: u32,
    c0: u32,
    d0: u32,
    buf: [64]u8,
    buf_len: usize,
    total_bytes: u64,
    fn new(): Md5Hasher;
    fn reset(_: auto);
    fn compressBuf(_: auto);
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out16: []u8);
    fn finalizeHex(_: auto, out32: []u8): usize;
}

export fn md5Bytes(data: []u8, out16: []u8);
export fn md5Hex(s: string, out32: []u8): usize;
