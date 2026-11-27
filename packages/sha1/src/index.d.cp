// Type definitions for CP

export fn sha1Add32(a: u32, b: u32): u32;
export fn sha1Add5(a: u32, b: u32, c: u32, d: u32, e: u32): u32;
export fn sha1Rotl1(x: u32): u32;
export fn sha1Rotl5(x: u32): u32;
export fn sha1Rotl30(x: u32): u32;
export fn sha1ReadBe32(b: []u8, off: usize): u32;
export fn sha1WriteBe32(out: []u8, off: usize, v: u32);
export fn sha1HexDigit(nibble: u8): u8;
export class Sha1Hasher {
    h0: u32,
    h1: u32,
    h2: u32,
    h3: u32,
    h4: u32,
    buf: [64]u8,
    buf_len: usize,
    total_bytes: u64,
    fn new(): Sha1Hasher;
    fn reset(_: auto);
    fn compressBuf(_: auto);
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out20: []u8);
    fn finalizeHex(_: auto, out40: []u8): usize;
}

export fn sha1Bytes(data: []u8, out20: []u8);
export fn sha1Hex(s: string, out40: []u8): usize;
