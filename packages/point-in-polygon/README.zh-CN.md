[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# point-in-polygon

Zero-heap 2D polygon geometry (Shoelace area, centroid, perimeter, ray-casting containment, convexity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:point-in-polygon` ([https://github.com/substack/point-in-polygon](https://github.com/substack/point-in-polygon)) / `crates.io:geo-types` ([https://github.com/georust/geo](https://github.com/georust/geo))（原库名 `point-in-polygon` → CP 包名 `point-in-polygon`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `point-in-polygon` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add point-in-polygon
```

```cp
import {
    Pt2,
    absF64,
    sqrtF64,
    signedArea,
    area,
    isCounterClockwise,
    perimeter,
    centroid,
} from "point-in-polygon";
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
| `signedArea` | `(pts: &[]Pt2): f64` | 调用 `signedArea` 并返回 `f64`。 |
| `area` | `(pts: &[]Pt2): f64` | 调用 `area` 并返回 `f64`。 |
| `isCounterClockwise` | `(pts: &[]Pt2): bool` | 执行 `isCounterClockwise` 条件判定，返回 `bool`。 |
| `perimeter` | `(pts: &[]Pt2): f64` | 调用 `perimeter` 并返回 `f64`。 |
| `centroid` | `(pts: &[]Pt2): Pt2` | 调用 `centroid` 并返回 `Pt2`。 |
| `pointOnSegment` | `(p: Pt2, a: Pt2, b: Pt2): bool` | 调用 `pointOnSegment` 并返回 `bool`。 |
| `containsPoint` | `(pts: &[]Pt2, p: Pt2): bool` | 调用 `containsPoint` 并返回 `bool`。 |
| `isConvex` | `(pts: &[]Pt2): bool` | 执行 `isConvex` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isCounterClockwise } from "point-in-polygon";

pub fn main() {
    let result = isCounterClockwise(/* pts: &[]Pt2 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
