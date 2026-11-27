# hdrhistogram — AI Agent Quick Ref

- **Import**: `import { Histogram, HdrError } from "hdrhistogram";`
- **Origin**: crates.io `hdrhistogram` (v1.0.0, BSD-2-Clause, native C `hdr_histogram.c` + RAII)
- **Full Types**: `src/index.d.cp`

## Errors
```cp
error HdrError { InitFailed, InvalidHandle, ValueOutOfRange }
```

## `Histogram` Class (RAII via `__cp_deinit`)
- **Lifecycle**:
  - `init(lowest: i64, highest: i64, sigfigs: i32): !void` (alias: `open(...)`)
  - `reset(): !void` | `memorySize(): !usize`
- **Recording**:
  - `record(value: i64): !void` | `saturatingRecord(value: i64): !void`
  - `recordCount(value: i64, count: i64): !void` (alias: `recordN`)
  - `recordCorrected(value: i64, expected_interval: i64): !void` (alias: `recordCorrecting`)
  - `recordCorrectingN(value: i64, count: i64, expected_interval: i64): !void`
- **Statistics & Percentiles**:
  - `len(): !u64` | `isEmpty(): !bool` | `min(): !i64` | `max(): !i64`
  - `mean(): !f64` | `stddev(): !f64` (alias: `stdev`)
  - `valueAtPercentile(p: f64): !i64` (alias: `percentile`, `p` in `0.0..100.0`)
  - `valueAtQuantile(q: f64): !i64` (`q` in `0.0..1.0`)
  - `percentileBelow(value: i64): !f64` | `quantileBelow(value: i64): !f64`
  - `countAt(value: i64): !i64`
- **Merge & Bucket Equivalence**:
  - `add(other: Histogram): !i64` | `addCorrecting(other: Histogram, expected_interval: i64): !i64`
  - `equivalent(a: i64, b: i64): !bool` | `lowestEquivalent(value: i64): !i64`
  - `medianEquivalent(value: i64): !i64` | `nextNonEquivalent(value: i64): !i64`
  - `sizeOfEquivalentRange(value: i64): !i64`

## Helper Functions
- `hdrClampF64(v: f64, lo: f64, hi: f64): f64`
- `hdrTotalCount(h: *&hdr_histogram!): i64`
- Low-level C bindings `hdr_init`, `hdr_close`, `hdr_record_value`, etc. are also exported.

## Gotchas
1. Always call `h.init(lowest, highest, sigfigs)?` (or `h.open(...)`) before recording; `sigfigs` must be `1..5` and `lowest >= 1`, `highest >= 2 * lowest`.
2. `valueAtPercentile` takes `0.0..100.0`, while `valueAtQuantile` takes `0.0..1.0` — do not mix them up.
3. `record()` returns `HdrError.ValueOutOfRange` for values outside `[lowest, highest]`; use `saturatingRecord()` to clamp instead.
4. Do NOT manually call `hdr_close` on `Histogram.handle` — `__cp_deinit` frees it automatically.
