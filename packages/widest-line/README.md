[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# widest-line

Zero-allocation widest visual terminal line width calculator (npm widest-line parity) composing string-width for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:widest-line` ([https://github.com/sindresorhus/widest-line](https://github.com/sindresorhus/widest-line))

| Metadata | Value |
| --- | --- |
| Package | `widest-line` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add widest-line
```

```cp
import {
    wlTrimCr,
    wlLineWidthWithTab,
    countLines,
    widestLine,
    widestLineWithOptions,
    lineWidths,
    lineWidthsWithOptions,
    widestLineWithTab,
} from "widest-line";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wlTrimCr` | `(line: string): string` | Invokes `wlTrimCr` and returns `string`. |
| `wlLineWidthWithTab` | `(line: string, tab_width: usize): usize` | Invokes `wlLineWidthWithTab` and returns `usize`. |
| `countLines` | `(s: string): usize` | Invokes `countLines` and returns `usize`. |
| `widestLine` | `(s: string): usize` | Invokes `widestLine` and returns `usize`. |
| `widestLineWithOptions` | `(s: string, opts: StringWidthOptions): usize` | Invokes `widestLineWithOptions` and returns `usize`. |
| `lineWidths` | `(s: string, out: []usize): usize` | Invokes `lineWidths` and returns `usize`. |
| `lineWidthsWithOptions` | `(s: string, opts: StringWidthOptions, out: []usize): usize` | Invokes `lineWidthsWithOptions` and returns `usize`. |
| `widestLineWithTab` | `(s: string, tab_width: usize): usize` | Invokes `widestLineWithTab` and returns `usize`. |

## Minimal Example

```cp
import { wlTrimCr } from "widest-line";

pub fn main() {
    let result = wlTrimCr("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
