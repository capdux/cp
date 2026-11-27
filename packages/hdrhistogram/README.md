[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# hdrhistogram

High Dynamic Range (HDR) latency histogram for CP backed by native C `hdr_histogram.c` with RAII lifecycle management (`Histogram`), coordinated-omission correction, quantile/percentile queries, and histogram merging.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [crates.io/crates/hdrhistogram](https://crates.io/crates/hdrhistogram) |
| **Upstream Package** | `hdrhistogram` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/HdrHistogram/HdrHistogram_rust |
| **License** | BSD-2-Clause (upstream Rust wrapper: MIT) |

Keep the exact package name when migrating from Rust to CP:

```rust
// Rust
use hdrhistogram::Histogram;
let mut hist = Histogram::<u64>::new_with_bounds(1, 60_000_000, 3)?;
hist.record(42)?;
let p99 = hist.value_at_quantile(0.99);
```

```cp
// CP — same package name, native C speed + automatic RAII cleanup
import { Histogram } from "hdrhistogram";
```

## Installation

```bash
cap add hdrhistogram
```

## Quick Start & Practical Examples

### 1. Recording Latencies & Querying Percentiles / Quantiles

```cp
import { Histogram } from "hdrhistogram";

fn measureLatency(): !void {
    // Track latencies from 1 us to 60_000_000 us (60s) with 3 significant figures
    mut h = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    h.init(1, 60_000_000, 3)?; // Automatically closed via __cp_deinit on scope exit

    h.record(120)?;
    h.recordCount(250, 10)?;   // Record 10 samples of 250 us
    h.recordN(500, 5)?;        // Alias for recordCount
    h.saturatingRecord(99_000_000)?; // Clamps to highest_trackable instead of failing

    let total = h.len()?;      // 17
    let empty = h.isEmpty()?;  // false
    let min_v = h.min()?;
    let max_v = h.max()?;
    let avg = h.mean()?;
    let sd = h.stddev()?;      // Alias: h.stdev()

    // Percentile (0.0 .. 100.0) and Quantile (0.0 .. 1.0)
    let p99 = h.valueAtPercentile(99.0)?; // Alias: h.percentile(99.0)
    let q999 = h.valueAtQuantile(0.999)?;

    // Reverse lookup: percentage / quantile at or below a threshold
    let pct_under_300 = h.percentileBelow(300)?;
    let q_under_300 = h.quantileBelow(300)?;
}
```

### 2. Coordinated-Omission Correction & Histogram Merging

```cp
import { Histogram } from "hdrhistogram";

fn mergeWorkerHistograms(): !void {
    mut h1 = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    mut h2 = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    h1.open(1, 10_000_000, 3)?;
    h2.open(1, 10_000_000, 3)?;

    // Back-fill omitted samples when a stall exceeds expected_interval (1000 us)
    h1.recordCorrecting(5_000, 1_000)?;
    h2.recordCorrectingN(8_000, 2, 1_000)?;

    // Merge h2 into h1 (returns count of dropped out-of-range samples)
    let dropped = h1.add(h2)?;
    let mem_bytes = h1.memorySize()?;

    // Bucket equivalence helpers
    let eq = h1.equivalent(1000, 1001)?;
    let lo_eq = h1.lowestEquivalent(5_000)?;
    let med_eq = h1.medianEquivalent(5_000)?;
    let next_ne = h1.nextNonEquivalent(5_000)?;
    let bucket_span = h1.sizeOfEquivalentRange(5_000)?;

    h1.reset()?;
}
```

## Performance & Safety Highlights

- **Native C Core (`-O3`)**: Directly binds the battle-tested `hdr_histogram.c` engine with prebuilt static artifacts for sub-10ns `record()` latency.
- **Deterministic RAII Cleanup**: `Histogram.__cp_deinit` automatically invokes `hdr_close` when a `Histogram` leaves scope, preventing C-heap leaks even on early error returns.
- **Typed Error Propagation (`HdrError`)**: Returns `HdrError.InitFailed`, `HdrError.InvalidHandle`, or `HdrError.ValueOutOfRange` instead of silent corruption; use `saturatingRecord` to clamp outliers safely.
- **Coordinated-Omission Correction**: Built-in `recordCorrected` / `recordCorrecting` / `recordCorrectingN` and `addCorrecting` preserve accurate tail latencies under load-generator stalls.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures (including raw C FFI bindings), or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
