# d3-time — AI Agent Quick Ref

Zero-allocation UTC calendar time interval arithmetic (second, minute, hour, day, week, month, year) in pure CP (`v1.0.0`, `ISC`)

## Import

```cp
import {
    D3TmYmd,
    d3tmIsLeapYear,
    d3tmDaysInMonth,
    d3tmCivilFromDays,
    d3tmDaysFromCivil,
    utcFromYmd,
    timeSecondFloor,
    timeSecondCeil,
} from "d3-time";
```

## Core Types & Constants

```cp
class D3TmYmd { year: i64, month: i64, day: i64 }
```

## API Signatures

```cp
fn d3tmIsLeapYear(y: i64): bool;
fn d3tmDaysInMonth(y: i64, m: i64): i64;
fn d3tmCivilFromDays(z_in: i64): D3TmYmd;
fn d3tmDaysFromCivil(y_in: i64, m: i64, d: i64): i64;
fn utcFromYmd(year: i64, month: i64, day: i64): i64;
fn timeSecondFloor(ms: i64): i64;
fn timeSecondCeil(ms: i64): i64;
fn timeSecondOffset(ms: i64, step: i64): i64;
fn timeSecondCount(start_ms: i64, end_ms: i64): i64;
fn timeMinuteFloor(ms: i64): i64;
fn timeMinuteOffset(ms: i64, step: i64): i64;
fn timeHourFloor(ms: i64): i64;
fn timeHourOffset(ms: i64, step: i64): i64;
fn timeDayFloor(ms: i64): i64;
fn timeDayCeil(ms: i64): i64;
fn timeDayOffset(ms: i64, step: i64): i64;
fn timeDayCount(start_ms: i64, end_ms: i64): i64;
fn timeWeekFloor(ms: i64): i64;
fn timeWeekOffset(ms: i64, step: i64): i64;
fn timeMonthFloor(ms: i64): i64;
fn timeMonthOffset(ms: i64, step: i64): i64;
fn timeYearFloor(ms: i64): i64;
fn timeYearOffset(ms: i64, step: i64): i64;
```

## Minimal Usage

```cp
import { d3tmIsLeapYear } from "d3-time";

pub fn main() {
    let result = d3tmIsLeapYear(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-time"`; install via `cap add d3-time`.
