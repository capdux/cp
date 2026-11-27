// Type definitions for CP

import { eqIgnoreCase, isDigit, isSpace } from "cap:ascii";
import { bytesEq } from "cap:bin";
export class RequestHeaders {
    if_none_match: &[]u8,
    if_modified_since: &[]u8,
    cache_control: &[]u8,
}

export class ResponseHeaders {
    etag: &[]u8,
    last_modified: &[]u8,
}

export class TrimRange {
    start: usize,
    end: usize,
}

export fn trimRange(s: &[]u8, start: usize, end: usize): TrimRange;
export fn hasNoCache(cache_control: &[]u8): bool;
export class ParsedEtag {
    start: usize,
    end: usize,
    weak: bool,
    valid: bool,
}

export fn parseEtagSlice(s: &[]u8, raw_start: usize, raw_end: usize): ParsedEtag;
export fn etagWeakMatch(a: &[]u8, b: &[]u8): bool;
export fn etagStrongMatch(a: &[]u8, b: &[]u8): bool;
export fn parse2Digits(s: &[]u8, pos: usize): i32;
export fn parse4Digits(s: &[]u8, pos: usize): i32;
export fn parseMonth(s: &[]u8, pos: usize): i32;
export fn isValidWeekday(s: &[]u8, pos: usize): bool;
export fn isLeapYear(y: u64): bool;
export fn daysInMonth(month: i32, leap: bool): i32;
export fn parseHttpDate(date_str: &[]u8): i64;
export fn matchIfNoneMatch(inm: &[]u8, res_etag: &[]u8): bool;
export fn isFresh(req: RequestHeaders, res: ResponseHeaders): bool;
