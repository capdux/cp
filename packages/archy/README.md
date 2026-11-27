[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# archy

Zero-allocation Unicode & ASCII dependency/directory tree renderer (archy / treeify / Rust ptree parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:archy` ([https://github.com/substack/node-archy](https://github.com/substack/node-archy))（原库名 `archy` → CP 包名 `archy`）

| Metadata | Value |
| --- | --- |
| Package | `archy` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add archy
```

```cp
import {
    TreeStyle,
    TreeNode,
    TreeBuilder,
    appendStr,
    appendByte,
    branchStr,
    guideStr,
    renderNodeRec,
} from "archy";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `TreeStyle` | `Unicode, Rounded, Ascii` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TreeNode` | `class` | Fields: `label: string, parent: i32, first_child: i32, last_child: i32, next_sibling: i32` |
| `TreeBuilder` | `class` | Fields: `nodes: [MAX_NODES]TreeNode, count: usize` · Methods: `init, addChild, nodeCount` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `branchStr` | `(style: TreeStyle, is_last: bool): string` | Invokes `branchStr` and returns `string`. |
| `guideStr` | `(style: TreeStyle, has_more: bool): string` | Invokes `guideStr` and returns `string`. |
| `renderNodeRec` | `(tree: &mut TreeBuilder, node_idx: usize, depth: usize, ancestors_have_next: &mut [MAX_DEPTH]bool, style: TreeStyle, out: []u8, pos_in: usize): usize` | Invokes `renderNodeRec` and returns `usize`. |
| `renderTreeWith` | `(tree: &mut TreeBuilder, style: TreeStyle, out: []u8): string` | Invokes `renderTreeWith` and returns `string`. |
| `renderTree` | `(tree: &mut TreeBuilder, out: []u8): string` | Invokes `renderTree` and returns `string`. |

## Minimal Example

```cp
import { appendStr } from "archy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
