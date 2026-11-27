[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# nalgebra

Zero-heap 2D/3D/4D linear algebra: Mat2/Mat3/Mat4 multiply, determinant, inverse, affine transforms & vector ops (crates.io nalgebra equivalent, reuses mint)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:nalgebra` ([https://github.com/dimforge/nalgebra](https://github.com/dimforge/nalgebra))

| 元信息 | 值 |
| --- | --- |
| 包名 | `nalgebra` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add nalgebra
```

```cp
import {
    naSqrt,
    vec2Add,
    vec2Sub,
    vec2Scale,
    vec2Norm,
    vec3Add,
    vec3Sub,
    vec3Scale,
} from "nalgebra";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `naSqrt` | `(x: f64): f64` | 调用 `naSqrt` 并返回 `f64`。 |
| `vec2Add` | `(a: Vector2, b: Vector2): Vector2` | 调用 `vec2Add` 并返回 `Vector2`。 |
| `vec2Sub` | `(a: Vector2, b: Vector2): Vector2` | 调用 `vec2Sub` 并返回 `Vector2`。 |
| `vec2Scale` | `(v: Vector2, s: f64): Vector2` | 调用 `vec2Scale` 并返回 `Vector2`。 |
| `vec2Norm` | `(v: Vector2): f64` | 调用 `vec2Norm` 并返回 `f64`。 |
| `vec3Add` | `(a: Vector3, b: Vector3): Vector3` | 调用 `vec3Add` 并返回 `Vector3`。 |
| `vec3Sub` | `(a: Vector3, b: Vector3): Vector3` | 调用 `vec3Sub` 并返回 `Vector3`。 |
| `vec3Scale` | `(v: Vector3, s: f64): Vector3` | 调用 `vec3Scale` 并返回 `Vector3`。 |
| `vec3Norm` | `(v: Vector3): f64` | 调用 `vec3Norm` 并返回 `f64`。 |
| `vec3Normalize` | `(v: Vector3): Vector3?` | 调用 `vec3Normalize` 并返回 `Vector3?`。 |
| `vec3Lerp` | `(a: Vector3, b: Vector3, t: f64): Vector3` | 调用 `vec3Lerp` 并返回 `Vector3`。 |
| `vec3Distance` | `(a: Vector3, b: Vector3): f64` | 调用 `vec3Distance` 并返回 `f64`。 |
| `mat2Mul` | `(a: RowMatrix2, b: RowMatrix2): RowMatrix2` | 调用 `mat2Mul` 并返回 `RowMatrix2`。 |
| `mat2Inverse` | `(m: RowMatrix2): RowMatrix2?` | 调用 `mat2Inverse` 并返回 `RowMatrix2?`。 |
| `mat3Det` | `(m: RowMatrix3): f64` | 调用 `mat3Det` 并返回 `f64`。 |
| `mat3Mul` | `(a: RowMatrix3, b: RowMatrix3): RowMatrix3` | 调用 `mat3Mul` 并返回 `RowMatrix3`。 |
| `mat3Inverse` | `(m: RowMatrix3): RowMatrix3?` | 调用 `mat3Inverse` 并返回 `RowMatrix3?`。 |
| `mat4Mul` | `(a: RowMatrix4, b: RowMatrix4): RowMatrix4` | 调用 `mat4Mul` 并返回 `RowMatrix4`。 |
| `mat4Translation` | `(tx: f64, ty: f64, tz: f64): RowMatrix4` | 调用 `mat4Translation` 并返回 `RowMatrix4`。 |
| `mat4Scaling` | `(sx: f64, sy: f64, sz: f64): RowMatrix4` | 调用 `mat4Scaling` 并返回 `RowMatrix4`。 |
| `mat4TransformPoint` | `(m: RowMatrix4, p: Vector3): Vector3` | 调用 `mat4TransformPoint` 并返回 `Vector3`。 |

## 最小调用示例

```cp
import { naSqrt } from "nalgebra";

pub fn main() {
    let result = naSqrt(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
