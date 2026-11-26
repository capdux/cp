// Type definitions for CP

export class ReadU8Result {
    ok: bool,
    val: u8,
}

export class ReadU16Result {
    ok: bool,
    val: u16,
}

export class ReadU32Result {
    ok: bool,
    val: u32,
}

export class ReadU64Result {
    ok: bool,
    val: u64,
}

export class BytesReader {
    data: &[]u8,
    pos: usize,
    fn new(data: &[]u8): BytesReader;
    fn remaining(): usize;
    fn hasRemaining(): bool;
    fn position(): usize;
    fn advance(_: auto, cnt: usize): bool;
    fn getU8(_: auto): ReadU8Result;
    fn getU16Be(_: auto): ReadU16Result;
    fn getU16Le(_: auto): ReadU16Result;
    fn getU32Be(_: auto): ReadU32Result;
    fn getU32Le(_: auto): ReadU32Result;
    fn getU64Be(_: auto): ReadU64Result;
    fn getU64Le(_: auto): ReadU64Result;
    fn copyToSlice(_: auto, dst: []u8): bool;
}

export class BytesMut {
    buf: [512]u8,
    used: usize,
    fn new(): BytesMut;
    fn len(): usize;
    fn capacity(): usize;
    fn remainingMut(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn truncate(_: auto, new_len: usize);
    fn byteAt(idx: usize): u8;
    fn putU8(_: auto, v: u8): bool;
    fn putU16Be(_: auto, v: u16): bool;
    fn putU16Le(_: auto, v: u16): bool;
    fn putU32Be(_: auto, v: u32): bool;
    fn putU32Le(_: auto, v: u32): bool;
    fn putU64Be(_: auto, v: u64): bool;
    fn putU64Le(_: auto, v: u64): bool;
    fn putSlice(_: auto, src: &[]u8): bool;
    fn splitTo(_: auto, at: usize, out: []u8): i64;
    fn copyAll(out: []u8): usize;
}

