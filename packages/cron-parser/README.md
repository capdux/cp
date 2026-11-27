[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cron-parser

Zero-heap 5-field standard Cron expression parser, bitmask scheduler, and time matcher (clean-room port of cron-parser v4)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cron-parser` ([https://github.com/harrisiirak/cron-parser](https://github.com/harrisiirak/cron-parser))

| Metadata | Value |
| --- | --- |
| Package | `cron-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cron-parser
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CronSchedule` | `class` | Fields: `valid: bool, min_mask: u64, hour_mask: u32, dom_mask: u32, month_mask: u16, dow_mask: u8` · Methods: `invalid` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `crnParseUint` | `(s: string): i32` | Invokes `crnParseUint` and returns `i32`. |
| `crnDaysInMonth` | `(month: u8): u8` | Invokes `crnDaysInMonth` and returns `u8`. |
| `crnParseField` | `(field: string, min_val: i32, max_val: i32, is_dow: bool, ok_out: []bool): u64` | Invokes `crnParseField` and returns `u64`. |
| `parseCron` | `(expr: string): CronSchedule` | Parses input via `parseCron` and returns `CronSchedule`. |
| `matches` | `(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): bool` | Invokes `matches` and returns `bool`. |
| `nextMinuteOffset` | `(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): u32?` | Invokes `nextMinuteOffset` and returns `u32?`. |
| `isValidCron` | `(expr: string): bool` | Checks predicate `isValidCron` and returns `bool`. |

## Minimal Example

```cp
import { parseCron } from "cron-parser";

pub fn main() {
    let result = parseCron("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
