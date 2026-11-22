# CP 标准库：2D 平面计算几何、凸包与碰撞检测引擎 (`cap:geom2d` / `cap.geom2d`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:geo`，导入请使用 `import { ... } from "cap:geo"`。

## 一、 架构总览与设计准则

`cap:geom2d` 是 CP 语言官方内置的纯原生、零 C 依赖的 **2D 平面计算几何、凸包（Convex Hull）、线段相交与碰撞检测引擎**，面向图形渲染（配合 `cap:svg`）、游戏物理、CAD 与机器人路径规划场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:geo"` 与 `import cap from "cap"; cap.geom2d.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/geom2d/geom.cp` | `// Source: cp/lib/geom2d/geom.cp (...)` | `GeomError`、`Orientation`、`Point2`、`Segment2`、`Aabb2`、`Circle2` 与 `Geom2D` 核心几何算法 |
| `lib/geom2d/index.cp` | `// Source: cp/lib/geom2d/index.cp (...)` | `cap:geom2d` 与 `cap.geom2d` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与方向枚举 (`GeomError` & `Orientation`)

```typescript
pub err GeomError {
    InsufficientPoints,
    DegenerateGeometry,
    OutOfMemory,
}

pub enum Orientation {
    Collinear = 0,
    Clockwise = 1,
    CounterClockwise = 2,
}
```

### 2. 几何基元 (`Point2`, `Segment2`, `Aabb2`, `Circle2`)

- **`Point2` (`x: f64, y: f64`)**：
  - `Point2.init(x, y): Point2` / `Point2.zero(): Point2`
  - `add(other): Point2` / `sub(other): Point2` / `scale(factor): Point2`
  - `dot(other): f64` / `cross(other): f64`
  - `distSq(other): f64` / `dist(other): f64` / `normSq(): f64` / `norm(): f64`
  - `rotate(angle_rad): Point2` — 绕原点逆时针旋转 `angle_rad` 弧度
  - `equals(other, eps): bool`
- **`Segment2` (`a: Point2, b: Point2`)**：
  - `Segment2.init(a, b): Segment2`
  - `lengthSq(): f64` / `length(): f64` / `midpoint(): Point2`
- **`Aabb2` (`min_x, min_y, max_x, max_y`)**：
  - `Aabb2.init(x1, y1, x2, y2): Aabb2` — 自动规范化最小/最大边界
  - `Aabb2.fromPoints(pts): Aabb2 ! GeomError` — 计算点集紧致包围盒
  - `width(): f64` / `height(): f64` / `area(): f64` / `center(): Point2`
  - `contains(p): bool` / `intersects(other): bool`
- **`Circle2` (`center: Point2, radius: f64`)**：
  - `Circle2.init(center, radius): Circle2`
  - `area(): f64` / `circumference(): f64`
  - `contains(p): bool` / `intersects(other): bool`

### 3. 核心算法引擎 (`Geom2D`)

- `Geom2D.orientation(p, q, r): Orientation` — 基于叉积符号判定三点共线、顺时针或逆时针。
- `Geom2D.segmentsIntersect(s1, s2): bool` — 快速 AABB 排斥 + 跨立实验 + 端点接触与共线重叠判定。
- `Geom2D.segmentIntersectionPoint(s1, s2): Point2?` — 精确计算两线段交点；对共线重叠返回字典序最小的公共端点，不相交返回 `null`。
- `Geom2D.pointToSegmentDist(p, s): f64` — 点到线段的钳位投影最短欧氏距离（自动处理退化零长线段）。
- `Geom2D.polygonArea(pts): f64` — Shoelace 鞋带公式计算简单多边形无符号面积。
- `Geom2D.polygonCentroid(pts): Point2 ! GeomError` — 计算非退化简单多边形几何质心（少于 3 点返回 `InsufficientPoints`，面积为 0 返回 `DegenerateGeometry`）。
- `Geom2D.pointInPolygon(p, pts): bool` — 射线法判断点是否在多边形内（含边上与顶点精确判定，支持凹多边形）。
- `Geom2D.convexHull(alloc, pts): []Point2 ! GeomError` — Andrew 单调链 $O(N \log N)$ 凸包算法，自动去重、过滤内点与边界共线点，返回逆时针极点切片（调用方负责 `alloc.free`）；退化共线或少于 3 点分别返回 `DegenerateGeometry` / `InsufficientPoints`。
- `Geom2D.aabbIntersects(a, b): bool` / `Geom2D.circleIntersects(c1, c2): bool` / `Geom2D.circleAabbIntersects(c, box): bool` — 精确 2D 碰撞检测。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Point2, Segment2, Aabb2, Circle2, Geom2D } from "cap:geo";

pub fn demoGeom2d(): ! {
    const alloc = std.heap.page_allocator;

    const s1 = Segment2.init(Point2.init(0.0, 0.0), Point2.init(4.0, 4.0));
    const s2 = Segment2.init(Point2.init(0.0, 4.0), Point2.init(4.0, 0.0));
    if (hit := Geom2D.segmentIntersectionPoint(s1, s2)) {
        // hit == (2.0, 2.0)
        * = hit;
    }

    const pts = [
        Point2.init(0.0, 0.0),
        Point2.init(4.0, 0.0),
        Point2.init(2.0, 1.0),
        Point2.init(4.0, 4.0),
        Point2.init(0.0, 4.0),
    ];
    const hull = Geom2D.convexHull(alloc, &pts)?;
    defer alloc.free(hull);

    // 动态懒加载挂载等价用法
    const box = cap.geom2d.Aabb2.init(0.0, 0.0, 10.0, 10.0);
    const cir = cap.geom2d.Circle2.init(cap.geom2d.Point2.init(11.0, 5.0), 1.5);
    const hit_box = cap.geom2d.Geom2D.circleAabbIntersects(cir, box);
    * = hit_box;
}
```
