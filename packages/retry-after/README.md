[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# retry-after

Zero-heap RFC 9110 HTTP Retry-After header parser and formatter (delay-seconds & HTTP-date) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:retry-after` ([https://github.com/jwilm/retry-after](https://github.com/jwilm/retry-after))

| Metadata | Value |
| --- | --- |
| Package | `retry-after` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add retry-after
```

```cp
import {
    RetryAfter,
    parseUint,
    parseMonth,
    isLeapYear,
    daysInMonth,
    daysFromCivil,
    parseHttpDate,
    parseRetryAfter,
} from "retry-after";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RetryAfter` | `class` | Fields: `kind: u8, delay_seconds: u64, epoch_seconds: i64, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseUint` | `(s: &[]u8): i64` | Parses input via `parseUint` and returns `i64`. |
| `parseMonth` | `(s: &[]u8): i32` | Parses input via `parseMonth` and returns `i32`. |
| `isLeapYear` | `(y: i64): bool` | Checks predicate `isLeapYear` and returns `bool`. |
| `daysInMonth` | `(y: i64, m: i32): i32` | Invokes `daysInMonth` and returns `i32`. |
| `daysFromCivil` | `(y_in: i64, m_in: i64, d: i64): i64` | Invokes `daysFromCivil` and returns `i64`. |
| `parseHttpDate` | `(s: &[]u8): i64` | Parses input via `parseHttpDate` and returns `i64`. |
| `parseRetryAfter` | `(header: &[]u8): RetryAfter` | Parses input via `parseRetryAfter` and returns `RetryAfter`. |
| `waitSeconds` | `(ra: RetryAfter, now_epoch_sec: i64): i64` | Invokes `waitSeconds` and returns `i64`. |
| `formatDelaySeconds` | `(seconds: u64, dst: []u8): i64` | Serializes/formats data via `formatDelaySeconds` returning `i64`. |

## Minimal Example

```cp
import { parseUint } from "retry-after";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseUint(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
