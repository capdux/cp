[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sparkly

Zero-heap Unicode sparkline (▁▂▃▄▅▆▇█), horizontal bar chart, and summary statistics renderer for CLI & monitoring

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sparkly` ([https://github.com/sindresorhus/sparkly](https://github.com/sindresorhus/sparkly))（原库名 `sparkly` → CP 包名 `sparkly`）

| Metadata | Value |
| --- | --- |
| Package | `sparkly` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sparkly
```

```cp
import {
    SeriesStats,
    SparkOptions,
    SparkResult,
    defaultOptions,
    computeStats,
    appendBlock,
    appendByte,
    appendBytes,
} from "sparkly";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SeriesStats` | `class` | Fields: `count: usize, minVal: f64, maxVal: f64, sum: f64, mean: f64, hasValues: bool` |
| `SparkOptions` | `class` | Fields: `useCustomBounds: bool, minBound: f64, maxBound: f64, asciiOnly: bool` |
| `SparkResult` | `class` | Fields: `buf: [512]u8, len: usize, truncated: bool` · Methods: `asSlice` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `defaultOptions` | `(): SparkOptions` | Invokes `defaultOptions` and returns `SparkOptions`. |
| `computeStats` | `(values: &[]f64): SeriesStats` | Invokes `computeStats` and returns `SeriesStats`. |
| `appendBlock` | `(res: &mut SparkResult, level: u8, asciiOnly: bool)` | Executes `appendBlock` in-place. |
| `appendByte` | `(res: &mut SparkResult, b: u8)` | Executes `appendByte` in-place. |
| `appendBytes` | `(res: &mut SparkResult, s: string)` | Executes `appendBytes` in-place. |
| `quantizeLevel` | `(value: f64, lo: f64, hi: f64): u8` | Invokes `quantizeLevel` and returns `u8`. |
| `renderWithOptions` | `(values: &[]f64, opts: SparkOptions): SparkResult` | Invokes `renderWithOptions` and returns `SparkResult`. |
| `render` | `(values: &[]f64): SparkResult` | Invokes `render` and returns `SparkResult`. |
| `renderAscii` | `(values: &[]f64): SparkResult` | Invokes `renderAscii` and returns `SparkResult`. |
| `renderBar` | `(ratio: f64, width: usize, fillAscii: bool): SparkResult` | Invokes `renderBar` and returns `SparkResult`. |
| `renderDownsampled` | `(values: &[]f64, targetBuckets: usize, asciiOnly: bool): SparkResult` | Invokes `renderDownsampled` and returns `SparkResult`. |
| `renderLabeledBar` | `(label: string, ratio: f64, barWidth: usize, fillAscii: bool): SparkResult` | Invokes `renderLabeledBar` and returns `SparkResult`. |

## Minimal Example

```cp
import { appendBytes } from "sparkly";

pub fn main() {
    appendBytes(/* res: &mut SparkResult */, "example");
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
