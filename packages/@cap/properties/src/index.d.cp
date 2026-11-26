// Type definitions for CP

export class PropEntry {
    key_start: usize,
    key_len: usize,
    val_start: usize,
    val_len: usize,
}

export fn isLineWs(c: u8): bool;
export fn fromHex(c: u8): i32;
export fn bytesEq(a: &[]u8, b: &[]u8): bool;
export class EscDecodeResult {
    next_pos: usize,
    written: usize,
}

export fn decodeEscapeInto(src: &[]u8, esc_pos: usize, dst: []u8, dst_pos: usize): EscDecodeResult;
export class Properties {
    entries: [128]PropEntry,
    count: usize,
    val_buf: [2048]u8,
    buf_used: usize,
    fn parse(src: &[]u8): Properties;
    fn len(): usize;
    fn findIndex(key: &[]u8): i64;
    fn has(key: &[]u8): bool;
    fn get(key: &[]u8): &[]u8;
}

export fn pushByte(dst: []u8, pos: usize, b: u8): i64;
export fn pushEscaped(dst: []u8, pos: usize, ch: u8): i64;
export fn formatEntry(key: &[]u8, value: &[]u8, dst: []u8): i64;
