[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tzfile

Zero-heap RFC 8536 TZif binary header parser, POSIX TZ string parser, and UTC offset transition evaluator (crates.io tzfile equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tzfile` ([https://github.com/x-hgg-x/tz-rs](https://github.com/x-hgg-x/tz-rs))

| Metadata | Value |
| --- | --- |
| Package | `tzfile` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tzfile
```

```cp
import {
    TzifHeader,
    PosixTz,
    TzTransition,
    TzAbbrRes,
    TzOffRes,
    tzReadU32Be,
    isTzifMagic,
    parseTzifHeader,
} from "tzfile";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TzifHeader` | `class` | Fields: `version: u8, isutcnt: u32, isstdcnt: u32, leapcnt: u32, timecnt: u32, typecnt: u32, +1 more` |
| `PosixTz` | `class` | Fields: `std_abbr: string, std_offset_secs: i32, has_dst: bool, dst_abbr: string, dst_offset_secs: i32` |
| `TzTransition` | `class` | Fields: `at_unix: i64, offset_secs: i32, is_dst: bool` |
| `TzAbbrRes` | `class` | Fields: `abbr: string, next_pos: usize, ok: bool` |
| `TzOffRes` | `class` | Fields: `secs: i32, next_pos: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `tzReadU32Be` | `(buf: []u8, off: usize): u32` | Invokes `tzReadU32Be` and returns `u32`. |
| `isTzifMagic` | `(buf: []u8): bool` | Checks predicate `isTzifMagic` and returns `bool`. |
| `parseTzifHeader` | `(buf: []u8): TzifHeader?` | Parses input via `parseTzifHeader` and returns `TzifHeader?`. |
| `tzIsAlpha` | `(c: u8): bool` | Invokes `tzIsAlpha` and returns `bool`. |
| `tzIsDigit` | `(c: u8): bool` | Invokes `tzIsDigit` and returns `bool`. |
| `tzParseAbbr` | `(s: string, pos_in: usize): TzAbbrRes` | Invokes `tzParseAbbr` and returns `TzAbbrRes`. |
| `tzParsePosixOffset` | `(s: string, pos_in: usize): TzOffRes` | Invokes `tzParsePosixOffset` and returns `TzOffRes`. |
| `parsePosixTz` | `(spec: string): PosixTz?` | Parses input via `parsePosixTz` and returns `PosixTz?`. |
| `lookupTransitionOffset` | `(transitions: []TzTransition, unix_secs: i64, default_offset: i32): i32` | Queries/retrieves data via `lookupTransitionOffset` returning `i32`. |
| `utcToLocalSeconds` | `(unix_secs: i64, offset_secs: i32): i64` | Invokes `utcToLocalSeconds` and returns `i64`. |

## Minimal Example

```cp
import { parsePosixTz } from "tzfile";

pub fn main() {
    let result = parsePosixTz("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
