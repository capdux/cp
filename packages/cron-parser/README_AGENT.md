# cron-parser — AI Agent Quick Ref

Zero-heap 5-field standard Cron expression parser, bitmask scheduler, and time matcher (clean-room port of cron-parser v4) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CronSchedule,
    crnParseUint,
    crnDaysInMonth,
    crnParseField,
    parseCron,
    matches,
    nextMinuteOffset,
    isValidCron,
} from "cron-parser";
```

## Core Types & Constants

```cp
class CronSchedule { valid: bool, min_mask: u64, hour_mask: u32, dom_mask: u32, /* +2 fields */ } // methods: invalid(...)
```

## API Signatures

```cp
fn crnParseUint(s: string): i32;
fn crnDaysInMonth(month: u8): u8;
fn crnParseField(field: string, min_val: i32, max_val: i32, is_dow: bool, ok_out: []bool): u64;
fn parseCron(expr: string): CronSchedule;
fn matches(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): bool;
fn nextMinuteOffset(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): u32?;
fn isValidCron(expr: string): bool;
```

## Minimal Usage

```cp
import { parseCron } from "cron-parser";

pub fn main() {
    let result = parseCron("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cron-parser"`; install via `cap add cron-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
