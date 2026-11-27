[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pretty-format

Zero-heap pure CP structured value pretty-printer with indentation, compact mode, and max-depth protection (clean-room MIT implementation of npm pretty-format)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-format` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest))

| Metadata | Value |
| --- | --- |
| Package | `pretty-format` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pretty-format
```

```cp
import {
    PrettyOptions,
    PrettyWriter,
    pfmtWriteByte,
    pfmtWriteSpaces,
    pfmtWriteF64,
    pfmtWriteStringLiteral,
    formatStringLiteral,
    formatPrimitiveInt,
} from "pretty-format";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PrettyOptions` | `class` | Fields: `indent: usize, max_depth: usize, min: bool, escape_string: bool, print_basic_prototype: bool` · Methods: `defaults, compact` |
| `PrettyWriter` | `class` | Fields: `opts: PrettyOptions, buf: [2048]u8, len: usize, depth: usize, item_counts: [32]usize, truncated_depth: usize` · Methods: `new, defaultWriter, clear, isTruncated, writeNull, writeUndefined, +12 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pfmtWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `pfmtWriteByte` and returns `usize`. |
| `pfmtWriteSpaces` | `(out: []u8, pos: usize, count: usize): usize` | Invokes `pfmtWriteSpaces` and returns `usize`. |
| `pfmtWriteF64` | `(out: []u8, pos: usize, val: f64): usize` | Invokes `pfmtWriteF64` and returns `usize`. |
| `pfmtWriteStringLiteral` | `(out: []u8, pos: usize, s: string, escape_string: bool): usize` | Invokes `pfmtWriteStringLiteral` and returns `usize`. |
| `formatStringLiteral` | `(s: string, escape: bool, out: []u8): usize` | Serializes/formats data via `formatStringLiteral` returning `usize`. |
| `formatPrimitiveInt` | `(v: i64, out: []u8): usize` | Serializes/formats data via `formatPrimitiveInt` returning `usize`. |

## Minimal Example

```cp
import { formatStringLiteral } from "pretty-format";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatStringLiteral("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
