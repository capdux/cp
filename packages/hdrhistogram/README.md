[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hdrhistogram

High Dynamic Range (HDR) Histogram for CP with RAII native handle management, coordinated omission correction, percentile/quantile queries, and histogram merging.

- **Upstream Alignment**: Modeled after Rust [`hdrhistogram`](https://github.com/HdrHistogram/HdrHistogram_rust) (`crates.io`, MIT) backed by the native C `hdr_histogram.c` engine (`BSD-2-Clause`).
- **Package Version**: `1.0.0`

## Installation & Import

```bash
cap add hdrhistogram
```

In `cap.json`:

```json
{
  "dependencies": {
    "hdrhistogram": "^1.0.0"
  }
}
```

```typescript
import { Histogram, HdrError } from "hdrhistogram";
```

## Core API Reference

### Error Set

| Symbol | Variants | Description |
| :--- | :--- | :--- |
| `HdrError` | `InitFailed`, `InvalidHandle`, `ValueOutOfRange` | Returned by `Histogram` fallible methods (`!T`) |

### `class Histogram` (RAII High-Level API)

Fields: `handle: *hdr_histogram!?`, `lowest_trackable: i64`, `highest_trackable: i64`, `sigfigs: i32`. Implements `__cp_deinit` to automatically call `hdr_close` on scope exit.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `init` | `init(*_, lowest: i64, highest: i64, sigfigs: i32): !void` | Initialize native histogram (`sigfigs` in `1..5`) |
| `open` | `open(*_, lowest: i64, highest: i64, sigfigs: i32): !void` | Close any existing handle and initialize a new histogram |
| `record` | `record(value: i64): !void` | Record a single sample; fails with `ValueOutOfRange` if out of bounds |
| `recordCount` / `recordN` | `recordCount(value: i64, count: i64): !void` | Record `value` repeated `count` times |
| `recordCorrected` / `recordCorrecting` | `recordCorrected(value: i64, expected_interval: i64): !void` | Record sample with coordinated omission correction |
| `recordCorrectingN` | `recordCorrectingN(value: i64, count: i64, expected_interval: i64): !void` | Record `count` samples with coordinated omission correction |
| `saturatingRecord` | `saturatingRecord(value: i64): !void` | Clamp `value` to `[lowest_trackable, highest_trackable]` before recording |
| `reset` | `reset(): !void` | Reset all recorded counts to zero |
| `memorySize` | `memorySize(): !usize` | Return underlying C histogram memory footprint in bytes |
| `min` / `max` | `min(): !i64` / `max(): !i64` | Minimum / maximum recorded value |
| `mean` | `mean(): !f64` | Arithmetic mean of recorded samples |
| `stddev` / `stdev` | `stddev(): !f64` | Standard deviation of recorded samples |
| `percentile` / `valueAtPercentile` | `percentile(p: f64): !i64` | Value at percentile `p` (`0.0..=100.0`) |
| `valueAtQuantile` | `valueAtQuantile(q: f64): !i64` | Value at quantile `q` (`0.0..=1.0`) |
| `len` / `isEmpty` | `len(): !u64` / `isEmpty(): !bool` | Total recorded sample count / empty check |
| `percentileBelow` / `quantileBelow` | `percentileBelow(value: i64): !f64` / `quantileBelow(value: i64): !f64` | Percentage (`0..100`) or quantile (`0..1`) of samples at or below `value` |
| `add` | `add(other: Histogram): !i64` | Merge `other` into this histogram; returns dropped count |
| `addCorrecting` | `addCorrecting(other: Histogram, expected_interval: i64): !i64` | Merge `other` with coordinated omission correction |
| `countAt` | `countAt(value: i64): !i64` | Recorded count in the bucket containing `value` |
| `equivalent` | `equivalent(a: i64, b: i64): !bool` | Check if `a` and `b` fall in the same precision bucket |
| `lowestEquivalent` / `medianEquivalent` | `lowestEquivalent(value: i64): !i64` / `medianEquivalent(value: i64): !i64` | Lowest / median value in `value`'s equivalence range |
| `nextNonEquivalent` / `sizeOfEquivalentRange` | `nextNonEquivalent(value: i64): !i64` / `sizeOfEquivalentRange(value: i64): !i64` | Next bucket start / bucket width for `value` |

### Low-Level C FFI & Helpers

Direct bindings are also exported when raw C interop is required: `hdr_histogram`, `hdr_init`, `hdr_close`, `hdr_reset`, `hdr_get_memory_size`, `hdr_record_value`, `hdr_record_values`, `hdr_record_corrected_value`, `hdr_record_corrected_values`, `hdr_add`, `hdr_add_while_correcting_for_coordinated_omission`, `hdr_min`, `hdr_max`, `hdr_mean`, `hdr_stddev`, `hdr_value_at_percentile`, `hdr_count_at_value`, `hdr_values_are_equivalent`, `hdr_lowest_equivalent_value`, `hdr_median_equivalent_value`, `hdr_next_non_equivalent_value`, `hdr_size_of_equivalent_value_range`, `hdrClampF64`, `hdrTotalCount`.

## Examples

### 1. Recording Latencies & Querying Percentiles

```typescript
import { Histogram } from "hdrhistogram";
import { print } from "cap:io";

fn main(): !void {
    let mut h = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    h.open(1, 3_600_000_000, 3)?;

    h.record(120)?;
    h.record(450)?;
    h.recordN(1_250, 5)?;
    h.saturatingRecord(9_999_999_999)?;

    let p50 = h.valueAtPercentile(50.0)?;
    let p99 = h.valueAtQuantile(0.99)?;
    let total = h.len()?;
    print("total={} p50={} p99={} mean={}\n", total, p50, p99, h.mean()?);
}
```

### 2. Coordinated Omission Correction & Merging Histograms

```typescript
import { Histogram } from "hdrhistogram";

fn mergeWorkerHistograms(): !i64 {
    let mut a = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    let mut b = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    a.open(1, 1_000_000, 3)?;
    b.open(1, 1_000_000, 3)?;

    a.recordCorrected(5_000, 1_000)?;
    b.recordCorrectingN(2_500, 4, 1_000)?;

    let dropped = a.add(b)?;
    return dropped;
}
```

## Differences from Upstream & CP Highlights

- **Deterministic RAII Cleanup**: `Histogram.__cp_deinit` releases the native C `hdr_histogram` allocation automatically on scope exit—no manual `hdr_close` needed unless using raw FFI.
- **Explicit Error Unions (`!T`)**: Every operation returns `HdrError` (`InitFailed`, `InvalidHandle`, `ValueOutOfRange`) rather than panicking or silently ignoring invalid handles.
- **Saturating & Quantile Conveniences**: Includes Rust-aligned helpers (`saturatingRecord`, `valueAtQuantile`, `quantileBelow`, `stdev`, `recordN`) directly on `Histogram`.
