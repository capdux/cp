# kdbush@1.0.0 (Agent Quickref)

> Pure CP zero-allocation static 2D K-D Tree with balanced median-split build, hyperplane-pruned nearest neighbor, rectangle and radius queries (kdbush / kiddo parity) | License: MIT
> Import: `import { ... } from "kdbush";` | Full types: `src/index.d.cp`

## Functions
- `export fn buildKdTree(pts: []KdPt, nodes: []KdNode): i32;`
- `export fn nearestNeighbor(nodes: []KdNode, root: i32, qx: f64, qy: f64): NearestResult;`
- `export fn rangeQueryRect(nodes: []KdNode, root: i32, min_x: f64, min_y: f64, max_x: f64, max_y: f64, out_ids: []u32): usize;`
- `export fn radiusQuery(nodes: []KdNode, root: i32, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize;`
