[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# debugid

Zero-heap Sentry/Breakpad/PDB DebugId (128-bit UUID + appendix age) parser and formatter

| Metadata | Value |
| --- | --- |
| Package | `debugid` |
| Version | `0.8.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add debugid
```

```cp
import { DebugId, parseDebugId, formatBreakpad, formatCanonical } from "debugid";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DebugId` | `class` | Fields: `bytes: [16]u8, appendix: u32, valid: bool` · Methods: `isNil` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseDebugId` | `(s: string): DebugId` | Parses input via `parseDebugId` and returns `DebugId`. |
| `formatBreakpad` | `(id: DebugId, out: []u8): usize` | Serializes/formats data via `formatBreakpad` returning `usize`. |
| `formatCanonical` | `(id: DebugId, out: []u8): usize` | Serializes/formats data via `formatCanonical` returning `usize`. |

## Minimal Example

```cp
import { parseDebugId } from "debugid";

pub fn main() {
    let result = parseDebugId("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
