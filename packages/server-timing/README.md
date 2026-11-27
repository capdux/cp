[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# server-timing

Zero-allocation W3C Server-Timing HTTP response header parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:server-timing` ([https://github.com/yosuke-furukawa/server-timing](https://github.com/yosuke-furukawa/server-timing))

| Metadata | Value |
| --- | --- |
| Package | `server-timing` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add server-timing
```

```cp
import {
    TimingMetric,
    ParsedFloat,
    parseFloatSimple,
    parseSingleMetric,
    parseHeader,
    findMetric,
    totalDurationMs,
    isTokenChar,
} from "server-timing";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TimingMetric` | `class` | Fields: `name: string, has_dur: bool, dur_ms: f64, desc: string` |
| `ParsedFloat` | `class` | Fields: `val: f64, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseFloatSimple` | `(s: string): ParsedFloat` | Parses input via `parseFloatSimple` and returns `ParsedFloat`. |
| `parseSingleMetric` | `(seg: string, out: &mut TimingMetric): bool` | Parses input via `parseSingleMetric` and returns `bool`. |
| `parseHeader` | `(header: string, out: []TimingMetric): usize` | Parses input via `parseHeader` and returns `usize`. |
| `findMetric` | `(header: string, name: string, out: &mut TimingMetric): bool` | Queries/retrieves data via `findMetric` returning `bool`. |
| `totalDurationMs` | `(header: string): f64` | Invokes `totalDurationMs` and returns `f64`. |
| `isTokenChar` | `(b: u8): bool` | Checks predicate `isTokenChar` and returns `bool`. |
| `descNeedsQuotes` | `(s: string): bool` | Invokes `descNeedsQuotes` and returns `bool`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `pushU64` | `(dst: []u8, pos: usize, val: u64): i64` | Mutates or appends state via `pushU64`. |
| `pushDurMs` | `(dst: []u8, pos: usize, dur_ms: f64): i64` | Mutates or appends state via `pushDurMs`. |
| `formatMetric` | `(metric: TimingMetric, out: []u8): usize` | Serializes/formats data via `formatMetric` returning `usize`. |
| `formatHeader` | `(metrics: &[]TimingMetric, out: []u8): usize` | Serializes/formats data via `formatHeader` returning `usize`. |

## Minimal Example

```cp
import { parseFloatSimple } from "server-timing";

pub fn main() {
    let result = parseFloatSimple("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
