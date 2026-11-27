// Type definitions for CP

export enum CborHeaderKind {
    Positive,
    Negative,
    Bytes,
    Text,
    Array,
    Map,
    Tag,
    Bool,
    Null,
    Undefined,
    Float,
    Break,
    Invalid
}

export class CborHeader {
    kind: CborHeaderKind,
    u64_val: u64,
    i64_val: i64,
    f64_val: f64,
    bool_val: bool,
    len: i64,
    fn invalid(): CborHeader;
}

export fn cblMaxI64AsU64(): u64;
export fn cblReadBe16(data: &[]u8, pos: usize): u16;
export fn cblReadBe32(data: &[]u8, pos: usize): u32;
export fn cblReadBe64(data: &[]u8, pos: usize): u64;
export fn cblDecodeHalf(bits: u16): f64;
export fn cblArgSize(val: u64): usize;
export fn cblWriteHead(buf: []u8, pos: usize, major: u8, val: u64): usize;
export class CborDecoder {
    data: &[]u8,
    pos: usize,
    fn new(data: &[]u8): CborDecoder;
    fn remaining(): usize;
    fn isEof(): bool;
    fn readBytes(_: auto, n: usize): (&[]u8)?;
    fn pull(_: auto): CborHeader;
}

export class CborEncoder {
    buf: []u8,
    pos: usize,
    fn new(buf: []u8): CborEncoder;
    fn written(): usize;
    fn remaining(): usize;
    fn slice(): &[]u8;
    fn pushPositive(_: auto, val: u64): bool;
    fn pushNegativeRaw(_: auto, raw: u64): bool;
    fn pushNegative(_: auto, val: i64): bool;
    fn pushBytesHeader(_: auto, len: i64): bool;
    fn pushBytes(_: auto, data: &[]u8): bool;
    fn pushTextHeader(_: auto, len: i64): bool;
    fn pushText(_: auto, text: string): bool;
    fn pushArray(_: auto, len: i64): bool;
    fn pushMap(_: auto, len: i64): bool;
    fn pushTag(_: auto, tag: u64): bool;
    fn pushBool(_: auto, val: bool): bool;
    fn pushNull(_: auto): bool;
    fn pushUndefined(_: auto): bool;
    fn pushFloat64(_: auto, val: f64): bool;
    fn pushBreak(_: auto): bool;
}

