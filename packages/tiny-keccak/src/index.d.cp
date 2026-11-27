// Type definitions for CP

export fn kckU64(hi: u32, lo: u32): u64;
export fn kckRotl64(x: u64, n: u32): u64;
export fn kckReadLe64(b: []u8, off: usize): u64;
export fn kckWriteLe64(out: []u8, off: usize, v: u64);
export fn kckHexDigit(nibble: u8): u8;
export fn kckRoundConst(round: usize): u64;
export fn kckPermute(st: []u64);
export fn kckAbsorbBlock(st: []u64, block: []u8);
export fn kckFinalizeSponge(st: []u64, buf: []u8, buf_len: usize, domain_sep: u8, out32: []u8);
export fn kckEncodeHex32(digest: []u8, out64: []u8): usize;
export class Sha3_256 {
    st: [25]u64,
    buf: [136]u8,
    buf_len: usize,
    fn new(): Sha3_256;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out32: []u8);
    fn finalizeHex(_: auto, out64: []u8): usize;
}

export class Keccak256 {
    st: [25]u64,
    buf: [136]u8,
    buf_len: usize,
    fn new(): Keccak256;
    fn update(_: auto, data: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(_: auto, out32: []u8);
    fn finalizeHex(_: auto, out64: []u8): usize;
}

export fn sha3_256Hex(s: string, out64: []u8): usize;
export fn keccak256Hex(s: string, out64: []u8): usize;
