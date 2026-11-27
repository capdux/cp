# sparkly — AI Agent Quick Ref

Zero-heap Unicode sparkline (▁▂▃▄▅▆▇█), horizontal bar chart, and summary statistics renderer for CLI & monitoring (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class SeriesStats { count: usize, minVal: f64, maxVal: f64, sum: f64, /* +2 fields */ }
class SparkOptions { useCustomBounds: bool, minBound: f64, maxBound: f64, asciiOnly: bool }
class SparkResult { buf: [512]u8, len: usize, truncated: bool } // methods: asSlice(...)
```

## API Signatures

```cp
fn defaultOptions(): SparkOptions;
fn computeStats(values: &[]f64): SeriesStats;
fn appendBlock(res: &mut SparkResult, level: u8, asciiOnly: bool);
fn appendByte(res: &mut SparkResult, b: u8);
fn appendBytes(res: &mut SparkResult, s: string);
fn quantizeLevel(value: f64, lo: f64, hi: f64): u8;
fn renderWithOptions(values: &[]f64, opts: SparkOptions): SparkResult;
fn render(values: &[]f64): SparkResult;
fn renderAscii(values: &[]f64): SparkResult;
fn renderBar(ratio: f64, width: usize, fillAscii: bool): SparkResult;
fn renderDownsampled(values: &[]f64, targetBuckets: usize, asciiOnly: bool): SparkResult;
fn renderLabeledBar(label: string, ratio: f64, barWidth: usize, fillAscii: bool): SparkResult;
```

## Minimal Usage

```cp
import { appendBytes } from "sparkly";

pub fn main() {
    appendBytes(/* res: &mut SparkResult */, "example");
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sparkly"`; install via `cap add sparkly`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
