[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# prettier

Zero-heap JSON and CSS pretty-printer formatter (clean-room CP implementation inspired by npm prettier)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:prettier` ([https://github.com/prettier/prettier](https://github.com/prettier/prettier))（原库名 `prettier` → CP 包名 `prettier`）

| Metadata | Value |
| --- | --- |
| Package | `prettier` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add prettier
```

```cp
import {
    FormatOptions,
    pfIsSpace,
    pfPushByte,
    pfPushIndent,
    pfPeekNextNonSpace,
    formatJson,
    formatCss,
} from "prettier";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FormatOptions` | `class` | Fields: `tab_width: usize, use_tabs: bool, trailing_newline: bool` · Methods: `defaults` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pfIsSpace` | `(c: u8): bool` | Invokes `pfIsSpace` and returns `bool`. |
| `pfPushByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `pfPushByte` and returns `usize`. |
| `pfPushIndent` | `(out: []u8, pos: usize, depth: usize, opts: FormatOptions): usize` | Invokes `pfPushIndent` and returns `usize`. |
| `pfPeekNextNonSpace` | `(s: string, start: usize): u8` | Invokes `pfPeekNextNonSpace` and returns `u8`. |
| `formatJson` | `(input: string, opts: FormatOptions, out: []u8): usize` | Serializes/formats data via `formatJson` returning `usize`. |
| `formatCss` | `(input: string, opts: FormatOptions, out: []u8): usize` | Serializes/formats data via `formatCss` returning `usize`. |

## Minimal Example

```cp
import { formatJson } from "prettier";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatJson("example", /* opts: FormatOptions */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
