# opentelemetry — AI Agent Quick Ref

Zero-heap W3C Trace Context (traceparent & tracestate) distributed tracing header parser and formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TraceParent,
    parseHexByteLower,
    invalidTraceParent,
    parseTraceParent,
    makeChild,
    getTraceState,
} from "opentelemetry";
```

## Core Types & Constants

```cp
class TraceParent { version: u8, trace_id: [16]u8, parent_id: [8]u8, trace_flags: u8, /* +1 fields */ } // methods: isSampled(...) +1
```

## API Signatures

```cp
fn parseHexByteLower(hi: u8, lo: u8): i32;
fn invalidTraceParent(): TraceParent;
fn parseTraceParent(header: &[]u8): TraceParent;
fn makeChild(parent: TraceParent, new_span_id: [8]u8): TraceParent;
fn getTraceState(header: &[]u8, vendor_key: &[]u8): &[]u8;
```

## Minimal Usage

```cp
import { parseHexByteLower } from "opentelemetry";

pub fn main() {
    let result = parseHexByteLower(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"opentelemetry"`; install via `cap add opentelemetry`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
