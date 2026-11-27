[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-time

Zero-allocation UTC calendar time interval arithmetic (second, minute, hour, day, week, month, year) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-time` ([https://github.com/d3/d3-time](https://github.com/d3/d3-time))

| Metadata | Value |
| --- | --- |
| Package | `d3-time` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-time
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `D3TmYmd` | `class` | Fields: `year: i64, month: i64, day: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `d3tmIsLeapYear` | `(y: i64): bool` | Invokes `d3tmIsLeapYear` and returns `bool`. |
| `d3tmDaysInMonth` | `(y: i64, m: i64): i64` | Invokes `d3tmDaysInMonth` and returns `i64`. |
| `d3tmCivilFromDays` | `(z_in: i64): D3TmYmd` | Invokes `d3tmCivilFromDays` and returns `D3TmYmd`. |
| `d3tmDaysFromCivil` | `(y_in: i64, m: i64, d: i64): i64` | Invokes `d3tmDaysFromCivil` and returns `i64`. |
| `utcFromYmd` | `(year: i64, month: i64, day: i64): i64` | Invokes `utcFromYmd` and returns `i64`. |
| `timeSecondFloor` | `(ms: i64): i64` | Invokes `timeSecondFloor` and returns `i64`. |
| `timeSecondCeil` | `(ms: i64): i64` | Invokes `timeSecondCeil` and returns `i64`. |
| `timeSecondOffset` | `(ms: i64, step: i64): i64` | Invokes `timeSecondOffset` and returns `i64`. |
| `timeSecondCount` | `(start_ms: i64, end_ms: i64): i64` | Invokes `timeSecondCount` and returns `i64`. |
| `timeMinuteFloor` | `(ms: i64): i64` | Invokes `timeMinuteFloor` and returns `i64`. |
| `timeMinuteOffset` | `(ms: i64, step: i64): i64` | Invokes `timeMinuteOffset` and returns `i64`. |
| `timeHourFloor` | `(ms: i64): i64` | Invokes `timeHourFloor` and returns `i64`. |
| `timeHourOffset` | `(ms: i64, step: i64): i64` | Invokes `timeHourOffset` and returns `i64`. |
| `timeDayFloor` | `(ms: i64): i64` | Invokes `timeDayFloor` and returns `i64`. |
| `timeDayCeil` | `(ms: i64): i64` | Invokes `timeDayCeil` and returns `i64`. |
| `timeDayOffset` | `(ms: i64, step: i64): i64` | Invokes `timeDayOffset` and returns `i64`. |
| `timeDayCount` | `(start_ms: i64, end_ms: i64): i64` | Invokes `timeDayCount` and returns `i64`. |
| `timeWeekFloor` | `(ms: i64): i64` | Invokes `timeWeekFloor` and returns `i64`. |
| `timeWeekOffset` | `(ms: i64, step: i64): i64` | Invokes `timeWeekOffset` and returns `i64`. |
| `timeMonthFloor` | `(ms: i64): i64` | Invokes `timeMonthFloor` and returns `i64`. |
| `timeMonthOffset` | `(ms: i64, step: i64): i64` | Invokes `timeMonthOffset` and returns `i64`. |
| `timeYearFloor` | `(ms: i64): i64` | Invokes `timeYearFloor` and returns `i64`. |
| `timeYearOffset` | `(ms: i64, step: i64): i64` | Invokes `timeYearOffset` and returns `i64`. |

## Minimal Example

```cp
import { d3tmIsLeapYear } from "d3-time";

pub fn main() {
    let result = d3tmIsLeapYear(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
