[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cssstyle

Zero-allocation CSS inline style declaration parser, query, mutator, and serializer (npm cssstyle parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`cssstyle`](https://github.com/jsdom/cssstyle)（`MIT`）（包名 `cssstyle`）。

| Metadata | Value |
| --- | --- |
| Package | `cssstyle` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cssstyle
```

```cp
import {
    CssProp,
    CstParsedValue,
    CssStyleDecl,
    cstSkipSpaceAndComments,
    cstStripTrailingComments,
    cstExtractImportant,
} from "cssstyle";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CssProp` | `class` | Fields: `name: string, value: string, important: bool` |
| `CstParsedValue` | `class` | Fields: `value: string, important: bool` |
| `CssStyleDecl` | `class` | Fields: `props: [64]CssProp, count: usize` · Methods: `new, len, parse, getPropertyValue, getPropertyPriority, setProperty, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cstSkipSpaceAndComments` | `(s: string, pos: usize): usize` | Invokes `cstSkipSpaceAndComments` and returns `usize`. |
| `cstStripTrailingComments` | `(s: string): string` | Invokes `cstStripTrailingComments` and returns `string`. |
| `cstExtractImportant` | `(raw: string): CstParsedValue` | Invokes `cstExtractImportant` and returns `CstParsedValue`. |

## Minimal Example

```cp
import { cstSkipSpaceAndComments } from "cssstyle";

pub fn main() {
    let result = cstSkipSpaceAndComments("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
