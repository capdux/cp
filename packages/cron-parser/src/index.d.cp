// Type definitions for CP

import { isSpace, eqIgnoreCase, trim, findByte } from "cap:ascii";
import { parseU16 } from "cap:fmt";
export fn crnParseUint(s: string): i32;
export fn crnDaysInMonth(month: u8): u8;
export fn crnParseField(field: string, min_val: i32, max_val: i32, is_dow: bool, ok_out: []bool): u64;
export class CronSchedule {
    valid: bool,
    min_mask: u64,
    hour_mask: u32,
    dom_mask: u32,
    month_mask: u16,
    dow_mask: u8,
    fn invalid(): CronSchedule;
}

export fn parseCron(expr: string): CronSchedule;
export fn matches(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): bool;
export fn nextMinuteOffset(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): u32?;
export fn isValidCron(expr: string): bool;
