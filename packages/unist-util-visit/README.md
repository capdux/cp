[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unist-util-visit

Zero-heap unist AST tree pre-order/post-order visitor and node matcher (covers unist-util-visit-parents and unist-util-is)

| Metadata | Value |
| --- | --- |
| Package | `unist-util-visit` |
| Version | `5.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unist-util-visit
```

```cp
import { UnistNode, UnistTree, uuvStrEq, newUnistTree } from "unist-util-visit";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UnistNode` | `class` | Fields: `kind: string, value: string, parent: u32, first_child: u32, next_sibling: u32` |
| `UnistTree` | `class` | Fields: `nodes: [128]UnistNode, len: usize` · Methods: `addNode, isNode, visitPreOrder, getAncestors` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `uuvStrEq` | `(a: string, b: string): bool` | Invokes `uuvStrEq` and returns `bool`. |
| `newUnistTree` | `(): UnistTree` | Constructs/initializes an instance via `newUnistTree`. |

## Minimal Example

```cp
import { uuvStrEq } from "unist-util-visit";

pub fn main() {
    let result = uuvStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
