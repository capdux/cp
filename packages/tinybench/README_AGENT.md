# tinybench — AI Agent Quick Ref

Zero-heap micro-benchmark statistics engine (mean, ops/sec, min/max, p50/p75/p95/p99, stddev, RME) (`v3.2.0`, `MIT`)

## Import

```cp
import {
    BenchStats,
    tbnSqrt,
    tbnSortAsc,
    tbnPercentile,
    computeStats,
    speedupRatio,
} from "tinybench";
```

## Core Types & Constants

```cp
class BenchStats { count: usize, total_ns: f64, mean_ns: f64, min_ns: f64, /* +9 fields */ }
```

## API Signatures

```cp
fn tbnSqrt(x: f64): f64;
fn tbnSortAsc(arr: []f64);
fn tbnPercentile(sorted: &[]f64, p: f64): f64;
fn computeStats(samples_ns: &[]f64): BenchStats;
fn speedupRatio(baseline: BenchStats, candidate: BenchStats): f64;
```

## Minimal Usage

```cp
import { tbnSqrt } from "tinybench";

pub fn main() {
    let result = tbnSqrt(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tinybench"`; install via `cap add tinybench`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
