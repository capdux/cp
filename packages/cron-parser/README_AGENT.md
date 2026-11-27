# cron-parser@1.0.0 (Agent Quickref)

> Zero-heap 5-field standard Cron expression parser, bitmask scheduler, and time matcher (clean-room port of cron-parser v4) | License: MIT
> Import: `import { ... } from "cron-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn crnIsSpace(c: u8): bool;`
- `export fn crnToLower(c: u8): u8;`
- `export fn crnEqIgnoreCase(a: string, b: string): bool;`
- `export fn crnTrim(s: string): string;`
- `export fn crnParseUint(s: string): i32;`
- `export fn crnFindByte(s: string, target: u8): usize;`
- `export fn crnDaysInMonth(month: u8): u8;`
- `export fn crnParseField(field: string, min_val: i32, max_val: i32, is_dow: bool, ok_out: []bool): u64;`
- `export fn parseCron(expr: string): CronSchedule;`
- `export fn matches(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): bool;`
- `export fn nextMinuteOffset(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): u32?;`
- `export fn isValidCron(expr: string): bool;`
