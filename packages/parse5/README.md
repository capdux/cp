[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parse5

Zero-heap WHATWG HTML5 streaming tokenizer, attribute extractor, and text stripper (clean-room port of npm parse5)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse5` ([https://github.com/inikulin/parse5](https://github.com/inikulin/parse5))

| Metadata | Value |
| --- | --- |
| Package | `parse5` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parse5
```

```cp
import {
    HtmlToken,
    HtmlAttr,
    HtmlTokenizer,
    p5IsWs,
    p5IsTagNameChar,
    p5Trim,
    p5FindSubstr,
    isVoidElement,
} from "parse5";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HtmlToken` | `class` | Fields: `kind: u8, name: string, raw_attrs: string, text: string, start: usize, end: usize` |
| `HtmlAttr` | `class` | Fields: `name: string, value: string` |
| `HtmlTokenizer` | `class` | Fields: `src: string, pos: usize` · Methods: `new, next` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `p5IsWs` | `(b: u8): bool` | Invokes `p5IsWs` and returns `bool`. |
| `p5IsTagNameChar` | `(b: u8): bool` | Invokes `p5IsTagNameChar` and returns `bool`. |
| `p5Trim` | `(s: string): string` | Invokes `p5Trim` and returns `string`. |
| `p5FindSubstr` | `(s: string, start_pos: usize, needle: string): usize` | Invokes `p5FindSubstr` and returns `usize`. |
| `isVoidElement` | `(tag: string): bool` | Checks predicate `isVoidElement` and returns `bool`. |
| `parseAttributes` | `(raw_attrs: string, out_attrs: []HtmlAttr): usize` | Parses input via `parseAttributes` and returns `usize`. |
| `extractTextContent` | `(html: string, out: []u8): usize` | Invokes `extractTextContent` and returns `usize`. |

## Minimal Example

```cp
import { isVoidElement } from "parse5";

pub fn main() {
    let result = isVoidElement("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
