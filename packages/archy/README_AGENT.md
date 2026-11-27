# archy — AI Agent Quick Ref

Zero-allocation Unicode & ASCII dependency/directory tree renderer (archy / treeify / Rust ptree parity) for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum TreeStyle { Unicode, Rounded, Ascii }
class TreeNode { label: string, parent: i32, first_child: i32, last_child: i32, /* +1 fields */ }
class TreeBuilder { nodes: [MAX_NODES]TreeNode, count: usize } // methods: init(...) +2
```

## API Signatures

```cp
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn branchStr(style: TreeStyle, is_last: bool): string;
fn guideStr(style: TreeStyle, has_more: bool): string;
fn renderNodeRec(tree: &mut TreeBuilder, node_idx: usize, depth: usize, ancestors_have_next: &mut [MAX_DEPTH]bool, style: TreeStyle, out: []u8, pos_in: usize): usize;
fn renderTreeWith(tree: &mut TreeBuilder, style: TreeStyle, out: []u8): string;
fn renderTree(tree: &mut TreeBuilder, out: []u8): string;
```

## Minimal Usage

```cp
import { appendStr } from "archy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"archy"`; install via `cap add archy`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
