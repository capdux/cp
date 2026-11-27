# cron@1.0.0 — AI Agent Quick Ref

> Zero-alloc 5-field POSIX Cron bitmask parser & next UTC trigger calculator | Source: `crates.io:cron` | License: MIT
> Import: `import { CronSchedule, epochSecToUtc, utcToEpochSec } from "cron";` | Types: `src/index.d.cp`

## Core Types & Methods

```cp
export class CronSchedule {
    min_mask: u64, hour_mask: u32, dom_mask: u32, mon_mask: u16, dow_mask: u8,
    dom_any: bool, dow_any: bool, valid: bool,
    fn parse(expr_in: &[]u8): CronSchedule;
    fn isValid(): bool;
    fn matchesDay(day: u32, dow: u32): bool;
    fn matchesUtc(epoch_sec: i64): bool;
    fn nextAfter(epoch_sec: i64): i64;
}

export class UtcDateTime {
    year: u32, month: u32, day: u32, hour: u32, minute: u32, dow: u32,
}

export class FieldResult { mask: u64, is_star: bool, ok: bool }
```

## Exported Functions

- `epochSecToUtc(sec: u64): UtcDateTime`
- `utcToEpochSec(year: u32, month: u32, day: u32, hour: u32, minute: u32): u64`
- `isLeapYear(y: u32): bool` | `daysInMonth(y: u32, m: u32): u32` | `dowForDate(year: u32, month: u32, day: u32): u32`
- `parseField(field: &[]u8, min_val: u32, max_val: u32, is_dow: bool): FieldResult`
- `parseUint(s: &[]u8): i32` | `bit64/bit32/bit16/bit8(idx: u32)`

## Minimal Usage

```cp
import { CronSchedule } from "cron";

const expr: []const u8 = "0 9 * * 1-5";
const sched = CronSchedule.parse(&expr);
if (sched.isValid()) {
    const next_ts = sched.nextAfter(1735689600 as i64); // -1 if unreachable
}
```

## Pitfalls & Rules

1. **5-field POSIX only**: Supports `min hour dom month dow` plus macros `@hourly`, `@daily`, `@weekly`, `@monthly`, `@yearly`. Does NOT take 6-field (seconds) or 7-field (year) expressions.
2. **Slice borrow parameter**: `CronSchedule.parse(expr_in: &[]u8)` expects a byte-slice borrow (`const expr: []const u8 = "..."; CronSchedule.parse(&expr)`).
3. **Check `isValid()` / `-1`**: Invalid syntax yields `valid == false`; `nextAfter` returns `-1` on invalid schedules or impossible dates (e.g. `0 0 31 2 *`).
4. **POSIX DOM/DOW semantics**: When both `dom` and `dow` are non-`*`, a date matches if EITHER matches (OR semantics). DOW `0` and `7` both mean Sunday.
5. **UTC only**: All timestamps are Unix epoch seconds in UTC; `nextAfter` always returns a minute-aligned timestamp strictly greater than `epoch_sec`.
