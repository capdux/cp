// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
import { trim, isDigit, findByte } from "cap:ascii";
import { parseU32, BufWriter } from "cap:fmt";
export class RRule {
    valid: bool,
    freq: u8,
    interval: u32,
    count: u32,
    has_count: bool,
    until: string,
    byday_mask: u8,
    bymonth_mask: u16,
    wkst: u8,
}

export fn rrlInvalid(): RRule;
export fn rrlParsePosU32(s: string): u32;
export fn rrlParseFreq(v: string): u8;
export fn rrlParseDayCode(code: string): u8;
export fn rrlParseByDayItem(item: string): u8;
export fn rrlParse2Digits(s: string, pos: usize): u32;
export fn rrlIsValidUntil(v: string): bool;
export fn parseRRule(input: string): RRule;
export fn rrlFreqName(freq: u8): string;
export fn rrlDayName(mask: u8): string;
export fn formatRRule(r: RRule, out: []u8): usize;
export fn matchesWeekday(r: RRule, dow: u8): bool;
export fn isValidRRule(input: string): bool;
