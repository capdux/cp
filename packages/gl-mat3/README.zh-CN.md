[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# gl-mat3

Zero-allocation 3x3 row-major matrix and 2D affine transform library with built-in pure-CP sin/cos for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:gl-mat3` ([https://github.com/stackgl/gl-mat3](https://github.com/stackgl/gl-mat3)) / `crates.io:cgmath` ([https://github.com/rustgd/cgmath](https://github.com/rustgd/cgmath))（原库名 `gl-mat3` → CP 包名 `gl-mat3`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `gl-mat3` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add gl-mat3
```

```cp
import {
    Mat3,
    Vec2D,
    m3Abs,
    m3WrapPi,
    m3Sin,
    m3Cos,
    zero,
    identity,
} from "gl-mat3";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Mat3` | `class` | 字段: `m: [9]f64` |
| `Vec2D` | `class` | 字段: `x: f64, y: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `m3Abs` | `(x: f64): f64` | 调用 `m3Abs` 并返回 `f64`。 |
| `m3WrapPi` | `(rad: f64): f64` | 调用 `m3WrapPi` 并返回 `f64`。 |
| `m3Sin` | `(rad: f64): f64` | 调用 `m3Sin` 并返回 `f64`。 |
| `m3Cos` | `(rad: f64): f64` | 调用 `m3Cos` 并返回 `f64`。 |
| `zero` | `(): Mat3` | 调用 `zero` 并返回 `Mat3`。 |
| `identity` | `(): Mat3` | 调用 `identity` 并返回 `Mat3`。 |
| `fromTranslation` | `(tx: f64, ty: f64): Mat3` | 调用 `fromTranslation` 并返回 `Mat3`。 |
| `fromScale` | `(sx: f64, sy: f64): Mat3` | 调用 `fromScale` 并返回 `Mat3`。 |
| `fromRotation` | `(radians: f64): Mat3` | 调用 `fromRotation` 并返回 `Mat3`。 |
| `mul` | `(a: Mat3, b: Mat3): Mat3` | 调用 `mul` 并返回 `Mat3`。 |
| `transpose` | `(a: Mat3): Mat3` | 调用 `transpose` 并返回 `Mat3`。 |
| `determinant` | `(a: Mat3): f64` | 调用 `determinant` 并返回 `f64`。 |
| `isInvertible` | `(a: Mat3): bool` | 执行 `isInvertible` 条件判定，返回 `bool`。 |
| `inverse` | `(a: Mat3): Mat3` | 调用 `inverse` 并返回 `Mat3`。 |
| `transformPoint` | `(mat: Mat3, p: Vec2D): Vec2D` | 调用 `transformPoint` 并返回 `Vec2D`。 |
| `transformDir` | `(mat: Mat3, v: Vec2D): Vec2D` | 调用 `transformDir` 并返回 `Vec2D`。 |
| `approxEq` | `(a: Mat3, b: Mat3, eps: f64): bool` | 调用 `approxEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { isInvertible } from "gl-mat3";

pub fn main() {
    let result = isInvertible(/* a: Mat3 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
