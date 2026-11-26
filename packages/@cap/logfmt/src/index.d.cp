// Type definitions for CP

export class LogfmtField {
    key_start: usize,
    key_len: usize,
    val_start: usize,
    val_len: usize,
}

export fn isSpace(b: u8): bool;
export fn bytesEq(a: &[]u8, b: &[]u8): bool;
export class LogfmtRecord {
    key_buf: [512]u8,
    key_used: usize,
    val_buf: [1024]u8,
    val_used: usize,
    fields: [64]LogfmtField,
    count: usize,
    fn parse(line: &[]u8): LogfmtRecord;
    fn len(): usize;
    fn has(key: &[]u8): bool;
    fn get(_: auto, key: &[]u8): &[]u8;
}

export fn needsQuotes(val: &[]u8): bool;
export class LogfmtWriter {
    buf: []u8,
    pos: usize,
    count: usize,
    fn init(buf: []u8): LogfmtWriter;
    fn writeStr(_: auto, key: &[]u8, val: &[]u8): bool;
    fn writeInt(_: auto, key: &[]u8, val: i64): bool;
    fn writeBool(_: auto, key: &[]u8, val: bool): bool;
    fn written(): &[]u8;
    fn len(): usize;
}

