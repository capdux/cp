# opentelemetry@1.0.0 (Agent Quickref)

> Zero-heap W3C Trace Context (traceparent & tracestate) distributed tracing header parser and formatter in pure CP | License: MIT
> Import: `import { ... } from "opentelemetry";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseHexByteLower(hi: u8, lo: u8): i32;`
- `export fn invalidTraceParent(): TraceParent;`
- `export fn parseTraceParent(header: &[]u8): TraceParent;`
- `export fn makeChild(parent: TraceParent, new_span_id: [8]u8): TraceParent;`
- `export fn getTraceState(header: &[]u8, vendor_key: &[]u8): &[]u8;`
