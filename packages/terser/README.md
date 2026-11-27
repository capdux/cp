[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# terser

Zero-allocation lexer-safe JS/TS/CSS code minifier (npm terser parity) preserving strings, templates, legal comments, keyword spacing, and unary operator separation for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:terser` ([https://github.com/terser/terser](https://github.com/terser/terser))（原库名 `terser` → CP 包名 `terser`）

| Metadata | Value |
| --- | --- |
| Package | `terser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add terser
```

```cp
import {
    MinifyOptions,
    MinifyStats,
    tmIsSpace,
    tmIsWord,
    tmNeedsJsSpace,
    tmContainsSubstr,
    tmIsLegalComment,
    tmIsCssPunct,
} from "terser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MinifyOptions` | `class` | Fields: `strip_comments: bool, preserve_legal_comments: bool, collapse_whitespace: bool, keep_newlines: bool` · Methods: `defaults` |
| `MinifyStats` | `class` | Fields: `input_len: usize, output_len: usize, comments_removed: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `tmIsSpace` | `(ch: u8): bool` | Invokes `tmIsSpace` and returns `bool`. |
| `tmIsWord` | `(ch: u8): bool` | Invokes `tmIsWord` and returns `bool`. |
| `tmNeedsJsSpace` | `(prev: u8, next: u8): bool` | Invokes `tmNeedsJsSpace` and returns `bool`. |
| `tmContainsSubstr` | `(s: string, needle: string): bool` | Invokes `tmContainsSubstr` and returns `bool`. |
| `tmIsLegalComment` | `(comment: string): bool` | Invokes `tmIsLegalComment` and returns `bool`. |
| `tmIsCssPunct` | `(ch: u8): bool` | Invokes `tmIsCssPunct` and returns `bool`. |
| `minifyJs` | `(input: string, opts: MinifyOptions, out: []u8): MinifyStats` | Invokes `minifyJs` and returns `MinifyStats`. |
| `minifyCss` | `(input: string, out: []u8): MinifyStats` | Invokes `minifyCss` and returns `MinifyStats`. |

## Minimal Example

```cp
import { tmContainsSubstr } from "terser";

pub fn main() {
    let result = tmContainsSubstr("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
