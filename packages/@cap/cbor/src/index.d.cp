// Type definitions for CP

export error CborError { BufferTooSmall, UnexpectedEof, TypeMismatch, InvalidHeader, Overflow, UnsupportedIndefinite }

export enum CborMajor {
    Uint = 0,
    NegInt = 1,
    Bytes = 2,
    Text = 3,
    Array = 4,
    Map = 5,
    Tag = 6,
    Simple = 7
}

export enum CborValueKind {
    Uint,
    NegInt,
    Bytes,
    Text,
    Array,
    Map,
    Tag,
    Bool,
    Null,
    Undefined,
    Float16,
    Float32,
    Float64,
    Simple
}

export fn writeU16Be(buf: []u8, offset: usize, v: u16);
export fn writeU32Be(buf: []u8, offset: usize, v: u32);
export fn writeU64Be(buf: []u8, offset: usize, v: u64);
export fn readU16Be(buf: &[]u8, offset: usize): u16;
export fn readU32Be(buf: &[]u8, offset: usize): u32;
export fn readU64Be(buf: &[]u8, offset: usize): u64;
export fn copyBytes(dst: []u8, dst_off: usize, src: &[]u8);
export fn headerSizeForArg(val: u64): usize;
export fn majorFromCode(code: u8): CborMajor;
export class CborEncoder {
    buf: []u8,
    pos: usize,
    fn init(buf: []u8): CborEncoder;
    fn written(): &[]u8;
    fn remaining(): usize;
    fn reset(_: auto);
    fn ensureSpace(needed: usize): void ! CborError;
    fn writeHeader(_: auto, major: u8, val: u64): void ! CborError;
    fn writeUint(_: auto, val: u64): void ! CborError;
    fn writeInt(_: auto, val: i64): void ! CborError;
    fn writeBytes(_: auto, data: &[]u8): void ! CborError;
    fn writeText(_: auto, text: &[]u8): void ! CborError;
    fn writeArrayHeader(_: auto, count: u64): void ! CborError;
    fn writeMapHeader(_: auto, count: u64): void ! CborError;
    fn writeTag(_: auto, tag: u64): void ! CborError;
    fn writeBool(_: auto, val: bool): void ! CborError;
    fn writeNull(_: auto): void ! CborError;
    fn writeUndefined(_: auto): void ! CborError;
    fn writeF32(_: auto, val: f32): void ! CborError;
    fn writeF64(_: auto, val: f64): void ! CborError;
}

export class CborDecoder {
    data: &[]u8,
    pos: usize,
    fn init(data: &[]u8): CborDecoder;
    fn remaining(): usize;
    fn ensureAvail(needed: usize): void ! CborError;
    fn peekInitialByte(): u8 ! CborError;
    fn validateHeaderByte(ib: u8): void ! CborError;
    fn peekMajor(): CborMajor ! CborError;
    fn peekKind(): CborValueKind ! CborError;
    fn readArg(_: auto, expected_major: u8): u64 ! CborError;
    fn readUint(_: auto): u64 ! CborError;
    fn readInt(_: auto): i64 ! CborError;
    fn readBytes(_: auto): &[]u8 ! CborError;
    fn readText(_: auto): &[]u8 ! CborError;
    fn readArrayHeader(_: auto): u64 ! CborError;
    fn readMapHeader(_: auto): u64 ! CborError;
    fn readTag(_: auto): u64 ! CborError;
    fn readBool(_: auto): bool ! CborError;
    fn readNull(_: auto): void ! CborError;
    fn readUndefined(_: auto): void ! CborError;
    fn readF32(_: auto): f32 ! CborError;
    fn readF64(_: auto): f64 ! CborError;
    fn skipValueDepth(_: auto, depth: usize): void ! CborError;
    fn skipValue(_: auto): void ! CborError;
}

