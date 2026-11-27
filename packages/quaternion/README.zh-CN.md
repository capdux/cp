[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# quaternion

Zero-heap 3D unit quaternion rotation, SLERP/NLERP, and Euler conversion in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:quaternion` ([https://github.com/rawify/Quaternion.js](https://github.com/rawify/Quaternion.js))（原库名 `quaternion` → CP 包名 `quaternion`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `quaternion` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add quaternion
```

```cp
import {
    Quat,
    Point3,
    absF64,
    sqrtF64,
    wrapAngle,
    sinF64,
    cosF64,
    atanF64,
} from "quaternion";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Quat` | `class` | 字段: `x: f64, y: f64, z: f64, w: f64` |
| `Point3` | `class` | 字段: `x: f64, y: f64, z: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `wrapAngle` | `(x: f64): f64` | 调用 `wrapAngle` 并返回 `f64`。 |
| `sinF64` | `(x: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(x: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |
| `atanF64` | `(x: f64): f64` | 调用 `atanF64` 并返回 `f64`。 |
| `acosF64` | `(x: f64): f64` | 调用 `acosF64` 并返回 `f64`。 |
| `new` | `(x: f64, y: f64, z: f64, w: f64): Quat` | 创建或初始化实例，返回 `Quat`。 |
| `identity` | `(): Quat` | 调用 `identity` 并返回 `Quat`。 |
| `fromAxisAngle` | `(ax: f64, ay: f64, az: f64, radians: f64): Quat` | 调用 `fromAxisAngle` 并返回 `Quat`。 |
| `fromEulerXYZ` | `(rx: f64, ry: f64, rz: f64): Quat` | 调用 `fromEulerXYZ` 并返回 `Quat`。 |
| `mul` | `(a: Quat, b: Quat): Quat` | 调用 `mul` 并返回 `Quat`。 |
| `conjugate` | `(q: Quat): Quat` | 调用 `conjugate` 并返回 `Quat`。 |
| `dot` | `(a: Quat, b: Quat): f64` | 调用 `dot` 并返回 `f64`。 |
| `length` | `(q: Quat): f64` | 调用 `length` 并返回 `f64`。 |
| `inverse` | `(q: Quat): Quat` | 调用 `inverse` 并返回 `Quat`。 |
| `normalize` | `(q: Quat): Quat` | 调用 `normalize` 并返回 `Quat`。 |
| `nlerp` | `(a: Quat, b: Quat, t: f64): Quat` | 调用 `nlerp` 并返回 `Quat`。 |
| `slerp` | `(a: Quat, b: Quat, t: f64): Quat` | 调用 `slerp` 并返回 `Quat`。 |
| `rotatePoint` | `(q: Quat, p: Point3): Point3` | 调用 `rotatePoint` 并返回 `Point3`。 |
| `approxEq` | `(a: Quat, b: Quat, eps: f64): bool` | 调用 `approxEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { absF64 } from "quaternion";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
