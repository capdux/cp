// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class D3tfDateTime {
    year: u64,
    month: u64,
    day: u64,
    hour: u64,
    minute: u64,
    second: u64,
    ms: u64,
    weekday: u64,
    yearday: u64,
}

export fn d3tfIsLeapYear(y: u64): bool;
export fn d3tfDaysInMonth(y: u64, m: u64): u64;
export fn d3tfDayOfYear(y: u64, m: u64, d: u64): u64;
export fn d3tfFromUnixMs(unix_ms: i64): D3tfDateTime;
export fn d3tfToUnixMs(year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, ms: u64): i64;
export fn d3tfMonthShort(m: u64): string;
export fn d3tfMonthFull(m: u64): string;
export fn d3tfWeekdayShort(w: u64): string;
export fn d3tfWeekdayFull(w: u64): string;
export fn d3tfWritePadded(out: []u8, pos: usize, val: u64, width: usize): usize;
export fn utcFormat(spec: string, unix_ms: i64, out: []u8): usize;
export fn d3tfMatchWordCI(input: string, pos: usize, word: string): bool;
export class D3tfUintParse {
    ok: bool,
    val: u64,
    next_pos: usize,
}

export fn d3tfParseDigits(input: string, pos: usize, min_d: usize, max_d: usize): D3tfUintParse;
export fn utcParse(spec: string, input: string): i64?;
