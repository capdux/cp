# server-timing — AI Agent Quick Ref

Zero-allocation W3C Server-Timing HTTP response header parser and formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TimingMetric,
    ParsedFloat,
    parseFloatSimple,
    parseSingleMetric,
    parseHeader,
    findMetric,
    totalDurationMs,
    isTokenChar,
} from "server-timing";
```

## Core Types & Constants

```cp
class TimingMetric { name: string, has_dur: bool, dur_ms: f64, desc: string }
class ParsedFloat { val: f64, ok: bool }
```

## API Signatures

```cp
fn parseFloatSimple(s: string): ParsedFloat;
fn parseSingleMetric(seg: string, out: &mut TimingMetric): bool;
fn parseHeader(header: string, out: []TimingMetric): usize;
fn findMetric(header: string, name: string, out: &mut TimingMetric): bool;
fn totalDurationMs(header: string): f64;
fn isTokenChar(b: u8): bool;
fn descNeedsQuotes(s: string): bool;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn pushU64(dst: []u8, pos: usize, val: u64): i64;
fn pushDurMs(dst: []u8, pos: usize, dur_ms: f64): i64;
fn formatMetric(metric: TimingMetric, out: []u8): usize;
fn formatHeader(metrics: &[]TimingMetric, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseFloatSimple } from "server-timing";

pub fn main() {
    let result = parseFloatSimple("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"server-timing"`; install via `cap add server-timing`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
