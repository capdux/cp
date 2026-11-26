// Type definitions for CP

export class CronSchedule {
    minutes: u64,
    hours: u32,
    days: u32,
    months: u16,
    dows: u8,
    valid: bool,
}

export fn makeInvalid(): CronSchedule;
export fn isWs(c: u8): bool;
export fn bytesEq(a: string, b: string): bool;
export fn parseU8Strict(s: string): i32;
export fn parseField(field: string, min_v: u8, max_v: u8, allow_sun7: bool, out_mask: &mut u64): bool;
export fn parseCron(expr: string): CronSchedule;
export fn matches(sched: CronSchedule, minute: u8, hour: u8, day: u8, month: u8, dow: u8): bool;
export fn isValidCron(expr: string): bool;
export fn popcount64(v: u64): u32;
export fn countMatchingMinutesPerDay(sched: CronSchedule): u32;
