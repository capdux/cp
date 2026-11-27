[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# estree-walker

Zero-allocation index-pooled ESTree AST arena and non-recursive DFS walker with skip, remove, and replace for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:estree-walker` ([https://github.com/Rich-Harris/estree-walker](https://github.com/Rich-Harris/estree-walker))

| Metadata | Value |
| --- | --- |
| Package | `estree-walker` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add estree-walker
```

```cp
import {
    EstreeNodeKind,
    WalkPhase,
    WalkEvent,
    EstreeNode,
    EstwFrame,
    EstreeArena,
    EstreeWalker,
    estwValidId,
} from "estree-walker";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `EstreeNodeKind` | `Program, Identifier, Literal, BinaryExpression, CallExpression, FunctionDeclaration, BlockStatement, ReturnStatement ...` |
| `enum` | `WalkPhase` | `Enter, Leave` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WalkEvent` | `class` | Fields: `phase: WalkPhase, node_id: i32, parent_id: i32, depth: usize` |
| `EstreeNode` | `class` | Fields: `kind: EstreeNodeKind, name: string, first_child: i32, next_sibling: i32, parent: i32, removed: bool` |
| `EstwFrame` | `class` | Fields: `node_id: i32, next_child: i32, entered: bool, skipped: bool` |
| `EstreeArena` | `class` | Fields: `count: usize, nodes: [64]EstreeNode` · Methods: `init, addNode, getNode, appendChild, removeNode, replaceNode` |
| `EstreeWalker` | `class` | Fields: `sp: usize, frames: [64]EstwFrame` · Methods: `init, skip, nextEvent` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `estwValidId` | `(id: i32, count: usize): bool` | Invokes `estwValidId` and returns `bool`. |

## Minimal Example

```cp
import { estwValidId } from "estree-walker";

pub fn main() {
    let result = estwValidId(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
