[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# stomp

Zero-allocation STOMP 1.2 messaging protocol frame encoder, decoder, and header escaper in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:stomp` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `stomp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add stomp
```

```cp
import {
    StompHeader,
    StompFrame,
    escapeHeader,
    unescapeHeader,
    encodeFrame,
    parseContentLength,
    decodeFrame,
    getHeader,
} from "stomp";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StompHeader` | `class` | Fields: `key: string, value: string` |
| `StompFrame` | `class` | Fields: `command: string, headers: [16]StompHeader, header_count: usize, body: string, consumed: usize` · Methods: `empty` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `escapeHeader` | `(input: string, out: []u8): usize` | Invokes `escapeHeader` and returns `usize`. |
| `unescapeHeader` | `(input: string, out: []u8): usize` | Invokes `unescapeHeader` and returns `usize`. |
| `encodeFrame` | `(command: string, headers: &[]StompHeader, body: string, out: []u8): usize` | Serializes/formats data via `encodeFrame` returning `usize`. |
| `parseContentLength` | `(val: string): i64` | Parses input via `parseContentLength` and returns `i64`. |
| `decodeFrame` | `(data: string, out_frame: &mut StompFrame): i32` | Parses input via `decodeFrame` and returns `i32`. |
| `getHeader` | `(frame: StompFrame, key: string): string` | Queries/retrieves data via `getHeader` returning `string`. |

## Minimal Example

```cp
import { parseContentLength } from "stomp";

pub fn main() {
    let result = parseContentLength("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
