[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# dedent

Zero-heap common leading indentation detector and dedent engine for multiline strings (compatible with npm strip-indent, dedent & Rust indoc)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dedent` ([https://github.com/dmnd/dedent](https://github.com/dmnd/dedent))（原库名 `dedent` → CP 包名 `dedent`）

| Metadata | Value |
| --- | --- |
| Package | `dedent` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add dedent
```

```cp
import { isHorizSpace, minIndent, stripIndent, indentLines } from "dedent";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isHorizSpace` | `(c: u8): bool` | Checks predicate `isHorizSpace` and returns `bool`. |
| `minIndent` | `(input: string): usize` | Invokes `minIndent` and returns `usize`. |
| `stripIndent` | `(input: string, out: []u8): usize` | Invokes `stripIndent` and returns `usize`. |
| `indentLines` | `(input: string, spaces: usize, out: []u8): usize` | Invokes `indentLines` and returns `usize`. |

## Minimal Example

```cp
import { minIndent } from "dedent";

pub fn main() {
    let result = minIndent("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
