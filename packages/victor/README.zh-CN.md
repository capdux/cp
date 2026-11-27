[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# victor

Zero-allocation 2D vector math and planar geometry library with built-in pure-CP sqrt/sin/cos for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:victor` ([https://github.com/maxkueng/victor](https://github.com/maxkueng/victor)) / `crates.io:euclid` ([https://github.com/servo/euclid](https://github.com/servo/euclid))（原库名 `victor` → CP 包名 `victor`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `victor` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add victor
```

```cp
import {
    Vec2,
    v2Abs,
    v2Sqrt,
    v2WrapPi,
    v2Sin,
    v2Cos,
    new,
    zero,
} from "victor";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Vec2` | `class` | 字段: `x: f64, y: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `v2Abs` | `(x: f64): f64` | 调用 `v2Abs` 并返回 `f64`。 |
| `v2Sqrt` | `(x: f64): f64` | 调用 `v2Sqrt` 并返回 `f64`。 |
| `v2WrapPi` | `(rad: f64): f64` | 调用 `v2WrapPi` 并返回 `f64`。 |
| `v2Sin` | `(rad: f64): f64` | 调用 `v2Sin` 并返回 `f64`。 |
| `v2Cos` | `(rad: f64): f64` | 调用 `v2Cos` 并返回 `f64`。 |
| `new` | `(x: f64, y: f64): Vec2` | 创建或初始化实例，返回 `Vec2`。 |
| `zero` | `(): Vec2` | 调用 `zero` 并返回 `Vec2`。 |
| `splat` | `(v: f64): Vec2` | 调用 `splat` 并返回 `Vec2`。 |
| `add` | `(a: Vec2, b: Vec2): Vec2` | 写入或更新状态数据。 |
| `sub` | `(a: Vec2, b: Vec2): Vec2` | 调用 `sub` 并返回 `Vec2`。 |
| `mul` | `(a: Vec2, b: Vec2): Vec2` | 调用 `mul` 并返回 `Vec2`。 |
| `scale` | `(v: Vec2, s: f64): Vec2` | 调用 `scale` 并返回 `Vec2`。 |
| `neg` | `(v: Vec2): Vec2` | 调用 `neg` 并返回 `Vec2`。 |
| `dot` | `(a: Vec2, b: Vec2): f64` | 调用 `dot` 并返回 `f64`。 |
| `cross` | `(a: Vec2, b: Vec2): f64` | 调用 `cross` 并返回 `f64`。 |
| `lengthSq` | `(v: Vec2): f64` | 调用 `lengthSq` 并返回 `f64`。 |
| `length` | `(v: Vec2): f64` | 调用 `length` 并返回 `f64`。 |
| `distance` | `(a: Vec2, b: Vec2): f64` | 调用 `distance` 并返回 `f64`。 |
| `normalize` | `(v: Vec2): Vec2` | 调用 `normalize` 并返回 `Vec2`。 |
| `lerp` | `(a: Vec2, b: Vec2, t: f64): Vec2` | 调用 `lerp` 并返回 `Vec2`。 |
| `perp` | `(v: Vec2): Vec2` | 调用 `perp` 并返回 `Vec2`。 |
| `reflect` | `(v: Vec2, normal: Vec2): Vec2` | 调用 `reflect` 并返回 `Vec2`。 |
| `project` | `(a: Vec2, onto: Vec2): Vec2` | 调用 `project` 并返回 `Vec2`。 |
| `rotate` | `(v: Vec2, radians: f64): Vec2` | 调用 `rotate` 并返回 `Vec2`。 |
| `clampLength` | `(v: Vec2, max_len: f64): Vec2` | 调用 `clampLength` 并返回 `Vec2`。 |
| `approxEq` | `(a: Vec2, b: Vec2, eps: f64): bool` | 调用 `approxEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { v2Abs } from "victor";

pub fn main() {
    let result = v2Abs(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
