# @cap/quadtree-2d

Zero-allocation pooled 2D QuadTree spatial index for CP with automatic 4-quadrant subdivision, rectangular range queries, and Euclidean radius queries.

## Features

- **Zero heap allocation**: caller provides fixed `[]QuadNode` and `[]QuadItem` pools; non-recursive DFS queries use an explicit stack.
- **Automatic subdivision**: splits leaf nodes into 4 quadrants when `max_leaf_items` is exceeded up to `max_depth`.
- **Spatial queries**: `queryRect` and `queryRadius`.

## Usage

```cp
import { Rect2, QuadNode, QuadItem, QuadTree } from "@cap/quadtree-2d";

let nodes: [64]QuadNode = undefined;
let items: [64]QuadItem = undefined;
let qt = QuadTree.empty();
* = qt.init(nodes[0..64], Rect2.new(0.0 as f64, 0.0 as f64, 100.0 as f64, 100.0 as f64), 4 as u32, 6 as u32);
* = qt.insert(nodes[0..64], items[0..64], 25.0 as f64, 30.0 as f64, 1 as u32);
```
