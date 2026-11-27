// Type definitions for CP

export enum RangePosition {
    StartAndEnd,
    StartOnly,
    Suffix
}

export class SyntacticByteRange {
    kind: RangePosition,
    start: u64,
    end: u64,
}

export class ValidatedByteRange {
    start: u64,
    end: u64,
    fn byteLen(): u64;
}

export fn hrhIsSpace(b: u8): bool;
export fn hrhIsDigit(b: u8): bool;
export fn hrhToLower(b: u8): u8;
export fn hrhHasBytesPrefix(s: string, start: usize, end: usize): bool;
export fn hrhParseU64(s: string, start: usize, end: usize): u64?;
export fn hrhWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn hrhWriteSlice(out: []u8, pos: usize, s: string): usize;
export fn hrhWriteU64(out: []u8, pos: usize, val: u64): usize;
export fn parseRangeHeader(header: string, out: []SyntacticByteRange): usize;
export fn validateRanges(ranges: &[]SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64;
export fn formatContentRange(start: u64, end: u64, total: u64, out: []u8): usize;
export fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize;
