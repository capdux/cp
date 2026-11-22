# `cap:voronoi` — 零分配 2D 有界 Voronoi 图与 Lloyd 松弛库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

零堆分配（Zero-Alloc）的纯 CP 2D 有界 Voronoi 剖分与质心 Voronoi 镶嵌（CVT / Lloyd 松弛）几何库，采用数值极稳健的 Sutherland-Hodgman 垂直平分线半平面逐站裁剪算法，在任意矩形包围盒 `[min_x, max_x] x [min_y, max_y]` 内精确构建每个站点的 CCW 凸多边形单元、计算 Shoelace 面积与面积加权质心。

## 核心特性

- **零堆分配（Zero-Alloc）**：所有多边形顶点缓冲与双缓冲 ping-pong 裁剪工作区由调用方栈或静态切片提供。
- **数值稳健半平面裁剪**：使用中点形式垂直平分线方程 $(Q - P) \cdot X \le (Q - P) \cdot \frac{P + Q}{2}$，避免大坐标平方相减抵消，自动跳过重合站点并去重相邻极近顶点。
- **精确面积守恒**：对任意非退化有限点集，所有 Voronoi 单元面积之和严格等于矩形包围盒总面积。
- **Lloyd 松弛迭代**：`lloydRelaxationStep` 单步将每个站点移动至其 Voronoi 单元质心并返回最大欧氏位移量，适合蓝噪声采样、网格均匀化与空间区域划分。
- **完备边界防护**：全面拦截空站点、越界索引、非法包围盒、缓冲区不足与 `NaN`/`Inf` 坐标。

## 导出 API

### 数据模型

- `Point2 { x: f64, y: f64 }`
- `Bounds2 { min_x: f64, min_y: f64, max_x: f64, max_y: f64 }`（方法 `isValid(): bool`, `area(): f64`, `contains(p: Point2): bool`）
- `CellMetrics { vertex_count: usize, area: f64, centroid: Point2 }`

### 核心函数

- `clipPolygonHalfPlane(poly_in: []const Point2, a: f64, b: f64, c: f64, poly_out: []Point2): usize?`
  - 保留满足 `a * x + b * y <= c` 的凸多边形区域。
- `polygonArea(poly: []const Point2): f64` — Shoelace 面积。
- `polygonCentroid(poly: []const Point2): Point2?` — 面积加权质心。
- `computeVoronoiCell(sites: []const Point2, site_idx: usize, bounds: Bounds2, out_poly: []Point2, scratch_poly: []Point2): CellMetrics?`
  - 计算 `sites[site_idx]` 的有界 Voronoi 凸多边形与面积/质心。
- `lloydRelaxationStep(sites: []const Point2, bounds: Bounds2, next_sites_out: []Point2, poly_buf_a: []Point2, poly_buf_b: []Point2): f64?`
  - 执行一轮 Lloyd 松弛，返回最大站点位移量。
- `nearestSite(sites: []const Point2, p: Point2): usize?` — 查询欧氏最近站点下标。

## 使用示例

```cp
import std from "std";
import {
    Point2,
    Bounds2,
    computeVoronoiCell,
    lloydRelaxationStep,
} from "cap:graph";

fn example() {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const sites: [4]Point2 = [
        Point2 { x: 2.0, y: 2.0 },
        Point2 { x: 8.0, y: 2.0 },
        Point2 { x: 2.0, y: 8.0 },
        Point2 { x: 8.0, y: 8.0 },
    ];
    let buf_a: [32]Point2 = std.mem.zeroes([32]Point2);
    let buf_b: [32]Point2 = std.mem.zeroes([32]Point2);

    if (cell := computeVoronoiCell(sites[0..], 0, bounds, buf_a[0..], buf_b[0..])) {
        * = cell.area; // 25.0
    }

    let next_sites: [4]Point2 = std.mem.zeroes([4]Point2);
    * = lloydRelaxationStep(sites[0..], bounds, next_sites[0..], buf_a[0..], buf_b[0..]);
}
```
