# hdrhistogram — AI Agent Quick Ref

High Dynamic Range (HDR) Histogram backed by C `hdr_histogram.c` with RAII `Histogram` wrapper and explicit `!T` error unions.

## Import
```typescript
import { Histogram, HdrError } from "hdrhistogram";
```

## API Signatures
```typescript
export error HdrError { InitFailed, InvalidHandle, ValueOutOfRange }

export class Histogram {
    handle: *hdr_histogram!?, lowest_trackable: i64, highest_trackable: i64, sigfigs: i32,
    fn init(*_, lowest: i64, highest: i64, sigfigs: i32): !void;
    fn open(*_, lowest: i64, highest: i64, sigfigs: i32): !void;
    fn record(value: i64): !void;
    fn recordCount(value: i64, count: i64): !void;
    fn recordN(value: i64, count: i64): !void;
    fn recordCorrected(value: i64, expected_interval: i64): !void;
    fn recordCorrecting(value: i64, expected_interval: i64): !void;
    fn recordCorrectingN(value: i64, count: i64, expected_interval: i64): !void;
    fn saturatingRecord(value: i64): !void;
    fn reset(): !void;
    fn memorySize(): !usize;
    fn min(): !i64;
    fn max(): !i64;
    fn mean(): !f64;
    fn stddev(): !f64;
    fn stdev(): !f64;
    fn percentile(p: f64): !i64;          // p in 0.0..=100.0
    fn valueAtPercentile(p: f64): !i64;   // p in 0.0..=100.0
    fn valueAtQuantile(q: f64): !i64;     // q in 0.0..=1.0
    fn len(): !u64;
    fn isEmpty(): !bool;
    fn percentileBelow(value: i64): !f64;
    fn quantileBelow(value: i64): !f64;
    fn add(other: Histogram): !i64;
    fn addCorrecting(other: Histogram, expected_interval: i64): !i64;
    fn countAt(value: i64): !i64;
    fn equivalent(a: i64, b: i64): !bool;
    fn lowestEquivalent(value: i64): !i64;
    fn medianEquivalent(value: i64): !i64;
    fn nextNonEquivalent(value: i64): !i64;
    fn sizeOfEquivalentRange(value: i64): !i64;
    fn __cp_deinit(_: &mut Histogram);
}
// Raw C FFI also exported: hdr_init, hdr_close, hdr_record_value, hdr_value_at_percentile, hdrTotalCount, hdrClampF64, ...
```

## Minimal Example
```typescript
import { Histogram } from "hdrhistogram";

fn run(): !i64 {
    let mut h = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    h.open(1, 1_000_000, 3)?;
    h.record(100)?;
    h.recordN(500, 3)?;
    return h.valueAtPercentile(99.0)?;
}
```

## Pitfalls & Rules
- `Histogram` must be declared `let mut` before calling `init` or `open`; `open` safely closes any prior handle before re-initializing.
- Always call `init`/`open` first; calling methods with `handle == null` returns `HdrError.InvalidHandle`.
- `sigfigs` must be in `1..=5` and `lowest >= 1`, `highest >= 2 * lowest`; otherwise `HdrError.InitFailed`.
- Use `saturatingRecord` instead of `record` if inputs may exceed `[lowest_trackable, highest_trackable]`.
- Do NOT call `hdr_close` manually on `Histogram.handle` — `__cp_deinit` frees it automatically.
