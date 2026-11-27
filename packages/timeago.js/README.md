[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# timeago.js

Zero-allocation deterministic relative time formatter (EN/ZH, Long/Short/Compact) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:timeago.js` ([https://github.com/hustcc/timeago.js](https://github.com/hustcc/timeago.js))（原库名 `timeago.js` → CP 包名 `timeago.js`）

| Metadata | Value |
| --- | --- |
| Package | `timeago.js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add timeago.js
```

```cp
import {
    TimeagoLocale,
    TimeagoStyle,
    TimeUnit,
    ScaledDelta,
    absU64,
    appendByte,
    appendStr,
    appendU64,
} from "timeago.js";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `TimeagoLocale` | `En, Zh` |
| `enum` | `TimeagoStyle` | `Long, Short` |
| `enum` | `TimeUnit` | `Second, Minute, Hour, Day, Week, Month, Year` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ScaledDelta` | `class` | Fields: `value: u64, unit: TimeUnit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absU64` | `(n: i64): u64` | Invokes `absU64` and returns `u64`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `appendU64` | `(out: []u8, pos: usize, val: u64): usize` | Invokes `appendU64` and returns `usize`. |
| `scaleDelta` | `(abs_sec: u64): ScaledDelta` | Invokes `scaleDelta` and returns `ScaledDelta`. |
| `enLongUnit` | `(unit: TimeUnit, plural: bool): string` | Invokes `enLongUnit` and returns `string`. |
| `enShortUnit` | `(unit: TimeUnit): string` | Invokes `enShortUnit` and returns `string`. |
| `zhLongUnit` | `(unit: TimeUnit): string` | Invokes `zhLongUnit` and returns `string`. |
| `zhShortUnit` | `(unit: TimeUnit): string` | Invokes `zhShortUnit` and returns `string`. |
| `formatTimeago` | `(delta_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string` | Serializes/formats data via `formatTimeago` returning `string`. |
| `formatBetween` | `(target_sec: i64, now_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string` | Serializes/formats data via `formatBetween` returning `string`. |
| `formatTimeagoEn` | `(delta_sec: i64, out: []u8): string` | Serializes/formats data via `formatTimeagoEn` returning `string`. |
| `formatTimeagoZh` | `(delta_sec: i64, out: []u8): string` | Serializes/formats data via `formatTimeagoZh` returning `string`. |
| `formatCompact` | `(delta_sec: i64, out: []u8): string` | Serializes/formats data via `formatCompact` returning `string`. |

## Minimal Example

```cp
import { appendStr } from "timeago.js";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
