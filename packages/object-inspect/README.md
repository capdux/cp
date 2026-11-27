[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# object-inspect

Zero-heap debug value inspector and string/bytes/number pretty-formatter with escape and truncation support

| Metadata | Value |
| --- | --- |
| Package | `object-inspect` |
| Version | `1.13.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add object-inspect
```

```cp
import {
    InspectOptions,
    defaultInspectOptions,
    inspectString,
    inspectBool,
    inspectInt,
    inspectBytes,
    inspectStringArray,
} from "object-inspect";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `InspectOptions` | `class` | Fields: `quote_style_double: bool, max_string_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `defaultInspectOptions` | `(): InspectOptions` | Invokes `defaultInspectOptions` and returns `InspectOptions`. |
| `inspectString` | `(s: string, opts: InspectOptions, out: []u8): usize` | Invokes `inspectString` and returns `usize`. |
| `inspectBool` | `(v: bool, out: []u8): usize` | Invokes `inspectBool` and returns `usize`. |
| `inspectInt` | `(val: i64, out: []u8): usize` | Invokes `inspectInt` and returns `usize`. |
| `inspectBytes` | `(data: &[]u8, max_bytes: usize, out: []u8): usize` | Invokes `inspectBytes` and returns `usize`. |
| `inspectStringArray` | `(items: &[]string, opts: InspectOptions, out: []u8): usize` | Invokes `inspectStringArray` and returns `usize`. |

## Minimal Example

```cp
import { inspectString } from "object-inspect";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = inspectString("example", /* opts: InspectOptions */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
