[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# escape-html

Zero-heap single-pass HTML entity escaper and unescaper (&, <, >, ", ') in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:escape-html` ([https://github.com/component/escape-html](https://github.com/component/escape-html)) / `npm:entities` ([https://github.com/fb55/entities](https://github.com/fb55/entities))

| Metadata | Value |
| --- | --- |
| Package | `escape-html` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add escape-html
```

```cp
import { needsHtmlEscape, escapedHtmlLen, escapeHtml, unescapeHtml } from "escape-html";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `needsHtmlEscape` | `(input: string): bool` | Invokes `needsHtmlEscape` and returns `bool`. |
| `escapedHtmlLen` | `(input: string): usize` | Invokes `escapedHtmlLen` and returns `usize`. |
| `escapeHtml` | `(input: string, out: []u8): usize` | Invokes `escapeHtml` and returns `usize`. |
| `unescapeHtml` | `(input: string, out: []u8): usize` | Invokes `unescapeHtml` and returns `usize`. |

## Minimal Example

```cp
import { needsHtmlEscape } from "escape-html";

pub fn main() {
    let result = needsHtmlEscape("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
