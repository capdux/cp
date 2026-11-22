# `cap:gjk` — 2D GJK 凸多边形碰撞检测与 SAT MTV 物理库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

零堆分配（Zero-Alloc）的纯 CP 2D 凸体碰撞检测与物理接触求解库，完整实现 Gilbert-Johnson-Keerthi (GJK) 单纯形演化、Minkowski 差支撑函数、分离轴定理（SAT）最小平移向量（MTV）以及圆-凸多边形精确接触检测，适用于 2D 游戏物理引擎、机器人避障与 CAD 几何干涉检测。

## 核心特性

- **零堆分配**：纯栈值与只读/可写顶点切片操作，无任何动态内存分配。
- **2D GJK 凸体检测**：支持 Line/Triangle 单纯形演化与三重积垂线方向更新，内置 64 轮迭代保护防退化振荡。
- **SAT 最小平移向量（MTV）**：遍历双多边形全边法线，发现分离轴提前退出；碰撞时输出精确最小穿透深度 `depth` 与从 A 指向 B 的单位法线 `normal`。
- **圆-凸多边形精确接触**：融合边法线与最近顶点 Voronoi 轴，精确区分面对面接触、对顶点接触与分离。

## 导出 API

### 数据模型

- `Vec2 { x, y }`：`add`, `sub`, `scale`, `neg`, `dot`, `cross`, `lenSq`, `len`, `normalize`, `perp`
- `SatContact { colliding, normal, depth }`：`mtv(): Vec2`
- `GjkResult { colliding, iterations }`

### 核心函数

- `polygonSupport(verts: []const Vec2, dir: Vec2): Vec2?`
- `minkowskiSupport(poly_a: []const Vec2, poly_b: []const Vec2, dir: Vec2): Vec2?`
- `gjkIntersects(poly_a: []const Vec2, poly_b: []const Vec2): GjkResult`
- `satCollide(poly_a: []const Vec2, poly_b: []const Vec2): SatContact`
- `circlePolygonCollide(center: Vec2, radius: f64, poly: []const Vec2): SatContact`
- `polygonCentroid(verts: []const Vec2): Vec2?`
- `translatePolygon(src: []const Vec2, delta: Vec2, dst: []Vec2): bool`

## 使用示例

```cp
import std from "std";
import { Vec2, gjkIntersects, satCollide, translatePolygon } from "cap:gfx";

fn example() {
    const a: [4]Vec2 = [
        Vec2 { x: 0.0, y: 0.0 }, Vec2 { x: 2.0, y: 0.0 },
        Vec2 { x: 2.0, y: 2.0 }, Vec2 { x: 0.0, y: 2.0 },
    ];
    const b: [4]Vec2 = [
        Vec2 { x: 1.5, y: 0.5 }, Vec2 { x: 3.5, y: 0.5 },
        Vec2 { x: 3.5, y: 2.5 }, Vec2 { x: 1.5, y: 2.5 },
    ];

    const g = gjkIntersects(a[0..], b[0..]); // colliding == true
    const s = satCollide(a[0..], b[0..]);    // depth == 0.5, normal == (1, 0)
    let pushed: [4]Vec2 = std.mem.zeroes([4]Vec2);
    * = translatePolygon(b[0..], s.mtv(), pushed[0..]);
    * = g;
}
```
