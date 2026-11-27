[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# httpdate

Zero-allocation RFC 9110 Section 5.6.7 HTTP-date parser (IMF-fixdate, RFC 850, asctime) and formatter with Hinnant calendar math in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:httpdate` ([https://github.com/pyfisch/httpdate](https://github.com/pyfisch/httpdate)) / `npm:http-date` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `httpdate` → CP 包名 `httpdate`）

| Metadata | Value |
| --- | --- |
| Package | `httpdate` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add httpdate
```

```cp
import {
    HttpDateTime,
    invalidDateTime,
    isOws,
    isLeapYear,
    daysInMonth,
    daysFromCivil,
    buildValidated,
    parse2Digits,
} from "httpdate";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HttpDateTime` | `class` | Fields: `year: u16, month: u8, day: u8, hour: u8, minute: u8, second: u8, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidDateTime` | `(): HttpDateTime` | Invokes `invalidDateTime` and returns `HttpDateTime`. |
| `isOws` | `(b: u8): bool` | Checks predicate `isOws` and returns `bool`. |
| `isLeapYear` | `(y: u16): bool` | Checks predicate `isLeapYear` and returns `bool`. |
| `daysInMonth` | `(y: u16, m: u8): u8` | Invokes `daysInMonth` and returns `u8`. |
| `daysFromCivil` | `(year: u16, month: u8, day: u8): u64` | Invokes `daysFromCivil` and returns `u64`. |
| `buildValidated` | `(year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32): HttpDateTime` | Invokes `buildValidated` and returns `HttpDateTime`. |
| `parse2Digits` | `(s: string, pos: usize): i32` | Parses input via `parse2Digits` and returns `i32`. |
| `parse4Digits` | `(s: string, pos: usize): i32` | Parses input via `parse4Digits` and returns `i32`. |
| `parseMonth` | `(s: string, pos: usize): i32` | Parses input via `parseMonth` and returns `i32`. |
| `isShortWeekday` | `(s: string, pos: usize): bool` | Checks predicate `isShortWeekday` and returns `bool`. |
| `isLongWeekday` | `(s: string): bool` | Checks predicate `isLongWeekday` and returns `bool`. |
| `parseImfFixdate` | `(s: string): HttpDateTime` | Parses input via `parseImfFixdate` and returns `HttpDateTime`. |
| `parseRfc850` | `(s: string): HttpDateTime` | Parses input via `parseRfc850` and returns `HttpDateTime`. |
| `parseAsctime` | `(s: string): HttpDateTime` | Parses input via `parseAsctime` and returns `HttpDateTime`. |
| `parseHttpDate` | `(input: string): HttpDateTime` | Parses input via `parseHttpDate` and returns `HttpDateTime`. |
| `toUnixSeconds` | `(dt: HttpDateTime): i64` | Invokes `toUnixSeconds` and returns `i64`. |
| `fromUnixSeconds` | `(ts: i64): HttpDateTime` | Invokes `fromUnixSeconds` and returns `HttpDateTime`. |
| `weekdayName` | `(wd: u8): string` | Invokes `weekdayName` and returns `string`. |
| `monthName` | `(m: u8): string` | Invokes `monthName` and returns `string`. |
| `formatImfFixdate` | `(dt: HttpDateTime, out: []u8): usize` | Serializes/formats data via `formatImfFixdate` returning `usize`. |
| `formatUnixHttpDate` | `(ts: i64, out: []u8): usize` | Serializes/formats data via `formatUnixHttpDate` returning `usize`. |
| `parseUnixSeconds` | `(input: string): i64` | Parses input via `parseUnixSeconds` and returns `i64`. |

## Minimal Example

```cp
import { parse2Digits } from "httpdate";

pub fn main() {
    let result = parse2Digits("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
