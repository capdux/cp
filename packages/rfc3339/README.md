[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rfc3339

Zero-heap strict RFC 3339 timestamp parser, Unix epoch converter, and UTC formatter (reuses iso8601)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rfc3339` ([https://github.com/chronotope/chrono](https://github.com/chronotope/chrono))

| Metadata | Value |
| --- | --- |
| Package | `rfc3339` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rfc3339
```

```cp
import {
    Rfc3339Timestamp,
    rfcIsDigit,
    daysFromCivil,
    civilFromDays,
    isValidRfc3339,
    parseRfc3339,
    fromUnixSeconds,
    formatRfc3339Utc,
} from "rfc3339";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rfc3339Timestamp` | `class` | Fields: `dt: IsoDateTime, unix_seconds: i64, millisecond: u16` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rfcIsDigit` | `(c: u8): bool` | Invokes `rfcIsDigit` and returns `bool`. |
| `daysFromCivil` | `(year: i32, month: u8, day: u8): i64` | Invokes `daysFromCivil` and returns `i64`. |
| `civilFromDays` | `(z_in: i64): IsoDate` | Invokes `civilFromDays` and returns `IsoDate`. |
| `isValidRfc3339` | `(s: string): bool` | Checks predicate `isValidRfc3339` and returns `bool`. |
| `parseRfc3339` | `(s: string): Rfc3339Timestamp?` | Parses input via `parseRfc3339` and returns `Rfc3339Timestamp?`. |
| `fromUnixSeconds` | `(unix_secs: i64): IsoDateTime` | Invokes `fromUnixSeconds` and returns `IsoDateTime`. |
| `formatRfc3339Utc` | `(unix_secs: i64, out: []u8): usize` | Serializes/formats data via `formatRfc3339Utc` returning `usize`. |
| `isRfcLeapYear` | `(year: i32): bool` | Checks predicate `isRfcLeapYear` and returns `bool`. |

## Minimal Example

```cp
import { isValidRfc3339 } from "rfc3339";

pub fn main() {
    let result = isValidRfc3339("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
