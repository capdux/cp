# `cap:bezier` — 2D 二次/三次贝塞尔曲线零分配计算几何库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

零堆分配（Zero-Alloc）的纯 CP 2D 贝塞尔曲线计算几何库，提供二次/三次贝塞尔曲线求值、一二阶导数、单位切/法向量、有符号曲率、De Casteljau 细分、二次精确升阶、分段 5 点 Gauss-Legendre 弧长积分、解析极值包围盒（Tight AABB）、均匀采样与弧长反求参数。

## 核心特性

- **零堆分配**：纯值语义结构体与调用方输出切片，适用于矢量图形渲染、SVG 路径处理、字体轮廓、机器人轨迹规划与 CAD 几何引擎。
- **精确解析包围盒（Tight AABB）**：分别求解二次与三次导数方程 $B'(t) = 0$ 的实根，得到紧贴曲线真实极值的最小包围盒。
- **高精度 Gauss-Legendre 弧长积分**：采用 4 段 × 5 点复合 Gauss-Legendre 求积，单次常数时间即可达到极高精度（四分之一圆弧误差远低于 `1e-4`）。
- **De Casteljau 细分与精确升阶**：支持任意参数 `t` 处无损二分以及二次贝塞尔无损升阶至三次贝塞尔。

## 导出 API

- `Vec2 { x, y }`：`add`, `sub`, `scale`, `dot`, `cross`, `len`, `dist`, `lerp`, `normalize`
- `BBox2 { min_x, min_y, max_x, max_y }`：`width`, `height`, `contains`, `expand`
- `QuadPair { left, right }`、`CubicPair { left, right }`
- `QuadBezier { p0, p1, p2 }`：`eval`, `derivative`, `tangent`, `normal`, `split`, `elevateToCubic`, `bbox`, `arcLength`
- `CubicBezier { p0, p1, p2, p3 }`：`eval`, `derivative`, `secondDerivative`, `tangent`, `normal`, `curvature`, `split`, `bbox`, `arcLengthTo`, `arcLength`, `sampleUniform`
- `tAtArcLength(curve: CubicBezier, target_len: f64): f64`：二分 + 积分反求弧长对应参数 `t ∈ [0, 1]`

## 使用示例

```cp
import { Vec2, CubicBezier, tAtArcLength } from "cap:gfx";

fn example() {
    const k: f64 = 0.5522847498307936;
    const arc = CubicBezier {
        p0: Vec2 { x: 1.0, y: 0.0 },
        p1: Vec2 { x: 1.0, y: k },
        p2: Vec2 { x: k, y: 1.0 },
        p3: Vec2 { x: 0.0, y: 1.0 },
    };

    const len = arc.arcLength();                 // ≈ pi / 2
    const t_mid = tAtArcLength(arc, len * 0.5);  // ≈ 0.5
    const box = arc.bbox();                      // [0, 1] x [0, 1]
    * = t_mid;
    * = box;
}
```
