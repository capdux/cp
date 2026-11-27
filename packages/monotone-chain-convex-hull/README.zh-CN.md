[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# monotone-chain-convex-hull

Zero-heap 2D Andrew's monotone chain convex hull, area, and perimeter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:monotone-chain-convex-hull` ([https://github.com/mikolalysenko/monotone-chain-convex-hull](https://github.com/mikolalysenko/monotone-chain-convex-hull))（原库名 `monotone-chain-convex-hull` → CP 包名 `monotone-chain-convex-hull`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `monotone-chain-convex-hull` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add monotone-chain-convex-hull
```

```cp
import {
    Pt2,
    absF64,
    sqrtF64,
    ptLess,
    ptEqual,
    crossOAB,
    sortPointsXY,
    convexHull,
} from "monotone-chain-convex-hull";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pt2` | `class` | 字段: `x: f64, y: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `ptLess` | `(a: Pt2, b: Pt2): bool` | 调用 `ptLess` 并返回 `bool`。 |
| `ptEqual` | `(a: Pt2, b: Pt2): bool` | 调用 `ptEqual` 并返回 `bool`。 |
| `crossOAB` | `(o: Pt2, a: Pt2, b: Pt2): f64` | 调用 `crossOAB` 并返回 `f64`。 |
| `sortPointsXY` | `(pts: []Pt2)` | 就地执行 `sortPointsXY` 操作。 |
| `convexHull` | `(work_pts: []Pt2, out_hull: []Pt2): usize` | 调用 `convexHull` 并返回 `usize`。 |
| `hullArea` | `(hull: &[]Pt2): f64` | 调用 `hullArea` 并返回 `f64`。 |
| `hullPerimeter` | `(hull: &[]Pt2): f64` | 调用 `hullPerimeter` 并返回 `f64`。 |

## 最小调用示例

```cp
import { absF64 } from "monotone-chain-convex-hull";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
