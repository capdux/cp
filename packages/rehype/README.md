[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rehype

Zero-allocation HAST (Hypertext Abstract Syntax Tree) builder & HTML5 serializer (npm rehype/hast parity) composing escape-html for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rehype` ([https://github.com/rehypejs/rehype](https://github.com/rehypejs/rehype))（原库名 `rehype` → CP 包名 `rehype`）

| Metadata | Value |
| --- | --- |
| Package | `rehype` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rehype
```

```cp
import {
    HastAttr,
    HastNode,
    HastTree,
    rhWriteByte,
    isVoidElement,
    rhMakeNode,
    rhSerializeHtml,
    rhExtractText,
} from "rehype";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HastAttr` | `class` | Fields: `name: [48]u8, name_len: usize, value: [192]u8, value_len: usize, next_attr: i32` |
| `HastNode` | `class` | Fields: `kind: u8, text: [256]u8, text_len: usize, first_child: i32, last_child: i32, next_sibling: i32, +2 more` |
| `HastTree` | `class` | Fields: `nodes: [64]HastNode, node_len: usize, attrs: [64]HastAttr, attr_len: usize` · Methods: `new, addElement, addText, addComment, addDoctype, setAttr, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rhWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `rhWriteByte` and returns `usize`. |
| `isVoidElement` | `(tag: string): bool` | Checks predicate `isVoidElement` and returns `bool`. |
| `rhMakeNode` | `(kind: u8, text: string): HastNode` | Invokes `rhMakeNode` and returns `HastNode`. |
| `rhSerializeHtml` | `(nodes: &[]HastNode, attrs: &[]HastAttr, node_id: usize, out: []u8, pos: usize, depth: usize): usize` | Invokes `rhSerializeHtml` and returns `usize`. |
| `rhExtractText` | `(nodes: &[]HastNode, node_id: usize, out: []u8, pos: usize, depth: usize): usize` | Invokes `rhExtractText` and returns `usize`. |

## Minimal Example

```cp
import { isVoidElement } from "rehype";

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
