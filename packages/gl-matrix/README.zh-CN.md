[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# gl-matrix

Zero-allocation 3D vector and spatial geometry library for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:gl-matrix` ([https://github.com/toji/gl-matrix](https://github.com/toji/gl-matrix))（原库名 `gl-matrix` → CP 包名 `gl-matrix`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `gl-matrix` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add gl-matrix
```

```cp
import {
    Vec3,
    sqrtF64,
    absF64,
    minF64,
    maxF64,
    new,
    zero,
    splat,
} from "gl-matrix";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Vec3` | `class` | 字段: `x: f64, y: f64, z: f64` · 方法: `new, zero, splat, unitX, unitY, unitZ, +18 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `minF64` | `(a: f64, b: f64): f64` | 调用 `minF64` 并返回 `f64`。 |
| `maxF64` | `(a: f64, b: f64): f64` | 调用 `maxF64` 并返回 `f64`。 |
| `new` | `(x: f64, y: f64, z: f64): Vec3` | 创建或初始化实例，返回 `Vec3`。 |
| `zero` | `(): Vec3` | 调用 `zero` 并返回 `Vec3`。 |
| `splat` | `(v: f64): Vec3` | 调用 `splat` 并返回 `Vec3`。 |
| `unitX` | `(): Vec3` | 调用 `unitX` 并返回 `Vec3`。 |
| `unitY` | `(): Vec3` | 调用 `unitY` 并返回 `Vec3`。 |
| `unitZ` | `(): Vec3` | 调用 `unitZ` 并返回 `Vec3`。 |
| `add` | `(a: Vec3, b: Vec3): Vec3` | 写入或更新状态数据。 |
| `sub` | `(a: Vec3, b: Vec3): Vec3` | 调用 `sub` 并返回 `Vec3`。 |
| `mul` | `(a: Vec3, b: Vec3): Vec3` | 调用 `mul` 并返回 `Vec3`。 |
| `scale` | `(v: Vec3, s: f64): Vec3` | 调用 `scale` 并返回 `Vec3`。 |
| `neg` | `(v: Vec3): Vec3` | 调用 `neg` 并返回 `Vec3`。 |
| `dot` | `(a: Vec3, b: Vec3): f64` | 调用 `dot` 并返回 `f64`。 |
| `cross` | `(a: Vec3, b: Vec3): Vec3` | 调用 `cross` 并返回 `Vec3`。 |
| `lengthSq` | `(v: Vec3): f64` | 调用 `lengthSq` 并返回 `f64`。 |
| `length` | `(v: Vec3): f64` | 调用 `length` 并返回 `f64`。 |
| `distance` | `(a: Vec3, b: Vec3): f64` | 调用 `distance` 并返回 `f64`。 |
| `normalize` | `(v: Vec3): Vec3` | 调用 `normalize` 并返回 `Vec3`。 |
| `lerp` | `(a: Vec3, b: Vec3, t: f64): Vec3` | 调用 `lerp` 并返回 `Vec3`。 |
| `reflect` | `(v: Vec3, normal: Vec3): Vec3` | 调用 `reflect` 并返回 `Vec3`。 |
| `project` | `(v: Vec3, onto: Vec3): Vec3` | 调用 `project` 并返回 `Vec3`。 |
| `minVec` | `(a: Vec3, b: Vec3): Vec3` | 调用 `minVec` 并返回 `Vec3`。 |
| `maxVec` | `(a: Vec3, b: Vec3): Vec3` | 调用 `maxVec` 并返回 `Vec3`。 |
| `clampLength` | `(v: Vec3, max_len: f64): Vec3` | 调用 `clampLength` 并返回 `Vec3`。 |
| `approxEq` | `(a: Vec3, b: Vec3, eps: f64): bool` | 调用 `approxEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { sqrtF64 } from "gl-matrix";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
