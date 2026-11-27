[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-width

Zero-allocation Unicode 15.1 East Asian Width & terminal display column width calculator with UTF-8 safe truncation for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-width` ([https://github.com/unicode-rs/unicode-width](https://github.com/unicode-rs/unicode-width)) / `npm:eastasianwidth` ([https://github.com/komagata/eastasianwidth](https://github.com/komagata/eastasianwidth)) / `npm:is-fullwidth-code-point` ([https://github.com/sindresorhus/is-fullwidth-code-point](https://github.com/sindresorhus/is-fullwidth-code-point)) / `npm:wcwidth` ([https://github.com/timoxley/wcwidth](https://github.com/timoxley/wcwidth))

| Metadata | Value |
| --- | --- |
| Package | `unicode-width` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-width
```

```cp
import {
    uwNextCodepoint,
    uwIsZeroWidth,
    uwIsWide,
    uwIsAmbiguous,
    charWidth,
    charWidthCjk,
    strWidth,
    strWidthCjk,
} from "unicode-width";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `uwNextCodepoint` | `(s: &[]u8, pos: usize, next_pos: &mut usize): u32` | Invokes `uwNextCodepoint` and returns `u32`. |
| `uwIsZeroWidth` | `(cp: u32): bool` | Invokes `uwIsZeroWidth` and returns `bool`. |
| `uwIsWide` | `(cp: u32): bool` | Invokes `uwIsWide` and returns `bool`. |
| `uwIsAmbiguous` | `(cp: u32): bool` | Invokes `uwIsAmbiguous` and returns `bool`. |
| `charWidth` | `(cp: u32): usize` | Invokes `charWidth` and returns `usize`. |
| `charWidthCjk` | `(cp: u32): usize` | Invokes `charWidthCjk` and returns `usize`. |
| `strWidth` | `(s: string): usize` | Invokes `strWidth` and returns `usize`. |
| `strWidthCjk` | `(s: string): usize` | Invokes `strWidthCjk` and returns `usize`. |
| `truncateToWidth` | `(s: string, max_cols: usize): usize` | Invokes `truncateToWidth` and returns `usize`. |

## Minimal Example

```cp
import { strWidth } from "unicode-width";

pub fn main() {
    let result = strWidth("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
