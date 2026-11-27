[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# parry3d

Zero-allocation 3D Axis-Aligned Bounding Box (AABB) collision detection, union/intersection, surface area/volume, and Kay-Kajiya slab raycast library for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:parry3d` ([https://github.com/dimforge/parry](https://github.com/dimforge/parry)) / `npm:aabb-3d` ([https://github.com/chrisdickinson/aabb-3d](https://github.com/chrisdickinson/aabb-3d))（原库名 `parry3d` → CP 包名 `parry3d`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `parry3d` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add parry3d
```

```cp
import {
    Pt3,
    Aabb3,
    abAbs,
    abMin,
    abMax,
} from "parry3d";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pt3` | `class` | 字段: `x: f64, y: f64, z: f64` |
| `Aabb3` | `class` | 字段: `min_x: f64, min_y: f64, min_z: f64, max_x: f64, max_y: f64, max_z: f64` · 方法: `new, fromCenterSize, isValid, volume, surfaceArea, center, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `abAbs` | `(x: f64): f64` | 调用 `abAbs` 并返回 `f64`。 |
| `abMin` | `(a: f64, b: f64): f64` | 调用 `abMin` 并返回 `f64`。 |
| `abMax` | `(a: f64, b: f64): f64` | 调用 `abMax` 并返回 `f64`。 |

## 最小调用示例

```cp
import { abAbs } from "parry3d";

pub fn main() {
    let result = abAbs(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
