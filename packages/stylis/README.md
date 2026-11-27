[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# stylis

Zero-heap CSS minifier and nested selector preprocessor in pure CP

| Metadata | Value |
| --- | --- |
| Package | `stylis` |
| Version | `4.3.6` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add stylis
```

```cp
import {
    stylIsWs,
    stylPushByte,
    stylIsStripAfter,
    stylIsStripBefore,
    minifyCss,
    stylExpandSingle,
    expandNestedSelector,
    stylSkipCommentOrQuote,
} from "stylis";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `stylIsWs` | `(c: u8): bool` | Invokes `stylIsWs` and returns `bool`. |
| `stylPushByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `stylPushByte` and returns `usize`. |
| `stylIsStripAfter` | `(c: u8): bool` | Invokes `stylIsStripAfter` and returns `bool`. |
| `stylIsStripBefore` | `(c: u8): bool` | Invokes `stylIsStripBefore` and returns `bool`. |
| `minifyCss` | `(css: string, out: []u8): usize` | Invokes `minifyCss` and returns `usize`. |
| `stylExpandSingle` | `(parent: string, child: string, out: []u8, start_pos: usize): usize` | Invokes `stylExpandSingle` and returns `usize`. |
| `expandNestedSelector` | `(parent: string, child: string, out: []u8): usize` | Invokes `expandNestedSelector` and returns `usize`. |
| `stylSkipCommentOrQuote` | `(s: string, idx: usize): usize` | Invokes `stylSkipCommentOrQuote` and returns `usize`. |
| `compileNestedCss` | `(parent_sel: string, body_css: string, out: []u8): usize` | Invokes `compileNestedCss` and returns `usize`. |

## Minimal Example

```cp
import { minifyCss } from "stylis";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = minifyCss("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
