// Type definitions for CP

export class ByteRange {
    start: u64,
    end: u64,
}

export class ParseRangeResult {
    status: i32,
    count: usize,
    ranges: [16]ByteRange,
}

export class ParsedU64 {
    val: u64,
    ok: bool,
}

export fn emptyResult(status: i32): ParseRangeResult;
export fn isSpace(b: u8): bool;
export fn toLowerAscii(b: u8): u8;
export fn parseU64Slice(data: &[]u8, start: usize, end: usize): ParsedU64;
export fn parseRange(size: u64, header: &[]u8, combine: bool): ParseRangeResult;
export fn writeBytes(dst: []u8, pos: usize, src: &[]u8): i64;
export fn writeU64(dst: []u8, pos: usize, val: u64): i64;
export fn formatContentRange(start: u64, end: u64, total_size: u64, dst: []u8): i64;
export fn formatUnsatisfiableRange(total_size: u64, dst: []u8): i64;
