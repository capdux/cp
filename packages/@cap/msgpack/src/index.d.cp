// Type definitions for CP

export error MsgPackError { BufferTooSmall, UnexpectedEof, TypeMismatch, InvalidMarker, Overflow }

export enum MsgPackType {
    Nil,
    Bool,
    Int,
    Uint,
    Float32,
    Float64,
    Str,
    Bin,
    Array,
    Map,
    Ext
}

export class MsgPackExt {
    type_code: i8,
    data: &[]u8,
}

export fn writeU16Be(buf: []u8, offset: usize, v: u16);
export fn writeU32Be(buf: []u8, offset: usize, v: u32);
export fn writeU64Be(buf: []u8, offset: usize, v: u64);
export fn readU16Be(buf: &[]u8, offset: usize): u16;
export fn readU32Be(buf: &[]u8, offset: usize): u32;
export fn readU64Be(buf: &[]u8, offset: usize): u64;
export fn copyBytes(dst: []u8, dst_off: usize, src: &[]u8);
export class MsgPackEncoder {
    buf: []u8,
    pos: usize,
    fn init(buf: []u8): MsgPackEncoder;
    fn written(): &[]u8;
    fn remaining(): usize;
    fn reset(_: auto);
    fn ensureSpace(needed: usize): void ! MsgPackError;
    fn writeNil(_: auto): void ! MsgPackError;
    fn writeBool(_: auto, val: bool): void ! MsgPackError;
    fn writeUint(_: auto, val: u64): void ! MsgPackError;
    fn writeInt(_: auto, val: i64): void ! MsgPackError;
    fn writeF32(_: auto, val: f32): void ! MsgPackError;
    fn writeF64(_: auto, val: f64): void ! MsgPackError;
    fn writeStr(_: auto, s: &[]u8): void ! MsgPackError;
    fn writeBin(_: auto, data: &[]u8): void ! MsgPackError;
    fn writeArrayHeader(_: auto, count: u32): void ! MsgPackError;
    fn writeMapHeader(_: auto, count: u32): void ! MsgPackError;
    fn writeExt(_: auto, type_code: i8, data: &[]u8): void ! MsgPackError;
}

export class MsgPackDecoder {
    data: &[]u8,
    pos: usize,
    fn init(data: &[]u8): MsgPackDecoder;
    fn remaining(): usize;
    fn peekMarker(): u8 ! MsgPackError;
    fn peekType(): MsgPackType ! MsgPackError;
    fn ensureAvail(needed: usize): void ! MsgPackError;
    fn readNil(_: auto): void ! MsgPackError;
    fn readBool(_: auto): bool ! MsgPackError;
    fn readUint(_: auto): u64 ! MsgPackError;
    fn readInt(_: auto): i64 ! MsgPackError;
    fn readF32(_: auto): f32 ! MsgPackError;
    fn readF64(_: auto): f64 ! MsgPackError;
    fn readStr(_: auto): &[]u8 ! MsgPackError;
    fn readBin(_: auto): &[]u8 ! MsgPackError;
    fn readArrayHeader(_: auto): u32 ! MsgPackError;
    fn readMapHeader(_: auto): u32 ! MsgPackError;
    fn readExt(_: auto): MsgPackExt ! MsgPackError;
    fn skipValueDepth(_: auto, depth: usize): void ! MsgPackError;
    fn skipValue(_: auto): void ! MsgPackError;
}

