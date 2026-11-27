// Type definitions for CP

import { isSpace, isDigit, isAlpha, toLowerByte, strEq } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export fn pdurCheckedAddU64(a: u64, b: u64): u64?;
export fn pdurCheckedMulU64(a: u64, b: u64): u64?;
export fn pdurMatchUnit(u: string): u32;
export fn pdurAppendPart(out: []u8, pos: usize, val: u64, suffix: string): usize;
export class ParsedDuration {
    secs: u64,
    nanos: u32,
    fn isZero(): bool;
    fn totalSecs(): u64;
    fn totalMillis(): u64;
    fn totalNanos(): u64?;
}

export fn parseDuration(input: string): ParsedDuration?;
export fn formatDuration(d: ParsedDuration, out: []u8): usize;
