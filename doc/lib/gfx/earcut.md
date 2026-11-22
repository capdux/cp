# `cap:earcut` — 2D 简单多边形耳切法三角剖分与计算几何库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

纯 CP、零堆分配的 2D 计算几何与多边形三角剖分标准库，提供鞋带公式有向面积、凹凸性判定、点在三角形/多边形内射线法判定以及自动兼容 CCW/CW 绕向的零分配耳切法三角剖分（Ear-Clipping Triangulation）。

## 导入

```cp
import {
    Point2D,
    TriangleIdx,
    signedArea,
    polygonArea,
    isCounterClockwise,
    isConvex,
    pointInTriangle,
    pointInPolygon,
    triangulate,
    triangulationTotalArea,
} from "cap:gfx";
```

## 核心 API

- **数据模型**：`Point2D { x: f64, y: f64 }`、`TriangleIdx { a: usize, b: usize, c: usize }`
- **基础几何原语**：
  - `signedArea(pts: []const Point2D): f64` — 鞋带公式有向面积（CCW 为正，CW 为负）。
  - `polygonArea(pts: []const Point2D): f64` — 非负面积。
  - `isCounterClockwise(pts: []const Point2D): bool`、`isConvex(pts: []const Point2D): bool`
  - `pointInTriangle(p, a, b, c): bool`、`pointInPolygon(p, pts): bool`（射线法）
- **耳切法三角剖分（零堆分配）**：
  - `triangulate(pts: []const Point2D, out_tris: []TriangleIdx, scratch_idx: []usize): usize?` — 输出 `n - 2` 个 CCW 三角形索引，自动兼容 CW 输入。
  - `triangulationTotalArea(pts: []const Point2D, tris: []const TriangleIdx): f64` — 校验剖分面积守恒。

## 示例

```cp
import std from "std";
import { Point2D, TriangleIdx, triangulate } from "cap:gfx";

fn triangulateQuad(): usize? {
    const quad: [4]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 2.0, y: 0.0 },
        Point2D { x: 2.0, y: 2.0 },
        Point2D { x: 0.0, y: 2.0 },
    ];
    let tris: [2]TriangleIdx = std.mem.zeroes([2]TriangleIdx);
    let scratch: [4]usize = std.mem.zeroes([4]usize);
    return triangulate(quad[0..], tris[0..], scratch[0..]);
}
```
