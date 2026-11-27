[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# b-spline

面向 CP 语言的零堆分配通用 B-Spline / NURBS（De Boor 算法）、三次 Hermite、Catmull-Rom 与均匀三次 B 样条插值库。

对齐 npm 源包 [`b-spline`](https://github.com/thibauts/b-spline) 与 Rust crates.io [`bspline`](https://github.com/Twinklebear/bspline)（MIT）。

## 安装与导入

```bash
cap add b-spline
```

```cp
import {
    BSPLINE_MAX_DEGREE,
    BSPLINE_MAX_KNOTS,
    Pt2,
    Pt3,
    bspline1D,
    bspline2D,
    bspline3D,
    bspline2DUniform,
    generateClampedKnots,
    sampleBSpline2D,
    catmullRom2D,
    hermite2D,
} from "b-spline";
```

## 核心 API

### 常量与数据结构

| 符号 | 类型 / 字段 | 说明 |
| --- | --- | --- |
| `BSPLINE_MAX_DEGREE` | `usize = 16` | 栈上工作区支持的最大样条阶数（degree）。 |
| `BSPLINE_MAX_KNOTS` | `usize = 256` | 栈上工作区支持的最大节点向量（knot vector）长度。 |
| `Pt2` | `{ x: f64, y: f64 }` | 二维控制点/采样点。 |
| `Pt3` | `{ x: f64, y: f64, z: f64 }` | 三维控制点/采样点。 |

### 样条与曲线函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `hermite1D` / `hermite2D` | `(p0, m0, p1, m1, t: f64): f64 / Pt2` | 给定端点与切线的三次 Hermite 插值。 |
| `catmullRom1D` / `catmullRom2D` | `(p0, p1, p2, p3, t: f64): f64 / Pt2` | 4 点 Catmull-Rom 曲线段插值。 |
| `uniformCubicBSpline1D` / `2D` | `(p0, p1, p2, p3, t: f64): f64 / Pt2` | 均匀三次 B 样条单段求值。 |
| `evalCatmullRomChain` | `(pts: []const Pt2, u: f64): Pt2` | 在参数 `u`（`[0, 1]`）处求值多点 2D Catmull-Rom 链。 |
| `generateClampedKnots` | `(num_points: usize, degree: usize, out_knots: []f64): usize` | 生成准均匀（两端钳位）节点向量写入 `out_knots`，返回写入长度。 |
| `generateUniformKnots` | `(num_points: usize, degree: usize, out_knots: []f64): usize` | 生成均匀节点向量写入 `out_knots`，返回写入长度。 |
| `findKnotSpan` | `(n: usize, degree: usize, u: f64, knots: []const f64): usize` | 查找参数 `u` 所在的节点区间索引。 |
| `bspline1D` / `2D` / `3D` | `(t: f64, degree: usize, points, knots: []const f64, weights: []const f64): T?` | 通用有理/非有理 B 样条求值（`knots`/`weights` 传空切片启用默认均匀/等权），非法参数返回 `null`。 |
| `bspline1DUniform` / `2DUniform` / `3DUniform` | `(t: f64, degree: usize, points): T?` | 无需手动构造节点与权重的均匀 B 样条快捷求值。 |
| `sampleBSpline2D` | `(degree: usize, points: []const Pt2, knots: []const f64, weights: []const f64, out: []Pt2): usize` | 在 `out` 中均匀采样 2D B 样条曲线，返回写入点数。 |

## 使用示例

### 1. 钳位 2D B 样条求值与批量采样

```cp
import { print } from "cap:io";
import { Pt2, generateClampedKnots, bspline2D, sampleBSpline2D } from "b-spline";

pub fn main() {
    let ctrl: [4]Pt2 = [
        Pt2 { x: 0.0, y: 0.0 },
        Pt2 { x: 1.0, y: 2.0 },
        Pt2 { x: 3.0, y: 2.0 },
        Pt2 { x: 4.0, y: 0.0 },
    ];
    let knots: [8]f64 = undefined;
    let klen = generateClampedKnots(4, 3, knots[0..8]);
    let empty_w: [0]f64 = [];

    let mid = bspline2D(0.5, 3, ctrl[0..4], knots[0..klen], empty_w[0..0]);
    if (mid != null) {
        print("mid=({}, {})\n", mid!.x, mid!.y);
    }

    let samples: [16]Pt2 = undefined;
    let n = sampleBSpline2D(3, ctrl[0..4], knots[0..klen], empty_w[0..0], samples[0..16]);
    print("已采样 {} 个点\n", n);
}
```

### 2. 均匀 3D B 样条快捷求值

```cp
import { print } from "cap:io";
import { Pt3, bspline3DUniform } from "b-spline";

pub fn main() {
    let pts: [4]Pt3 = [
        Pt3 { x: 0.0, y: 0.0, z: 0.0 },
        Pt3 { x: 1.0, y: 1.0, z: 0.5 },
        Pt3 { x: 2.0, y: 0.0, z: 1.0 },
        Pt3 { x: 3.0, y: 1.0, z: 1.5 },
    ];
    let p = bspline3DUniform(0.5, 2, pts[0..4]);
    if (p != null) {
        print("3D point: ({}, {}, {})\n", p!.x, p!.y, p!.z);
    }
}
```

### 3. Catmull-Rom 曲线链求值

```cp
import { Pt2, evalCatmullRomChain } from "b-spline";

pub fn samplePath(pts: []const Pt2): Pt2 {
    return evalCatmullRomChain(pts, 0.25);
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：De Boor 递推工作区全部位于栈上（`BSPLINE_MAX_DEGREE = 16`、`BSPLINE_MAX_KNOTS = 256`），无动态内存分配。
- **强类型 1D/2D/3D 接口**：以 `f64`、`Pt2`、`Pt3` 替代 JS 动态数组，便于寄存器优化与值传递。
- **NURBS 与多曲线合一**：支持可选有理权重（`weights`），同时内置 Hermite 与 Catmull-Rom 曲线求值器。
