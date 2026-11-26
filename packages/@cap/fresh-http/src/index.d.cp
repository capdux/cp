// Type definitions for CP

import { containsIgnoreCase } from "../../unicase/src/index.cp";
export fn frshIsSpace(c: u8): bool;
export fn frshIsDigit(c: u8): bool;
export fn frshToLower(c: u8): u8;
export fn frshStrEq(a: string, b: string): bool;
export fn frshEqIgnoreCase(a: string, b: string): bool;
export fn frshTrim(s: string): string;
export fn frshIsValidWeekday(s: string): bool;
export fn frshParseMonth(s: string): u64;
export fn frshIsLeapYear(y: u64): bool;
export fn frshDaysInMonth(y: u64, m: u64): u64;
export fn frshParse2Digits(s: string, pos: usize, out_val: &mut u64): bool;
export fn frshParse4Digits(s: string, pos: usize, out_val: &mut u64): bool;
export fn frshParseEtag(raw: string, is_weak: &mut bool): string;
export fn frshIsDirectiveBoundary(c: u8): bool;
export fn hasNoCacheDirective(cache_control: string): bool;
export fn etagWeakMatch(a: string, b: string): bool;
export fn etagStrongMatch(a: string, b: string): bool;
export fn parseHttpDate(date_str: string): i64?;
export fn isFresh(if_none_match: string, if_modified_since: string, cache_control: string, etag: string, last_modified: string): bool;
