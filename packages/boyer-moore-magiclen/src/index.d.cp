// Type definitions for CP

import { toLowerByte } from "cap:ascii";
export class BmhPattern {
    shift: [256]usize,
    pat_len: usize,
}

export fn compilePattern(needle: &[]u8): BmhPattern;
export fn findWithCompiled(haystack: &[]u8, needle: &[]u8, compiled: BmhPattern): i64;
export fn findFirst(haystack: &[]u8, needle: &[]u8): i64;
export fn findLast(haystack: &[]u8, needle: &[]u8): i64;
export fn countNonOverlapping(haystack: &[]u8, needle: &[]u8): usize;
export fn findIgnoreCaseAscii(haystack: string, needle: string): i64;
