// Type definitions for CP

export fn i32ToBits(v: i32): u32;
export fn bitsToI32(u: u32): i32;
export fn i64ToBits(v: i64): u64;
export fn bitsToI64(u: u64): i64;
export class BytesSpan {
    start: usize,
    len: usize,
    ok: bool,
}

export class BorshWriter {
    pos: usize,
    fn new(): BorshWriter;
    fn writeU8(_: auto, out: []u8, v: u8): bool;
    fn writeBool(_: auto, out: []u8, v: bool): bool;
    fn writeOptionTag(_: auto, out: []u8, is_some: bool): bool;
    fn writeU16Le(_: auto, out: []u8, v: u16): bool;
    fn writeU32Le(_: auto, out: []u8, v: u32): bool;
    fn writeU64Le(_: auto, out: []u8, v: u64): bool;
    fn writeI32Le(_: auto, out: []u8, v: i32): bool;
    fn writeI64Le(_: auto, out: []u8, v: i64): bool;
    fn writeBytes(_: auto, out: []u8, data: &[]u8): bool;
    fn writeString(_: auto, out: []u8, s: string): bool;
}

export class BorshReader {
    pos: usize,
    ok: bool,
    fn new(): BorshReader;
    fn remaining(buf: &[]u8): usize;
    fn readU8(_: auto, buf: &[]u8): u8;
    fn readBool(_: auto, buf: &[]u8): bool;
    fn readOptionTag(_: auto, buf: &[]u8): bool;
    fn readU16Le(_: auto, buf: &[]u8): u16;
    fn readU32Le(_: auto, buf: &[]u8): u32;
    fn readU64Le(_: auto, buf: &[]u8): u64;
    fn readI32Le(_: auto, buf: &[]u8): i32;
    fn readI64Le(_: auto, buf: &[]u8): i64;
    fn readBytesSpan(_: auto, buf: &[]u8): BytesSpan;
}

