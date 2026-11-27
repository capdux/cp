[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `point-in-polygon`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:point-in-polygon` ([https://github.com/substack/point-in-polygon](https://github.com/substack/point-in-polygon)) / `crates.io:geo-types` ([https://github.com/georust/geo](https://github.com/georust/geo))（原库名 `point-in-polygon` → CP 包名 `point-in-polygon`）

Zero-heap 2D polygon geometry algorithms in pure CP: Shoelace signed/unsigned area, winding direction, perimeter, centroid, ray-casting point-in-polygon with edge tolerance, and convexity check.

## Features

- **100% Zero Heap Allocation & Zero Dependencies**: Operates directly on `[]const Pt2` slices with built-in Newton-Raphson `sqrtF64`.
- **Complete Geometry Suite**: `signedArea`, `area`, `isCounterClockwise`, `perimeter`, `centroid`, `containsPoint`, `isConvex`.
- **Degenerate Safety**: Gracefully handles `< 3` vertices and zero-area polygons.

## Quick Start

```cp
import { Pt2, area, centroid, containsPoint, isConvex } from "point-in-polygon";

const sq: [4]Pt2 = [
    Pt2 { x: 0.0, y: 0.0 },
    Pt2 { x: 4.0, y: 0.0 },
    Pt2 { x: 4.0, y: 4.0 },
    Pt2 { x: 0.0, y: 4.0 },
];
const a = area(&sq); // 16.0
const c = centroid(&sq); // (2.0, 2.0)
const inside = containsPoint(&sq, Pt2 { x: 2.0, y: 2.0 }); // true
```

## License

MIT
