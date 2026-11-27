# hot-shots — AI Agent Quick Ref

Zero-heap StatsD and DogStatsD UDP metric packet encoder/decoder supporting Counter, Timing, Gauge, Set, sample rates, and tags. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StatsdMetric,
    isValidMetricName,
    isValidTags,
    appendByte,
    appendSlice,
    appendI64,
    formatMetric,
    parseSampleRateMilli,
} from "hot-shots";
```

## Core Types & Constants

```cp
class StatsdMetric { name_start: usize, name_len: usize, value: i64, kind: u8, /* +4 fields */ }
```

## API Signatures

```cp
fn isValidMetricName(name: &[]u8): bool;
fn isValidTags(tags: &[]u8): bool;
fn appendByte(dst: []u8, pos: usize, b: u8): i64;
fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;
fn appendI64(dst: []u8, pos: usize, v: i64): i64;
fn formatMetric(name: &[]u8, value: i64, kind: u8, sample_rate_milli: u16, tags: &[]u8, dst: []u8): i64;
fn parseSampleRateMilli(s: &[]u8): i32;
fn parseMetric(packet: &[]u8): StatsdMetric;
```

## Minimal Usage

```cp
import { isValidMetricName } from "hot-shots";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidMetricName(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hot-shots"`; install via `cap add hot-shots`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
