# `cap:cli_tree` — Zero-Alloc CLI Tree & Dependency Graph Renderer

Zero-allocation pure-CP renderer for hierarchical directory trees and package dependency trees, supporting Unicode (`├──`/`└──`/`│   `), ASCII (`|--`/`+--`/`|   `), and Rounded (`├──`/`╰──`/`│   `) connector styles.

## Types

### `TreeStyle`

```cp
pub enum TreeStyle {
    unicode,
    ascii,
    rounded,
}
```

### `TreePrefix`

```cp
pub struct TreePrefix {
    depth: u8;
    last_mask: u64;
}
```

Stack-resident 64-bit bitmask tracking whether each ancestor level (`0..63`) was the last sibling:
- `TreePrefix.init(): TreePrefix` — Initializes an empty root-level prefix (`depth = 0`).
- `push(is_last: bool): TreePrefix?` — Pushes an ancestor level (returns `null` once depth reaches 64).
- `pop(): TreePrefix` — Pops the deepest ancestor level (no-op at depth 0).
- `isAncestorLast(level: u8): bool` — Queries whether ancestor `level` was a last sibling.
- `formatPrefix(style: TreeStyle, is_last: bool, out: []u8): ([]const u8)?` — Formats ancestor guide columns plus the current branch connector into `out`. Returns `null` if `out` is too small.

### `TreeNode`

```cp
pub struct TreeNode {
    depth: u8;
    label: []const u8;
}
```

## Exports

- `renderFlatTree(nodes: []const TreeNode, style: TreeStyle, out: []u8): ([]const u8)?` — Renders a flat pre-order `(depth, label)` list into a full tree string by automatically looking ahead to detect last siblings at every depth. Supports depths `0..=64` with zero heap allocation; returns `null` if depth > 64 or `out` is too small.

## Example

```cp
import { TreeStyle, TreeNode, renderFlatTree } from "cap:cli_tree";

const nodes: [4]TreeNode = [
    TreeNode { depth: 0, label: "my-app@1.0.0" },
    TreeNode { depth: 1, label: "cap-http@0.2.0" },
    TreeNode { depth: 2, label: "cap-net@0.1.0" },
    TreeNode { depth: 1, label: "cap-json@0.3.0" },
];

let buf: [256]u8 = [0; 256];
if (tree := renderFlatTree(nodes[0..], TreeStyle.unicode, buf[0..])) {
    // my-app@1.0.0
    // ├── cap-http@0.2.0
    // │   └── cap-net@0.1.0
    // └── cap-json@0.3.0
}
```
