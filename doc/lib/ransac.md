# `cap:ransac` — 零分配确定性 RANSAC 鲁棒模型估计库

零堆分配（Zero-Alloc）的纯 CP RANSAC（Random Sample Consensus）鲁棒几何与回归拟合库，内置无全局状态的确定性 SplitMix64 伪随机采样器，支持 2D 直线（TLS 2×2 特征向量精修）、2D 圆（3 点外接圆 + Kåsa 代数精修）、3D 平面（3×3 协方差最小特征向量精修）及 1D 线性回归（OLS 精修）。

## 核心特性

- **零堆分配（Zero-Alloc）**：所有内点掩码由调用方切片 `inlier_mask_out` 提供，内部精修（2×2 解析特征向量、Kåsa 2×2 系统、3×3 Jacobi）全部在栈上完成。
- **确定性可复现**：相同 `RansacConfig.seed` 下产生完全一致的采样序列与模型参数，零全局可变状态。
- **全方向几何稳健性**：2D 直线采用正交距离 `|ax + by + c|`（支持垂直线 `x = c`）；3D 平面采用规范化单位法向量；2D 圆采用平移中心化外接圆公式抗大坐标抵消。
- **内点最小二乘精修**：可选 `refine: true` 在最优内点集上执行 TLS / Kåsa / 3×3 PCA / OLS 精修，并在不劣于采样模型时自动采纳。

## 导出 API

### 配置与数据模型

- `RansacConfig { threshold: f64, max_iterations: usize, seed: u64, min_inliers: usize, refine: bool }`
  - `RansacConfig.default(threshold: f64): RansacConfig`
- `Point2 { x: f64, y: f64 }`、`Point3 { x: f64, y: f64, z: f64 }`
- `Line2Model { a: f64, b: f64, c: f64, inlier_count: usize, rmse: f64, iterations: usize }`
- `Circle2Model { cx: f64, cy: f64, radius: f64, inlier_count: usize, rmse: f64, iterations: usize }`
- `Plane3Model { a: f64, b: f64, c: f64, d: f64, inlier_count: usize, rmse: f64, iterations: usize }`
- `Linear1DModel { slope: f64, intercept: f64, inlier_count: usize, rmse: f64, iterations: usize }`

### 核心函数

- `ransacFitLine2D(points: []const Point2, cfg: RansacConfig, inlier_mask_out: []u8): Line2Model?`
- `ransacFitCircle2D(points: []const Point2, cfg: RansacConfig, inlier_mask_out: []u8): Circle2Model?`
- `ransacFitPlane3D(points: []const Point3, cfg: RansacConfig, inlier_mask_out: []u8): Plane3Model?`
- `ransacFitLinear1D(xs: []const f64, ys: []const f64, cfg: RansacConfig, inlier_mask_out: []u8): Linear1DModel?`

## 使用示例

```cp
import std from "std";
import {
    RansacConfig,
    Point2,
    ransacFitLine2D,
} from "cap:ransac";

fn example() {
    const pts: [6]Point2 = [
        Point2 { x: 0.0, y: 1.0 },
        Point2 { x: 1.0, y: 3.0 },
        Point2 { x: 2.0, y: 5.0 },
        Point2 { x: 3.0, y: 7.0 },
        Point2 { x: 1.0, y: -50.0 }, // outlier
        Point2 { x: 2.0, y: 80.0 },  // outlier
    ];
    let mask: [6]u8 = std.mem.zeroes([6]u8);
    const cfg = RansacConfig.default(0.1);
    if (line := ransacFitLine2D(pts[0..], cfg, mask[0..])) {
        * = line.inlier_count; // 4
    }
}
```
