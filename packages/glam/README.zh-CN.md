[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# glam

Zero-heap 3D math library with Vec3, unit Quat, and column-major Mat4 (Rust glam parity, pure CP with built-in sqrt/sin/cos).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:glam` ([https://github.com/bitshifter/glam-rs](https://github.com/bitshifter/glam-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `glam` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add glam
```

```cp
import {
    Vec3,
    Quat,
    Mat4,
    absF64,
    sqrtF64,
    sinF64,
    cosF64,
} from "glam";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Vec3` | `class` | 字段: `x: f64, y: f64, z: f64` · 方法: `init, add, sub, scale, dot, cross, +5 项` |
| `Quat` | `class` | 字段: `x: f64, y: f64, z: f64, w: f64` · 方法: `identity, fromAxisAngle, normalize, conjugate, mul, rotateVec3` |
| `Mat4` | `class` | 字段: `m: [16]f64` · 方法: `identity, translation, scaling, rotationZ, mul, transformPoint, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `sinF64` | `(angle: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(angle: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |

## 最小调用示例

```cp
import { absF64 } from "glam";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
