// Type definitions for CP

export fn bbWriteBe64(out: []u8, offset: usize, val: u64);
export fn bbWriteLe64(out: []u8, offset: usize, val: u64);
export class BlockBuffer64 {
    buf: [64]u8,
    pos: usize,
    total_bytes: u64,
    fn new(): BlockBuffer64;
    fn remaining(): usize;
    fn getPos(): usize;
    fn reset(_: auto);
    fn feed(_: auto, data: &[]u8, out_blocks: []u8): usize;
    fn padMdBe64(_: auto, out_blocks: []u8): usize;
    fn padMdLe64(_: auto, out_blocks: []u8): usize;
    fn padPkcs7(_: auto, out_block: []u8): bool;
    fn padIso7816(_: auto, out_block: []u8): bool;
    fn padZero(_: auto, out_block: []u8): bool;
}

export class BlockBuffer128 {
    buf: [128]u8,
    pos: usize,
    total_bytes: u64,
    fn new(): BlockBuffer128;
    fn remaining(): usize;
    fn getPos(): usize;
    fn reset(_: auto);
    fn feed(_: auto, data: &[]u8, out_blocks: []u8): usize;
    fn padMdBe64(_: auto, out_blocks: []u8): usize;
    fn padMdLe64(_: auto, out_blocks: []u8): usize;
    fn padPkcs7(_: auto, out_block: []u8): bool;
    fn padIso7816(_: auto, out_block: []u8): bool;
    fn padZero(_: auto, out_block: []u8): bool;
}

