[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# splines

Zero-heap Natural Cubic Spline (Thomas O(N)), Catmull-Rom spline, quadratic/cubic Bezier curves, smoothstep, and linear interpolation.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:splines` ([https://github.com/phaazon/splines](https://github.com/phaazon/splines))（原库名 `splines` → CP 包名 `splines`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `splines` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add splines
```

```cp
import {
    Vec2,
    CubicSpline,
    lerp,
    smoothstep,
    bezierQuad,
    bezierCubic,
    catmullRom,
} from "splines";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Vec2` | `class` | 字段: `x: f64, y: f64` |
| `CubicSpline` | `class` | 字段: `xs: [64]f64, a: [64]f64, b: [64]f64, c: [64]f64, d: [64]f64, count: usize, +1 项` · 方法: `build, isValid, knotCount, findSegment, eval, derivative` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lerp` | `(a: f64, b: f64, t: f64): f64` | 调用 `lerp` 并返回 `f64`。 |
| `smoothstep` | `(edge0: f64, edge1: f64, x: f64): f64` | 调用 `smoothstep` 并返回 `f64`。 |
| `bezierQuad` | `(p0: Vec2, p1: Vec2, p2: Vec2, t: f64): Vec2` | 调用 `bezierQuad` 并返回 `Vec2`。 |
| `bezierCubic` | `(p0: Vec2, p1: Vec2, p2: Vec2, p3: Vec2, t: f64): Vec2` | 调用 `bezierCubic` 并返回 `Vec2`。 |
| `catmullRom` | `(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64` | 调用 `catmullRom` 并返回 `f64`。 |

## 最小调用示例

```cp
import { lerp } from "splines";

pub fn main() {
    let result = lerp(0.0, 0.0, 0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
