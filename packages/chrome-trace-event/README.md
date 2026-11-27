[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# chrome-trace-event

Zero-heap Chrome DevTools / Perfetto Trace Event JSON serializer

| Metadata | Value |
| --- | --- |
| Package | `chrome-trace-event` |
| Version | `1.0.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add chrome-trace-event
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `TracePhase` | `Begin, End, Complete, Instant, Counter, Metadata` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TraceEvent` | `class` | Fields: `name: string, cat: string, ph: TracePhase, ts_us: u64, dur_us: u64, pid: u32, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `phaseCode` | `(ph: TracePhase): u8` | Invokes `phaseCode` and returns `u8`. |
| `ctevWriteByte` | `(out: []u8, pos: i64, b: u8): i64` | Invokes `ctevWriteByte` and returns `i64`. |
| `ctevWriteStr` | `(out: []u8, pos: i64, s: string): i64` | Invokes `ctevWriteStr` and returns `i64`. |
| `ctevWriteJsonStr` | `(out: []u8, pos: i64, s: string): i64` | Invokes `ctevWriteJsonStr` and returns `i64`. |
| `ctevWriteU64` | `(out: []u8, pos: i64, val: u64): i64` | Invokes `ctevWriteU64` and returns `i64`. |
| `formatTraceEvent` | `(ev: TraceEvent, out: []u8): usize` | Serializes/formats data via `formatTraceEvent` returning `usize`. |
| `formatTraceArray` | `(events: &[]TraceEvent, out: []u8): usize` | Serializes/formats data via `formatTraceArray` returning `usize`. |

## Minimal Example

```cp
import { ctevWriteStr } from "chrome-trace-event";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ctevWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
