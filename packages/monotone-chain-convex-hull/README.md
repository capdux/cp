[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `monotone-chain-convex-hull`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:monotone-chain-convex-hull` ([https://github.com/mikolalysenko/monotone-chain-convex-hull](https://github.com/mikolalysenko/monotone-chain-convex-hull))（原库名 `monotone-chain-convex-hull` → CP 包名 `monotone-chain-convex-hull`）

Zero-heap 2D Andrew's monotone chain convex hull algorithm with area and perimeter calculation in pure CP.

## Features

- **100% Zero Heap Allocation**: Uses caller-provided `work_pts: []Pt2` and `out_hull: []Pt2` slices.
- **Andrew's Monotone Chain**: Outputs counter-clockwise (CCW) hull vertices, automatically deduplicating coincident points and skipping interior and collinear intermediate points.
- **Metrics Included**: `crossOAB`, `sortPointsXY`, `convexHull`, `hullArea`, `hullPerimeter`.

## Quick Start

```cp
import { Pt2, convexHull, hullArea, hullPerimeter } from "monotone-chain-convex-hull";

let pts: [5]Pt2 = [
    Pt2 { x: 0.0, y: 0.0 },
    Pt2 { x: 4.0, y: 0.0 },
    Pt2 { x: 2.0, y: 2.0 }, // interior point
    Pt2 { x: 4.0, y: 4.0 },
    Pt2 { x: 0.0, y: 4.0 },
];
let hull: [5]Pt2 = undefined;
const count = convexHull(&pts, &hull); // 4
const a = hullArea(hull[0..count]);    // 16.0
```

## License

MIT
