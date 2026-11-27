[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ndjson

Zero-heap NDJSON / JSON Lines streaming incremental line splitter, scanner, and writer with max-line overflow protection.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ndjson` ([https://github.com/ndjson/ndjson.js](https://github.com/ndjson/ndjson.js))

| Metadata | Value |
| --- | --- |
| Package | `ndjson` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ndjson
```

```cp
import {
    NdjsonLine,
    NdjsonSplitter,
    isBlankSlice,
    nextLine,
    countLines,
    writeLine,
} from "ndjson";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `NdjsonLine` | `class` | Fields: `start: usize, len: usize, found: bool` |
| `NdjsonSplitter` | `class` | Fields: `buf: [1024]u8, pos: usize, ready_len: usize, overflowed: bool` · Methods: `init, reset, pushByte, currentLine, flush` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isBlankSlice` | `(s: &[]u8): bool` | Checks predicate `isBlankSlice` and returns `bool`. |
| `nextLine` | `(src: &[]u8, start_pos: usize, out_next_pos: &mut usize): NdjsonLine` | Invokes `nextLine` and returns `NdjsonLine`. |
| `countLines` | `(src: &[]u8): usize` | Invokes `countLines` and returns `usize`. |
| `writeLine` | `(json_record: &[]u8, dst: []u8): i64` | Invokes `writeLine` and returns `i64`. |

## Minimal Example

```cp
import { isBlankSlice } from "ndjson";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isBlankSlice(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
