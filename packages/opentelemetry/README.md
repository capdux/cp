[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# opentelemetry

Zero-heap W3C Trace Context (traceparent & tracestate) distributed tracing header parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:opentelemetry` ([https://github.com/open-telemetry/opentelemetry-rust](https://github.com/open-telemetry/opentelemetry-rust))（原库名 `opentelemetry` → CP 包名 `opentelemetry`）

| Metadata | Value |
| --- | --- |
| Package | `opentelemetry` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add opentelemetry
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TraceParent` | `class` | Fields: `version: u8, trace_id: [16]u8, parent_id: [8]u8, trace_flags: u8, valid: bool` · Methods: `isSampled, format` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseHexByteLower` | `(hi: u8, lo: u8): i32` | Parses input via `parseHexByteLower` and returns `i32`. |
| `invalidTraceParent` | `(): TraceParent` | Invokes `invalidTraceParent` and returns `TraceParent`. |
| `parseTraceParent` | `(header: &[]u8): TraceParent` | Parses input via `parseTraceParent` and returns `TraceParent`. |
| `makeChild` | `(parent: TraceParent, new_span_id: [8]u8): TraceParent` | Invokes `makeChild` and returns `TraceParent`. |
| `getTraceState` | `(header: &[]u8, vendor_key: &[]u8): &[]u8` | Queries/retrieves data via `getTraceState` returning `&[]u8`. |

## Minimal Example

```cp
import { parseHexByteLower } from "opentelemetry";

pub fn main() {
    let result = parseHexByteLower(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
