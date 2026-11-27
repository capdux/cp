[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# istanbul-lib-coverage

Zero-heap code coverage metrics, summary aggregation, and LCOV codec

| Metadata | Value |
| --- | --- |
| Package | `istanbul-lib-coverage` |
| Version | `3.2.2` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add istanbul-lib-coverage
```

```cp
import {
    CoverageMetric,
    CoverageSummary,
    LineHit,
    icovWriteByte,
    icovParseU32,
    formatLcovRecord,
    parseLcovSummary,
} from "istanbul-lib-coverage";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CoverageMetric` | `class` | Fields: `total: u32, covered: u32, skipped: u32` · Methods: `pct, merge` |
| `CoverageSummary` | `class` | Fields: `lines: CoverageMetric, statements: CoverageMetric, functions: CoverageMetric, branches: CoverageMetric` · Methods: `merge, isAboveThreshold` |
| `LineHit` | `class` | Fields: `line: u32, hits: u32` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `icovWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `icovWriteByte` and returns `usize`. |
| `icovParseU32` | `(s: string): u32` | Invokes `icovParseU32` and returns `u32`. |
| `formatLcovRecord` | `(file_path: string, hits: &[]LineHit, out: []u8): usize` | Serializes/formats data via `formatLcovRecord` returning `usize`. |
| `parseLcovSummary` | `(lcov_text: string): CoverageMetric` | Parses input via `parseLcovSummary` and returns `CoverageMetric`. |

## Minimal Example

```cp
import { formatLcovRecord } from "istanbul-lib-coverage";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatLcovRecord("example", /* hits: &[]LineHit */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
