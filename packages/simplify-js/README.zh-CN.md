[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# simplify-js

面向 CP 语言的零堆分配 Ramer-Douglas-Peucker（RDP）与径向距离 2D/3D 折线抽稀库，适用于 GPS 轨迹压缩、GIS 几何处理与矢量图形渲染。

对齐 npm 源包 [`simplify-js`](https://github.com/mourner/simplify-js)（MIT）。

## 安装与导入

```bash
cap add simplify-js
```

```cp
import {
    SIMPLIFY_MAX_STACK_PTS,
    Pt2,
    Point2D,
    Point3D,
    simplify,
    simplify3D,
    simplifyRadialDist,
    simplifyDouglasPeucker,
    simplifyPolyline,
} from "simplify-js";
```

## 核心 API

### 常量与数据结构

| 符号 | 类型 / 字段 | 说明 |
| --- | --- | --- |
| `SIMPLIFY_MAX_STACK_PTS` | `usize = 1024` | 内置栈上 RDP 工作缓冲区的最大点容量。 |
| `Pt2` / `Point2D` | `{ x: f64, y: f64 }` | 2D 点结构体（`Point2D` 为 `Pt2` 的类型别名）。 |
| `Point3D` | `{ x: f64, y: f64, z: f64 }` | 3D 点结构体，支持含高程/三维轨迹的抽稀。 |

### 距离与抽稀函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `getSqDist` | `(p1: Pt2, p2: Pt2): f64` | 计算两个 2D 点之间的欧氏距离平方。 |
| `pointToSegmentDistSq` | `(p: Pt2, a: Pt2, b: Pt2): f64` | 计算 2D 点 `p` 到线段 `ab` 的垂距平方。 |
| `getSqSegDist` | `(p: Pt2, p1: Pt2, p2: Pt2): f64` | `pointToSegmentDistSq` 别名。 |
| `getSqDist3D` | `(p1: Point3D, p2: Point3D): f64` | 计算两个 3D 点之间的欧氏距离平方。 |
| `getSqSegDist3D` | `(p: Point3D, a: Point3D, b: Point3D): f64` | 计算 3D 点 `p` 到线段 `ab` 的距离平方。 |
| `simplifyRadialDist` | `(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize` | 快速径向距离 2D 预过滤，写入 `out` 并返回保留点数。 |
| `simplifyDouglasPeucker` | `(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize` | 2D RDP 算法（参数为容差平方），返回写入点数。 |
| `simplify` | `(points: []const Pt2, tolerance: f64, highQuality: bool, out: []Pt2): usize` | 2D 主入口；`highQuality = true` 时跳过径向预过滤。 |
| `simplifyPolyline` | `(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize` | 由调用方提供掩码的 2D RDP，突破栈上限限制。 |
| `simplifyRadialDist3D` | `(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize` | 快速径向距离 3D 抽稀。 |
| `simplifyDouglasPeucker3D` | `(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize` | 3D RDP 抽稀（参数为容差平方）。 |
| `simplify3D` | `(points: []const Point3D, tolerance: f64, highQuality: bool, out: []Point3D): usize` | 3D 主入口，返回写入 `out` 的抽稀后点数。 |

## 使用示例

### 1. 标准 2D 折线抽稀

```cp
import { print } from "cap:io";
import { Pt2, simplify } from "simplify-js";

pub fn main() {
    let pts: [5]Pt2 = [
        Pt2 { x: 0.0, y: 0.0 },
        Pt2 { x: 1.0, y: 0.1 },
        Pt2 { x: 2.0, y: -0.1 },
        Pt2 { x: 3.0, y: 5.0 },
        Pt2 { x: 4.0, y: 6.0 },
    ];
    let out: [5]Pt2 = undefined;
    let n = simplify(pts[0..5], 1.0, true, out[0..5]);
    print("保留 {} 个点\n", n);
}
```

### 2. 3D GPS 轨迹抽稀

```cp
import { print } from "cap:io";
import { Point3D, simplify3D } from "simplify-js";

pub fn main() {
    let track: [4]Point3D = [
        Point3D { x: 0.0, y: 0.0, z: 10.0 },
        Point3D { x: 1.0, y: 0.0, z: 10.1 },
        Point3D { x: 2.0, y: 0.0, z: 10.0 },
        Point3D { x: 3.0, y: 2.0, z: 15.0 },
    ];
    let out: [4]Point3D = undefined;
    let count = simplify3D(track[0..4], 0.5, false, out[0..4]);
    print("3D 保留 {} 个点\n", count);
}
```

### 3. 超长折线（调用方提供掩码）

```cp
import { Pt2, simplifyPolyline } from "simplify-js";

pub fn compress(pts: []Pt2, mask: []u8, out: []Pt2): usize {
    return simplifyPolyline(pts, 0.25, mask, out);
}
```

## 与源包差异与零拷贝安全亮点

- **调用方输出缓冲区**：区别于 JS 动态数组分配，所有接口直接写入调用方传入的 `out` 切片并返回写入数量 `usize`。
- **原生 3D 支持**：内置 `Point3D` 与全套 `simplify*3D` 函数，无需额外插件。
- **迭代式无递归栈溢风险**：基于固定栈缓冲（`SIMPLIFY_MAX_STACK_PTS = 1024`）或外部 `keep_mask`（`simplifyPolyline`）运行，杜绝深递归栈溢出。
