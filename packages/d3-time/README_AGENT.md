# d3-time@1.0.0 (Agent Quickref)

> Zero-allocation UTC calendar time interval arithmetic (second, minute, hour, day, week, month, year) in pure CP | License: ISC
> Import: `import { ... } from "d3-time";` | Full types: `src/index.d.cp`

## Functions
- `export fn d3tmIsLeapYear(y: i64): bool;`
- `export fn d3tmDaysInMonth(y: i64, m: i64): i64;`
- `export fn d3tmCivilFromDays(z_in: i64): D3TmYmd;`
- `export fn d3tmDaysFromCivil(y_in: i64, m: i64, d: i64): i64;`
- `export fn utcFromYmd(year: i64, month: i64, day: i64): i64;`
- `export fn timeSecondFloor(ms: i64): i64;`
- `export fn timeSecondCeil(ms: i64): i64;`
- `export fn timeSecondOffset(ms: i64, step: i64): i64;`
- `export fn timeSecondCount(start_ms: i64, end_ms: i64): i64;`
- `export fn timeMinuteFloor(ms: i64): i64;`
- `export fn timeMinuteOffset(ms: i64, step: i64): i64;`
- `export fn timeHourFloor(ms: i64): i64;`
- `export fn timeHourOffset(ms: i64, step: i64): i64;`
- `export fn timeDayFloor(ms: i64): i64;`
- `export fn timeDayCeil(ms: i64): i64;`
- `export fn timeDayOffset(ms: i64, step: i64): i64;`
- `export fn timeDayCount(start_ms: i64, end_ms: i64): i64;`
- `export fn timeWeekFloor(ms: i64): i64;`
- `export fn timeWeekOffset(ms: i64, step: i64): i64;`
- `export fn timeMonthFloor(ms: i64): i64;`
- `export fn timeMonthOffset(ms: i64, step: i64): i64;`
- `export fn timeYearFloor(ms: i64): i64;`
- `export fn timeYearOffset(ms: i64, step: i64): i64;`
