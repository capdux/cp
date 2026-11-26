# @cap/kdtree-2d

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kdbush` ([https://github.com/mourner/kdbush](https://github.com/mourner/kdbush))（原库名 `kdbush` → CP 包名 `@cap/kdtree-2d`）

Pure CP, zero-allocation static 2D K-D Tree spatial index with balanced in-place median-split construction and iterative explicit-stack queries (parity with npm `kdbush` / Rust `kiddo`).

## Features

- **Balanced Static Build**: `buildKdTree(pts: []KdPt, nodes: []KdNode): i32` builds a balanced alternating X/Y median-split tree in-place without recursion or heap allocation.
- **Exact Nearest Neighbor**: `nearestNeighbor(nodes, root, qx, qy): NearestResult` with hyperplane distance pruning and backtracking.
- **Range Queries**: `rangeQueryRect` (axis-aligned bounding box) and `radiusQuery` (exact Euclidean circle).

## License

MIT
