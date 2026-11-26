// Type definitions for CP

export fn rmdAdd(a: u32, b: u32): u32;
export fn rmdAdd3(a: u32, b: u32, c: u32): u32;
export fn rmdAdd4(a: u32, b: u32, c: u32, d: u32): u32;
export fn rmdRol(x: u32, n: u32): u32;
export fn rmdNot(x: u32): u32;
export fn rmdFunc(round: usize, x: u32, y: u32, z: u32): u32;
export fn rmdKl(round: usize): u32;
export fn rmdKr(round: usize): u32;
export fn rmdReadU32Le(buf: []u8, off: usize): u32;
export fn rmdWriteU32Le(v: u32, out: []u8, off: usize);
export fn rmdHexNibble(v: u8): u8;
export fn rmdCompress(state: [5]u32, block: []u8): [5]u32;
export class Ripemd160 {
    state: [5]u32,
    buf: [64]u8,
    buf_len: usize,
    total_len: u64,
    fn new(): Ripemd160;
    fn reset(_: auto);
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out20: []u8);
    fn finalizeHex(_: auto, out40: []u8): usize;
}

export fn ripemd160Bytes(data: []u8, out20: []u8);
export fn ripemd160Hex(s: string, out40: []u8): usize;
