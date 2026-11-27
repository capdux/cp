[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# eventsource-parser

Zero-allocation W3C Server-Sent Events (text/event-stream) incremental streaming parser for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:eventsource-parser` ([https://github.com/rexxars/eventsource-parser](https://github.com/rexxars/eventsource-parser))（原库名 `eventsource-parser` → CP 包名 `eventsource-parser`）

| Metadata | Value |
| --- | --- |
| Package | `eventsource-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add eventsource-parser
```

```cp
import { SseParser, sseBytesEq } from "eventsource-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SseParser` | `class` | Fields: `data_buf: [2048]u8, data_len: usize, event_buf: [128]u8, event_len: usize, id_buf: [128]u8, id_len: usize, +5 more` · Methods: `init, processLine, pushByte, eventType, eventData, lastEventId, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sseBytesEq` | `(a: &[]u8, b: &[]u8): bool` | Invokes `sseBytesEq` and returns `bool`. |

## Minimal Example

```cp
import { sseBytesEq } from "eventsource-parser";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = sseBytesEq(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
