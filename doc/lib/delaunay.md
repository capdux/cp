# `cap:delaunay` — 纯 CP 零堆分配 2D Delaunay 三角剖分库

`cap:delaunay` 提供纯 CP、零堆分配的 2D Bowyer-Watson 增量 Delaunay 三角剖分实现，涵盖几何定向谓词、外接圆与空圆判定、点定位以及全量空外接圆准则校验。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部内部三角形池、空洞边界边与输出缓冲均由调用方栈/切片内存提供。
- **稳健 Bowyer-Watson 剖分**：自动构造包围点集的超大 CCW 超级三角形，使用无除法 $3 \times 3$ 行列式执行外接圆测试，自动跳过重合点，确保输出三角形全部保持逆时针（CCW）绕序。
- **空外接圆校验与点定位**：内置 `verifyDelaunayEmptyCircle` 与 `locateTriangle`，便于几何验证与网格查询。
- **完备边界防御**：优雅处理少于 3 点、全共线点、重合点；对 `NaN`/`Inf` 坐标或缓冲不足安全返回 `null`。

## API 参考

### 数据结构

```cp
pub struct Point2 { x: f64, y: f64 }
pub struct Triangle { a: usize, b: usize, c: usize }
pub struct Edge2 { u: usize, v: usize }
pub struct Circumcircle { cx: f64, cy: f64, radiusSq: f64 }

pub struct DelaunayWorkspace {
    tris: []Triangle,   // 推荐 >= delaunayMinTriPool(n) = 2*n + 16
    bad: []u8,          // >= tris.len
    edges: []Edge2,     // 推荐 >= delaunayMinEdgePool(n) = 6*n + 48
    edge_dup: []u8,     // >= edges.len
}
```

### 几何谓词与核心函数

- `delaunayMinTriPool(n: usize): usize` / `delaunayMinEdgePool(n: usize): usize`
- `orient2d(a: Point2, b: Point2, c: Point2): f64`
- `circumcircle(a: Point2, b: Point2, c: Point2): Circumcircle?`
- `inCircumcircle(a: Point2, b: Point2, c: Point2, p: Point2): bool`
- `pointInTriangle(a: Point2, b: Point2, c: Point2, p: Point2): bool`
- `triangulate(points: []const Point2, out_triangles: *[]Triangle, ws: *DelaunayWorkspace): usize?`
- `verifyDelaunayEmptyCircle(points: []const Point2, triangles: []const Triangle, eps: f64): bool`
- `locateTriangle(points: []const Point2, triangles: []const Triangle, p: Point2): usize?`

## 使用示例

```cp
import std from "std";
import {
    Point2, Triangle, Edge2, DelaunayWorkspace,
    triangulate, verifyDelaunayEmptyCircle, locateTriangle,
} from "cap:delaunay";

const pts: [5]Point2 = [
    Point2 { x: 0.0, y: 0.0 },
    Point2 { x: 2.0, y: 0.0 },
    Point2 { x: 2.0, y: 2.0 },
    Point2 { x: 0.0, y: 2.0 },
    Point2 { x: 1.0, y: 1.0 },
];

let tris_pool: [32]Triangle = std.mem.zeroes([32]Triangle);
let bad_pool: [32]u8 = std.mem.zeroes([32]u8);
let edge_pool: [96]Edge2 = std.mem.zeroes([96]Edge2);
let dup_pool: [96]u8 = std.mem.zeroes([96]u8);
let ws = DelaunayWorkspace {
    tris: tris_pool[0..], bad: bad_pool[0..],
    edges: edge_pool[0..], edge_dup: dup_pool[0..],
};

let out_buf: [16]Triangle = std.mem.zeroes([16]Triangle);
let out: []Triangle = out_buf[0..];

if (cnt := triangulate(pts[0..], *out, *ws)) {
    const ok = verifyDelaunayEmptyCircle(pts[0..], out[0..cnt], 1.0e-9);
    const hit = locateTriangle(pts[0..], out[0..cnt], Point2 { x: 0.5, y: 0.25 });
    * = ok;
    * = hit;
}
```
