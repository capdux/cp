// Type definitions for CP

import { isSpace, isAscii, startsWith, endsWith, toLower, toUpper, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { isValidUtf8 } from "cap:utf8";
import { memchr, memrchr, memmemFind, memmemRfind } from "../../memchr/src/index.cp";
export class SliceRange {
    start: usize,
    end: usize,
    fn len(): usize;
    fn isEmpty(): bool;
}

export fn bsIsContByte(b: u8): bool;
export fn bsUtf8ValidLen(src: &[]u8, i: usize): usize;
export fn bsUtf8InvalidSkip(src: &[]u8, i: usize): usize;
export fn bstrFind(haystack: &[]u8, needle: &[]u8): i64;
export fn bstrRfind(haystack: &[]u8, needle: &[]u8): i64;
export fn bstrFindByte(haystack: &[]u8, b: u8): i64;
export fn bstrRfindByte(haystack: &[]u8, b: u8): i64;
export fn bstrContains(haystack: &[]u8, needle: &[]u8): bool;
export fn bstrStartsWith(s: &[]u8, prefix: &[]u8): bool;
export fn bstrEndsWith(s: &[]u8, suffix: &[]u8): bool;
export fn bstrTrimStart(s: &[]u8): SliceRange;
export fn bstrTrimEnd(s: &[]u8): SliceRange;
export fn bstrTrim(s: &[]u8): SliceRange;
export fn bstrLineCount(s: &[]u8): usize;
export fn bstrLineAt(s: &[]u8, idx: usize): SliceRange;
export fn bstrReplaceAll(src: &[]u8, pattern: &[]u8, replacement: &[]u8, out: []u8): usize;
export fn bstrIsUtf8(src: &[]u8): bool;
export fn bstrToUtf8Lossy(src: &[]u8, out: []u8): usize;
export fn bstrIsAscii(s: &[]u8): bool;
export fn bstrToLowerAscii(src: &[]u8, out: []u8): usize;
export fn bstrToUpperAscii(src: &[]u8, out: []u8): usize;
export fn bstrEqIgnoreAsciiCase(a: &[]u8, b: &[]u8): bool;
