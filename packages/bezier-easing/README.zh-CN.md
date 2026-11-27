[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bezier-easing

Zero-allocation quadratic/cubic Bezier curves, de Casteljau splitting, arc length, and CSS cubic-bezier easing for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bezier-easing` ([https://github.com/gre/bezier-easing](https://github.com/gre/bezier-easing)) / `crates.io:kurbo` ([https://github.com/linebender/kurbo](https://github.com/linebender/kurbo)) / `crates.io:lyon_geom` ([https://github.com/nical/lyon](https://github.com/nical/lyon))（原库名 `bezier-easing` → CP 包名 `bezier-easing`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `bezier-easing` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bezier-easing
```

```cp
import {
    Pt2,
    QuadBezier,
    CubicBezier,
    CubicSplit,
    sqrtF64,
    absF64,
    lerpPt2,
    distPt2,
} from "bezier-easing";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pt2` | `class` | 字段: `x: f64, y: f64` · 方法: `new` |
| `QuadBezier` | `class` | 字段: `p0: Pt2, p1: Pt2, p2: Pt2` · 方法: `new, eval, deriv, arcLength` |
| `CubicBezier` | `class` | 字段: `p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2` · 方法: `new, eval, deriv, arcLength` |
| `CubicSplit` | `class` | 字段: `left: CubicBezier, right: CubicBezier` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `lerpPt2` | `(a: Pt2, b: Pt2, t: f64): Pt2` | 调用 `lerpPt2` 并返回 `Pt2`。 |
| `distPt2` | `(a: Pt2, b: Pt2): f64` | 调用 `distPt2` 并返回 `f64`。 |
| `splitCubic` | `(c: CubicBezier, t: f64): CubicSplit` | 调用 `splitCubic` 并返回 `CubicSplit`。 |
| `evalUnitCubic` | `(p1: f64, p2: f64, t: f64): f64` | 调用 `evalUnitCubic` 并返回 `f64`。 |
| `derivUnitCubic` | `(p1: f64, p2: f64, t: f64): f64` | 调用 `derivUnitCubic` 并返回 `f64`。 |
| `cubicBezierEasing` | `(x1: f64, y1: f64, x2: f64, y2: f64, x: f64): f64` | 调用 `cubicBezierEasing` 并返回 `f64`。 |

## 最小调用示例

```cp
import { sqrtF64 } from "bezier-easing";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
