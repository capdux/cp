[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# postgres-date

Zero-allocation PostgreSQL TIMESTAMP / TIMESTAMPTZ / DATE / infinity parser with microsecond and timezone offset precision (clean-room MIT implementation of npm postgres-date, #505 / 112M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postgres-date` ([https://github.com/bendrucker/postgres-date](https://github.com/bendrucker/postgres-date)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `postgres-date` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add postgres-date
```

```cp
import {
    PgDateKind,
    PgDateTime,
    pgdParseDigits,
    pgdIsLeap,
    pgdDaysInMonth,
    parsePgDate,
    toUnixMs,
} from "postgres-date";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `PgDateKind` | `Invalid, Finite, PosInfinity, NegInfinity` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PgDateTime` | `class` | Fields: `kind: PgDateKind, year: i32, month: u8, day: u8, hour: u8, minute: u8, +4 more` · Methods: `isValid` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pgdParseDigits` | `(s: string): i32?` | Invokes `pgdParseDigits` and returns `i32?`. |
| `pgdIsLeap` | `(y: i32): bool` | Invokes `pgdIsLeap` and returns `bool`. |
| `pgdDaysInMonth` | `(y: i32, m: u8): u8` | Invokes `pgdDaysInMonth` and returns `u8`. |
| `parsePgDate` | `(input: string): PgDateTime` | Parses input via `parsePgDate` and returns `PgDateTime`. |
| `toUnixMs` | `(dt: PgDateTime): i64?` | Invokes `toUnixMs` and returns `i64?`. |

## Minimal Example

```cp
import { parsePgDate } from "postgres-date";

pub fn main() {
    let result = parsePgDate("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
