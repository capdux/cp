[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rrule

Zero-allocation RFC 5545 iCalendar RRULE parser, validator & formatter (npm rrule v2 parity) composing unicase for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rrule` ([https://github.com/jkbrzt/rrule](https://github.com/jkbrzt/rrule))（原库名 `rrule` → CP 包名 `rrule`）

| Metadata | Value |
| --- | --- |
| Package | `rrule` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rrule
```

```cp
import {
    RRule,
    rrlInvalid,
    rrlParsePosU32,
    rrlParseFreq,
    rrlParseDayCode,
    rrlParseByDayItem,
    rrlParse2Digits,
    rrlIsValidUntil,
} from "rrule";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RRule` | `class` | Fields: `valid: bool, freq: u8, interval: u32, count: u32, has_count: bool, until: string, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rrlInvalid` | `(): RRule` | Invokes `rrlInvalid` and returns `RRule`. |
| `rrlParsePosU32` | `(s: string): u32` | Invokes `rrlParsePosU32` and returns `u32`. |
| `rrlParseFreq` | `(v: string): u8` | Invokes `rrlParseFreq` and returns `u8`. |
| `rrlParseDayCode` | `(code: string): u8` | Invokes `rrlParseDayCode` and returns `u8`. |
| `rrlParseByDayItem` | `(item: string): u8` | Invokes `rrlParseByDayItem` and returns `u8`. |
| `rrlParse2Digits` | `(s: string, pos: usize): u32` | Invokes `rrlParse2Digits` and returns `u32`. |
| `rrlIsValidUntil` | `(v: string): bool` | Invokes `rrlIsValidUntil` and returns `bool`. |
| `parseRRule` | `(input: string): RRule` | Parses input via `parseRRule` and returns `RRule`. |
| `rrlFreqName` | `(freq: u8): string` | Invokes `rrlFreqName` and returns `string`. |
| `rrlDayName` | `(mask: u8): string` | Invokes `rrlDayName` and returns `string`. |
| `formatRRule` | `(r: RRule, out: []u8): usize` | Serializes/formats data via `formatRRule` returning `usize`. |
| `matchesWeekday` | `(r: RRule, dow: u8): bool` | Invokes `matchesWeekday` and returns `bool`. |
| `isValidRRule` | `(input: string): bool` | Checks predicate `isValidRRule` and returns `bool`. |

## Minimal Example

```cp
import { parseRRule } from "rrule";

pub fn main() {
    let result = parseRRule("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
