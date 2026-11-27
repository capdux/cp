[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# parry2d

Zero-allocation 2D axis-aligned bounding box (AABB) collision detection, union, intersection, and ray slab intersection for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:parry2d` ([https://github.com/dimforge/parry](https://github.com/dimforge/parry))（原库名 `parry2d` → CP 包名 `parry2d`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `parry2d` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add parry2d
```

```cp
import {
    Pt2,
    Aabb2,
    minF64,
    maxF64,
    new,
    fromCenterSize,
    fromPoints,
} from "parry2d";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pt2` | `class` | 字段: `x: f64, y: f64` · 方法: `new` |
| `Aabb2` | `class` | 字段: `min_x: f64, min_y: f64, max_x: f64, max_y: f64` · 方法: `new, empty, fromCenterSize, fromPoints, isValid, width, +13 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `minF64` | `(a: f64, b: f64): f64` | 调用 `minF64` 并返回 `f64`。 |
| `maxF64` | `(a: f64, b: f64): f64` | 调用 `maxF64` 并返回 `f64`。 |
| `new` | `(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2` | 创建或初始化实例，返回 `Aabb2`。 |
| `fromCenterSize` | `(cx: f64, cy: f64, w: f64, h: f64): Aabb2` | 调用 `fromCenterSize` 并返回 `Aabb2`。 |
| `fromPoints` | `(pts: &[]Pt2): Aabb2` | 调用 `fromPoints` 并返回 `Aabb2`。 |

## 最小调用示例

```cp
import { minF64 } from "parry2d";

pub fn main() {
    let result = minF64(0.0, 0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
