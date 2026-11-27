// Type definitions for CP

export class BencodeIntResult {
    value: i64,
    next_pos: usize,
    ok: bool,
}

export class BencodeBytesResult {
    start: usize,
    len: usize,
    next_pos: usize,
    ok: bool,
}

export class BencodeWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn init(buf: []u8): BencodeWriter;
    fn writeByte(_: auto, b: u8): bool;
    fn writeU64Decimal(_: auto, val: u64): bool;
    fn writeInt(_: auto, v: i64): bool;
    fn writeBytes(_: auto, data: &[]u8): bool;
    fn beginList(_: auto): bool;
    fn beginDict(_: auto): bool;
    fn end(_: auto): bool;
    fn written(): &[]u8;
    fn len(): usize;
}

export fn peekKind(src: &[]u8, pos: usize): u8;
export fn decodeInt(src: &[]u8, pos: usize): BencodeIntResult;
export fn decodeBytes(src: &[]u8, pos: usize): BencodeBytesResult;
export fn compareBytes(a: &[]u8, b: &[]u8): i32;
export fn parseValueEnd(src: &[]u8, pos: usize, depth: usize): i64;
export fn validate(src: &[]u8): bool;
export fn dictLookup(src: &[]u8, key: &[]u8): BencodeBytesResult;
