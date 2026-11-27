// Type definitions for CP

import { isSpace, isDigit, trim } from "cap:ascii";
import { bytesEq } from "cap:bin";
export fn bit64(idx: u32): u64;
export fn bit32(idx: u32): u32;
export fn bit16(idx: u32): u16;
export fn bit8(idx: u32): u8;
export fn parseUint(s: &[]u8): i32;
export class FieldResult {
    mask: u64,
    is_star: bool,
    ok: bool,
}

export fn parseField(field: &[]u8, min_val: u32, max_val: u32, is_dow: bool): FieldResult;
export class UtcDateTime {
    year: u32,
    month: u32,
    day: u32,
    hour: u32,
    minute: u32,
    dow: u32,
}

export fn isLeapYear(y: u32): bool;
export fn daysInMonth(y: u32, m: u32): u32;
export fn epochSecToUtc(sec: u64): UtcDateTime;
export fn utcToEpochSec(year: u32, month: u32, day: u32, hour: u32, minute: u32): u64;
export fn dowForDate(year: u32, month: u32, day: u32): u32;
export class CronSchedule {
    min_mask: u64,
    hour_mask: u32,
    dom_mask: u32,
    mon_mask: u16,
    dow_mask: u8,
    dom_any: bool,
    dow_any: bool,
    valid: bool,
    fn parse(expr_in: &[]u8): CronSchedule;
    fn isValid(): bool;
    fn matchesDay(day: u32, dow: u32): bool;
    fn matchesUtc(epoch_sec: i64): bool;
    fn nextAfter(epoch_sec: i64): i64;
}

