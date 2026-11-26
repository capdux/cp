// Type definitions for CP

export class ParsedDisposition {
    kind: u8,
    filename_len: usize,
    is_utf8: bool,
    ok: bool,
}

export fn isWs(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn isRfc5987AttrChar(c: u8): bool;
export fn hexUpper(nib: u8): u8;
export fn fromHexDigit(c: u8): i32;
export fn appendSlice(dst: []u8, pos: usize, src: &[]u8): i64;
export fn appendByte(dst: []u8, pos: usize, b: u8): i64;
export fn format(kind: u8, filename: &[]u8, dst: []u8): i64;
export fn decodePercent(src: &[]u8, dst: []u8): i64;
export fn decodeQuotedOrToken(val: &[]u8, dst: []u8): i64;
export fn parse(header: &[]u8, filename_out: []u8): ParsedDisposition;
