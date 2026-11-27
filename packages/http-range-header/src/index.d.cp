// Type definitions for CP

import { eqIgnoreCase, findByte } from "cap:ascii";
import { parseU64, writeStr, writeU64 } from "cap:fmt";
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
export fn hrhParseU64(s: string, start: usize, end: usize): u64?;
export fn parseRangeHeader(header: string, out: []SyntacticByteRange): usize;
export fn validateRanges(ranges: &[]SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64;
export fn formatContentRange(start: u64, end: u64, total: u64, out: []u8): usize;
export fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize;
