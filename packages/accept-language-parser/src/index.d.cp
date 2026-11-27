// Type definitions for CP

import { isAlnum, isDigit, eqIgnoreCase, findByte } from "cap:ascii";
export class LangRange {
    tag: string,
    primary: string,
    region: string,
    quality: u16,
}

export fn isWs(c: u8): bool;
export fn isValidTag(tag: string): bool;
export fn parseQuality(s: string): i32;
export fn parseAcceptLanguage(header: string, out: []LangRange): usize;
export fn matchesTag(range_tag: string, supported_tag: string): bool;
export fn isExcluded(ranges: &[]LangRange, count: usize, candidate: string): bool;
export fn pickLanguage(header: string, supported: &[]string): string;
