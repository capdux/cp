[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# float-cmp

纯 CP 实现的零堆分配 IEEE-754 浮点数近似相等、ULPs 距离、相邻浮点步进、比例比较与近似排序库。

- **包名**：`float-cmp`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `crates.io/float-cmp`（`MIT`），底层复用 `approx` 的 IEEE-754 epsilon 与 ULP 原语。

## 安装与导入

```bash
cap add float-cmp
```

```typescript
import {
    F64Margin, F32Margin,
    approxEqF64, approxNeF64, approxOrdF64, approxEqRatioF64,
    ulpsDiffF64, nextUpF64, nextDownF64,
} from "float-cmp";
```

## 核心 API

### 容差构造器（Margin）

| 类型 | 字段与方法 | 说明 |
| --- | --- | --- |
| `F64Margin` | `epsilon: f64`, `ulps: u64`；`defaultMargin()`, `zero()`, `fromTuple(epsilon, ulps)`, `withEpsilon(eps)`, `withUlps(u)` | `f64` 双重容差配置（绝对 epsilon + ULPs）。 |
| `F32Margin` | `epsilon: f32`, `ulps: u32`；`defaultMargin()`, `zero()`, `fromTuple(epsilon, ulps)`, `withEpsilon(eps)`, `withUlps(u)` | `f32` 双重容差配置（绝对 epsilon + ULPs）。 |

### 近似相等、比例与排序

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `approxEqF64` / `approxNeF64` | `(a: f64, b: f64, margin: F64Margin): bool` | 基于 `epsilon` 与 `ulps` 双重容差判定 `f64` 近似相等/不等。 |
| `approxEqF32` / `approxNeF32` | `(a: f32, b: f32, margin: F32Margin): bool` | 基于 `epsilon` 与 `ulps` 双重容差判定 `f32` 近似相等/不等。 |
| `approxEqRatioF64` | `(a: f64, b: f64, ratio: f64): bool` | 检查 `|a - b| <= max(|a|, |b|) * ratio`（`f64`）。 |
| `approxEqRatioF32` | `(a: f32, b: f32, ratio: f32): bool` | 检查 `|a - b| <= max(|a|, |b|) * ratio`（`f32`）。 |
| `approxOrdF64` | `(a: f64, b: f64, margin: F64Margin): i32` | 近似相等返回 `0`，`a < b` 返回 `-1`，`a > b` 返回 `1`。 |
| `approxOrdF32` | `(a: f32, b: f32, margin: F32Margin): i32` | 近似相等返回 `0`，`a < b` 返回 `-1`，`a > b` 返回 `1`。 |

### IEEE-754 分类、ULPs 与相邻浮点

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `fcmpIsNanF64` / `fcmpIsNanF32` | `(x: f64/f32): bool` | 判断是否为 NaN。 |
| `fcmpIsInfF64` / `fcmpIsInfF32` | `(x: f64/f32): bool` | 判断是否为 `+inf` 或 `-inf`。 |
| `f64ToOrderedI64` / `f32ToOrderedI32` | `(x: f64): i64` / `(x: f32): i32` | 将有限 IEEE-754 位模式映射为保序有符号整数。 |
| `ulpsDiffF64` / `ulpsDiffF32` | `(a, b): i64?` / `i32?` | 计算有符号 ULP 差值（`a - b`）；遇 NaN/Inf 返回 `null`。 |
| `nextUpF64` / `nextDownF64` | `(x: f64): f64` | 朝 `+inf` / `-inf` 步进至下一个可表示的 `f64`（别名 `nextUp`, `nextDown`）。 |
| `nextUpF32` / `nextDownF32` | `(x: f32): f32` | 朝 `+inf` / `-inf` 步进至下一个可表示的 `f32`。 |

## 使用示例

### 1. 双重容差比较与排序

```typescript
import { F64Margin, approxEqF64, approxOrdF64 } from "float-cmp";

fn main() {
    let m = F64Margin.defaultMargin().withUlps(4);
    let ok = approxEqF64(0.1 + 0.2, 0.3, m); // true
    let ord = approxOrdF64(1.0, 2.0, m);     // -1
}
```

### 2. ULPs 距离与相邻浮点步进

```typescript
import { nextUpF64, ulpsDiffF64 } from "float-cmp";

fn main() {
    let a: f64 = 1.0;
    let b = nextUpF64(nextUpF64(a));
    if (let diff = ulpsDiffF64(b, a)) {
        // diff == 2
    }
}
```

### 3. 比例容差比较

```typescript
import { approxEqRatioF64, approxEqRatioF32 } from "float-cmp";

fn main() {
    let close64 = approxEqRatioF64(1000.0, 1000.5, 0.001); // true
    let close32 = approxEqRatioF32(10.0, 10.01, 0.005);    // true
}
```

## 与源包差异及零拷贝安全亮点

- **显式位宽函数**：用 `approxEqF64`/`approxEqF32`、`F64Margin`/`F32Margin` 替代 Rust 的 `ApproxEq` Trait 与 `approx_eq!` 宏。
- **可选类型安全**：`ulpsDiffF64`/`ulpsDiffF32` 返回 CP 可选类型 `i64?`/`i32?`，遇到 NaN 或无穷大时返回 `null`。
- **零堆分配与链式构造**：纯栈值类型，支持 `.withEpsilon()` / `.withUlps()` 不可变链式配置。
