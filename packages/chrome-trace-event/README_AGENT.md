# chrome-trace-event — AI Agent Quick Ref

Zero-heap Chrome DevTools / Perfetto Trace Event JSON serializer (`v1.0.4`, `MIT`)

## Import

```cp
import {
    TracePhase,
    TraceEvent,
    phaseCode,
    ctevWriteByte,
    ctevWriteStr,
    ctevWriteJsonStr,
    ctevWriteU64,
    formatTraceEvent,
} from "chrome-trace-event";
```

## Core Types & Constants

```cp
enum TracePhase { Begin, End, Complete, Instant, Counter, Metadata }
class TraceEvent { name: string, cat: string, ph: TracePhase, ts_us: u64, /* +3 fields */ }
```

## API Signatures

```cp
fn phaseCode(ph: TracePhase): u8;
fn ctevWriteByte(out: []u8, pos: i64, b: u8): i64;
fn ctevWriteStr(out: []u8, pos: i64, s: string): i64;
fn ctevWriteJsonStr(out: []u8, pos: i64, s: string): i64;
fn ctevWriteU64(out: []u8, pos: i64, val: u64): i64;
fn formatTraceEvent(ev: TraceEvent, out: []u8): usize;
fn formatTraceArray(events: &[]TraceEvent, out: []u8): usize;
```

## Minimal Usage

```cp
import { ctevWriteStr } from "chrome-trace-event";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ctevWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"chrome-trace-event"`; install via `cap add chrome-trace-event`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
