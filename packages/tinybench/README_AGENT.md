# tinybench@3.2.0 (Agent Quickref)

> Zero-heap micro-benchmark statistics engine (mean, ops/sec, min/max, p50/p75/p95/p99, stddev, RME) | License: MIT
> Import: `import { ... } from "tinybench";` | Full types: `src/index.d.cp`

## Functions
- `export fn computeStats(samples_ns: &[]f64): BenchStats;`
- `export fn speedupRatio(baseline: BenchStats, candidate: BenchStats): f64;`
