# istanbul-lib-coverage — AI Agent Quick Ref

Zero-heap code coverage metrics, summary aggregation, and LCOV codec (`v3.2.2`, `BSD-3-Clause`)

## Import

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

## Core Types & Constants

```cp
class CoverageMetric { total: u32, covered: u32, skipped: u32 } // methods: pct(...) +1
class CoverageSummary { lines: CoverageMetric, statements: CoverageMetric, functions: CoverageMetric, branches: CoverageMetric } // methods: merge(...) +1
class LineHit { line: u32, hits: u32 }
```

## API Signatures

```cp
fn icovWriteByte(out: []u8, pos: usize, b: u8): usize;
fn icovParseU32(s: string): u32;
fn formatLcovRecord(file_path: string, hits: &[]LineHit, out: []u8): usize;
fn parseLcovSummary(lcov_text: string): CoverageMetric;
```

## Minimal Usage

```cp
import { formatLcovRecord } from "istanbul-lib-coverage";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatLcovRecord("example", /* hits: &[]LineHit */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"istanbul-lib-coverage"`; install via `cap add istanbul-lib-coverage`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
