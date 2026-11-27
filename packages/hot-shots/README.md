[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hot-shots

Zero-heap StatsD and DogStatsD UDP metric packet encoder/decoder supporting Counter, Timing, Gauge, Set, sample rates, and tags.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hot-shots` ([https://github.com/brightcove/hot-shots](https://github.com/brightcove/hot-shots))（原库名 `hot-shots` → CP 包名 `hot-shots`）

| Metadata | Value |
| --- | --- |
| Package | `hot-shots` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hot-shots
```

```cp
import {
    StatsdMetric,
    isValidMetricName,
    isValidTags,
    appendByte,
    appendSlice,
    appendI64,
    formatMetric,
    parseSampleRateMilli,
} from "hot-shots";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StatsdMetric` | `class` | Fields: `name_start: usize, name_len: usize, value: i64, kind: u8, sample_rate_milli: u16, tags_start: usize, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isValidMetricName` | `(name: &[]u8): bool` | Checks predicate `isValidMetricName` and returns `bool`. |
| `isValidTags` | `(tags: &[]u8): bool` | Checks predicate `isValidTags` and returns `bool`. |
| `appendByte` | `(dst: []u8, pos: usize, b: u8): i64` | Invokes `appendByte` and returns `i64`. |
| `appendSlice` | `(dst: []u8, pos: usize, s: &[]u8): i64` | Invokes `appendSlice` and returns `i64`. |
| `appendI64` | `(dst: []u8, pos: usize, v: i64): i64` | Invokes `appendI64` and returns `i64`. |
| `formatMetric` | `(name: &[]u8, value: i64, kind: u8, sample_rate_milli: u16, tags: &[]u8, dst: []u8): i64` | Serializes/formats data via `formatMetric` returning `i64`. |
| `parseSampleRateMilli` | `(s: &[]u8): i32` | Parses input via `parseSampleRateMilli` and returns `i32`. |
| `parseMetric` | `(packet: &[]u8): StatsdMetric` | Parses input via `parseMetric` and returns `StatsdMetric`. |

## Minimal Example

```cp
import { isValidMetricName } from "hot-shots";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidMetricName(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
