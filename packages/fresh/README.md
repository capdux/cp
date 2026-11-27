[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fresh

Zero-allocation RFC 9110 HTTP conditional request freshness testing (ETag, If-None-Match, If-Modified-Since, Cache-Control) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fresh` ([https://github.com/jshttp/fresh](https://github.com/jshttp/fresh))

| Metadata | Value |
| --- | --- |
| Package | `fresh` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fresh
```

```cp
import {
    RequestHeaders,
    ResponseHeaders,
    TrimRange,
    ParsedEtag,
    trimRange,
    hasNoCache,
    parseEtagSlice,
    etagWeakMatch,
} from "fresh";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RequestHeaders` | `class` | Fields: `if_none_match: &[]u8, if_modified_since: &[]u8, cache_control: &[]u8` |
| `ResponseHeaders` | `class` | Fields: `etag: &[]u8, last_modified: &[]u8` |
| `TrimRange` | `class` | Fields: `start: usize, end: usize` |
| `ParsedEtag` | `class` | Fields: `start: usize, end: usize, weak: bool, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `trimRange` | `(s: &[]u8, start: usize, end: usize): TrimRange` | Invokes `trimRange` and returns `TrimRange`. |
| `hasNoCache` | `(cache_control: &[]u8): bool` | Checks predicate `hasNoCache` and returns `bool`. |
| `parseEtagSlice` | `(s: &[]u8, raw_start: usize, raw_end: usize): ParsedEtag` | Parses input via `parseEtagSlice` and returns `ParsedEtag`. |
| `etagWeakMatch` | `(a: &[]u8, b: &[]u8): bool` | Invokes `etagWeakMatch` and returns `bool`. |
| `etagStrongMatch` | `(a: &[]u8, b: &[]u8): bool` | Invokes `etagStrongMatch` and returns `bool`. |
| `parse2Digits` | `(s: &[]u8, pos: usize): i32` | Parses input via `parse2Digits` and returns `i32`. |
| `parse4Digits` | `(s: &[]u8, pos: usize): i32` | Parses input via `parse4Digits` and returns `i32`. |
| `parseMonth` | `(s: &[]u8, pos: usize): i32` | Parses input via `parseMonth` and returns `i32`. |
| `isValidWeekday` | `(s: &[]u8, pos: usize): bool` | Checks predicate `isValidWeekday` and returns `bool`. |
| `isLeapYear` | `(y: u64): bool` | Checks predicate `isLeapYear` and returns `bool`. |
| `daysInMonth` | `(month: i32, leap: bool): i32` | Invokes `daysInMonth` and returns `i32`. |
| `parseHttpDate` | `(date_str: &[]u8): i64` | Parses input via `parseHttpDate` and returns `i64`. |
| `matchIfNoneMatch` | `(inm: &[]u8, res_etag: &[]u8): bool` | Invokes `matchIfNoneMatch` and returns `bool`. |
| `isFresh` | `(req: RequestHeaders, res: ResponseHeaders): bool` | Checks predicate `isFresh` and returns `bool`. |

## Minimal Example

```cp
import { parseEtagSlice } from "fresh";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseEtagSlice(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
