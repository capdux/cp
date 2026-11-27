[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# cron

Zero-allocation 5-field POSIX Cron bitmask parser and next UTC trigger timestamp calculator for CP — clean-room MIT parity with Rust `cron` and npm `cron-parser`.

Parses standard 5-field POSIX schedules (`minute hour dom month dow`) and predefined macros (`@hourly`, `@daily`, `@weekly`, `@monthly`, `@yearly`) into compact integer bitmasks (`u64`/`u32`/`u16`/`u8`), with exact leap-year (`Feb 29`) jump calculation and zero heap allocation.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Package | `cron` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/zslayton/cron` |
| Upstream License | `MIT OR Apache-2.0` (CP clean-room: `MIT`) |

Keep the exact package name when migrating from Rust to CP:

```rust
// Rust (crates.io: cron)
use cron::Schedule;
use std::str::FromStr;

let schedule = Schedule::from_str("0 0 9 * * 1-5").unwrap();
let next = schedule.upcoming(chrono::Utc).next();
```

```cp
// CP (cap: cron) — zero-allocation bitmask schedule & UTC epoch calculation
import { CronSchedule } from "cron";

const expr: []const u8 = "0 9 * * 1-5";
const sched = CronSchedule.parse(&expr);
const next_ts = sched.nextAfter(1735689600 as i64);
```

## Installation

```bash
cap add cron
```

## Usage

### Basic Usage: Parse, Validate & Compute Next UTC Trigger

```cp
import { CronSchedule, epochSecToUtc, utcToEpochSec } from "cron";

fn demoSchedule() {
    const expr: []const u8 = "*/15 9-17 * * 1-5";
    const sched = CronSchedule.parse(&expr);
    if (!sched.isValid()) {
        return;
    }

    // 2025-01-01T08:50:00Z
    const now = utcToEpochSec(2025, 1, 1, 8, 50) as i64;
    const is_now = sched.matchesUtc(now);    // false
    const next = sched.nextAfter(now);       // 2025-01-01T09:00:00Z
    const dt = epochSecToUtc(next as u64);
    // dt.year == 2025, dt.month == 1, dt.day == 1, dt.hour == 9, dt.minute == 0
    _ = is_now;
    _ = dt;
}
```

### Edge Cases: Leap-Year Feb 29, Macros & Invalid Expressions

```cp
import { CronSchedule, isLeapYear, daysInMonth } from "cron";

fn demoEdgeCases() {
    // 1. Leap-year jump: 2025-01-01 -> 2028-02-29T00:00:00Z (1835395200)
    const leap_expr: []const u8 = "0 0 29 2 *";
    const leap_sched = CronSchedule.parse(&leap_expr);
    const next_leap = leap_sched.nextAfter(1735689600 as i64);

    // 2. Predefined macros (@hourly, @daily, @weekly, @monthly, @yearly)
    const daily_expr: []const u8 = "@daily";
    const daily = CronSchedule.parse(&daily_expr);

    // 3. Invalid expression & impossible calendar date (Feb 31)
    const bad_expr: []const u8 = "60 * * * *";
    const bad = CronSchedule.parse(&bad_expr);
    // bad.isValid() == false; bad.nextAfter(1735689600) == -1

    const feb31_expr: []const u8 = "0 0 31 2 *";
    const feb31 = CronSchedule.parse(&feb31_expr);
    // feb31.nextAfter(1735689600) == -1 (no matching date within search horizon)

    const ly = isLeapYear(2028);      // true
    const dim = daysInMonth(2028, 2); // 29
    _ = next_leap;
    _ = daily;
    _ = bad;
    _ = feb31;
    _ = ly;
    _ = dim;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `CronSchedule.parse` | `fn(expr_in: &[]u8): CronSchedule` | Parses a 5-field POSIX Cron string or macro into bitmasks (`valid == false` on syntax/range error). |
| `CronSchedule.isValid` | `fn(): bool` | Returns whether the parsed schedule is valid. |
| `CronSchedule.matchesDay` | `fn(day: u32, dow: u32): bool` | Checks POSIX DOM/DOW matching semantics (OR when both are restricted, AND when either is `*`). |
| `CronSchedule.matchesUtc` | `fn(epoch_sec: i64): bool` | Returns `true` if the UTC Unix timestamp matches the schedule. |
| `CronSchedule.nextAfter` | `fn(epoch_sec: i64): i64` | Computes the next strictly-greater minute-aligned UTC Unix timestamp (`-1` if invalid or unreachable). |
| `epochSecToUtc` | `fn(sec: u64): UtcDateTime` | Converts UTC epoch seconds to `{ year, month, day, hour, minute, dow }`. |
| `utcToEpochSec` | `fn(year: u32, month: u32, day: u32, hour: u32, minute: u32): u64` | Converts UTC date/time components to epoch seconds. |
| `isLeapYear` / `daysInMonth` / `dowForDate` | `fn(...) -> bool / u32` | Calendar helper utilities (Gregorian leap years, month lengths, day-of-week `0..6`). |
| `parseField` | `fn(field: &[]u8, min_val: u32, max_val: u32, is_dow: bool): FieldResult` | Low-level single-field bitmask parser (`*`, `*/n`, `a-b`, `a-b/n`, `a,b,c`). |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isSpace`, `isDigit`, `trim`) and `cap:bin` (`bytesEq`) for fast tokenization and macro matching.
- **Zero Heap Allocation & Compact Bitmasks**: The entire `CronSchedule` fits in stack registers (`u64`/`u32`/`u16`/`u8` bitmasks) with O(1) bit checks.
- **Zero-Clear Stack Arrays (`undefined`)**: Internal field-span arrays (`[5]usize = undefined`) avoid redundant zero-initialization overhead.
- **Safe Failure Semantics**: Never panics on malformed inputs or impossible dates (e.g., Feb 31); returns `valid == false` or `-1`.

## License

MIT — Clean-room CP implementation inspired by crates.io `cron`.
