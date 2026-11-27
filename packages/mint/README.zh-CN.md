[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mint

Zero-heap Math Interoperability Standard Types: Point2/3, Vector2/3/4, Quaternion, RowMatrix2/3/4, ColumnMatrix4, EulerAngles (crates.io mint equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mint` ([https://github.com/kvark/mint](https://github.com/kvark/mint))

| 元信息 | 值 |
| --- | --- |
| 包名 | `mint` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mint
```

```cp
import {
    Point2,
    Point3,
    Vector2,
    Vector3,
    Vector4,
    Quaternion,
    EulerAngles,
    RowMatrix2,
} from "mint";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Point2` | `class` | 字段: `x: f64, y: f64` · 方法: `new, fromSlice, toArray` |
| `Point3` | `class` | 字段: `x: f64, y: f64, z: f64` · 方法: `new, toArray` |
| `Vector2` | `class` | 字段: `x: f64, y: f64` · 方法: `new, dot, lenSq, toArray` |
| `Vector3` | `class` | 字段: `x: f64, y: f64, z: f64` · 方法: `new, dot, cross, lenSq, toArray` |
| `Vector4` | `class` | 字段: `x: f64, y: f64, z: f64, w: f64` · 方法: `new, dot, toArray` |
| `Quaternion` | `class` | 字段: `v: Vector3, s: f64` · 方法: `identity, new, conjugate, normSq, mul` |
| `EulerAngles` | `class` | 字段: `a: f64, b: f64, c: f64` · 方法: `new` |
| `RowMatrix2` | `class` | 字段: `x: Vector2, y: Vector2` · 方法: `identity, det, transpose` |
| `RowMatrix3` | `class` | 字段: `x: Vector3, y: Vector3, z: Vector3` · 方法: `identity, transpose, mulVec3` |
| `RowMatrix4` | `class` | 字段: `x: Vector4, y: Vector4, z: Vector4, w: Vector4` · 方法: `identity, transpose, mulVec4, toColumnMajor` |
| `ColumnMatrix4` | `class` | 字段: `x: Vector4, y: Vector4, z: Vector4, w: Vector4` · 方法: `identity, toRowMajor` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `point3ToVector3` | `(p: Point3): Vector3` | 调用 `point3ToVector3` 并返回 `Vector3`。 |
| `vector3ToPoint3` | `(v: Vector3): Point3` | 调用 `vector3ToPoint3` 并返回 `Point3`。 |

## 最小调用示例

```cp
import { point3ToVector3 } from "mint";

pub fn main() {
    let result = point3ToVector3(/* p: Point3 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
