// Type definitions for CP

import { CronSchedule, parseCron, matches } from "../../cron-parser/src/index.cp";
export class CronExpr6 {
    valid: bool,
    sec_mask: u64,
    base: CronSchedule,
    fn invalid(): CronExpr6;
}

export fn crxIsSpace(c: u8): bool;
export fn crxTrim(s: string): string;
export fn crxFindByte(s: string, target: u8): usize;
export fn crxParseUint(s: string): i32;
export fn crxParseSecField(field: string, ok_out: []bool): u64;
export fn parseCron6(expr: string): CronExpr6;
export fn matches6(sched: CronExpr6, month: u8, dom: u8, dow: u8, hour: u8, minute: u8, sec: u8): bool;
export fn nextSecInMinute(sched: CronExpr6, from_sec: u8): u8?;
export fn isValidCron6(expr: string): bool;
