[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# approx

纯 CP 实现的零堆 IEEE-754 浮点数近似相等比较库，完整提供 `f64`、`f32` 与切片的 `abs_diff_eq`、`relative_eq` 与 `ulps_eq` 比较能力。

- **源包对齐**：参考 Rust [`approx`](https://github.com/brendanzab/approx)（`crates.io`，`Apache-2.0`）净室重构。
- **包版本**：`1.0.0`（`MIT`）。

## 安装与导入

```bash
cap add approx
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "approx": "^1.0.0"
  }
}
```

```typescript
import {
    F64_EPSILON, F32_EPSILON, DEFAULT_MAX_ULPS_F64, DEFAULT_MAX_ULPS_F32,
    absDiffEq, absDiffNe, absDiffEqF64, absDiffNeF64, absDiffEqF32, absDiffNeF32,
    relativeEq, relativeNe, relativeEqF64, relativeNeF64, relativeEqF32, relativeNeF32,
    ulpsEq, ulpsNe, ulpsEqF64, ulpsNeF64, ulpsEqF32, ulpsNeF32,
    ulpsDistanceF64, ulpsDistanceF32,
    absDiffEqDefaultF64, absDiffEqDefaultF32,
    relativeEqDefaultF64, relativeEqDefaultF32,
    ulpsEqDefaultF64, ulpsEqDefaultF32,
    absDiffEqSliceF64, absDiffEqSliceF32,
    relativeEqSliceF64, relativeEqSliceF32,
    ulpsEqSliceF64, ulpsEqSliceF32
} from "approx";
```

## 核心 API 参考

### 常量

| 常量 | 类型 | 值 | 说明 |
| :--- | :--- | :--- | :--- |
| `F64_EPSILON` | `f64` | `2.220446049250313e-16` | IEEE-754 `f64` 机器精度 |
| `F32_EPSILON` | `f32` | `1.1920929e-7` | IEEE-754 `f32` 机器精度 |
| `DEFAULT_MAX_ULPS_F64` | `u64` | `4` | `ulpsEqDefaultF64` 默认 ULP 容差 |
| `DEFAULT_MAX_ULPS_F32` | `u32` | `4` | `ulpsEqDefaultF32` 默认 ULP 容差 |

### 标量与默认容差比较

| 系列 | 函数 | 说明 |
| :--- | :--- | :--- |
| 绝对误差（`f64`/`f32`） | `absDiffEq` / `absDiffNe` / `absDiffEqF64` / `absDiffNeF64` / `absDiffEqF32` / `absDiffNeF32` | 判断 `\|a - b\| <= epsilon`（同号无穷大相等，`NaN` 恒为 `false`） |
| 相对误差（`f64`/`f32`） | `relativeEq` / `relativeNe` / `relativeEqF64` / `relativeNeF64` / `relativeEqF32` / `relativeNeF32` | 先判绝对误差，再判 `\|a - b\| <= max_relative * max(\|a\|, \|b\|)` |
| ULP 误差（`f64`/`f32`） | `ulpsEq` / `ulpsNe` / `ulpsEqF64` / `ulpsNeF64` / `ulpsEqF32` / `ulpsNeF32` | 先判绝对误差，再判 IEEE-754 ULP 距离 `<= max_ulps` |
| ULP 距离 | `ulpsDistanceF64(a: f64, b: f64): u64?` / `ulpsDistanceF32(a: f32, b: f32): u32?` | 返回同号有限浮点数之间的精确 ULP 距离；`NaN`/无穷/异号返回 `null` |
| 默认容差快捷版 | `absDiffEqDefaultF64`、`absDiffEqDefaultF32`、`relativeEqDefaultF64`、`relativeEqDefaultF32`、`ulpsEqDefaultF64`、`ulpsEqDefaultF32` | 自动使用 `F64_EPSILON`/`F32_EPSILON` 与 `DEFAULT_MAX_ULPS_*` |

### 切片逐元素比较

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `absDiffEqSliceF64` / `absDiffEqSliceF32` | `(a: &[]f64, b: &[]f64, epsilon: f64): bool` | 逐元素绝对误差比较（要求长度相等） |
| `relativeEqSliceF64` / `relativeEqSliceF32` | `(a: &[]f64, b: &[]f64, epsilon: f64, max_relative: f64): bool` | 逐元素相对误差比较 |
| `ulpsEqSliceF64` / `ulpsEqSliceF32` | `(a: &[]f64, b: &[]f64, epsilon: f64, max_ulps: u64): bool` | 逐元素 ULP 误差比较 |

## 使用示例

### 1. 标量浮点近似比较

```typescript
import { absDiffEq, relativeEq, ulpsEq, ulpsDistanceF64 } from "approx";

fn verifyMath(): bool {
    let ok1 = absDiffEq(0.1 + 0.2, 0.3, 1.0e-12);
    let ok2 = relativeEq(1000.0001, 1000.0002, 1.0e-9, 1.0e-6);
    let ok3 = ulpsEq(1.0, 1.0 + 2.220446049250313e-16, 0.0, 2);
    let dist = ulpsDistanceF64(1.0, 1.0) orelse 999;
    return ok1 && ok2 && ok3 && dist == 0;
}
```

### 2. 数值向量切片比较

```typescript
import { relativeEqSliceF64, F64_EPSILON } from "approx";

fn checkVector(): bool {
    let expected = [1.0, 2.0, 3.0];
    let actual = [1.0 + 1.0e-12, 2.0 - 1.0e-12, 3.0 + 1.0e-12];
    return relativeEqSliceF64(&expected, &actual, F64_EPSILON, 1.0e-9);
}
```

## 与源包差异与 CP 安全亮点

- **零宏强类型函数**：无需 Rust 宏，直接提供 `f64`（无后缀默认别名）、`f32` 及只读切片 `&[]f64` / `&[]f32` 全套函数。
- **可选类型 ULP 距离**：`ulpsDistanceF64` / `ulpsDistanceF32` 返回 `u64?` / `u32?`，安全区分 `NaN`、无穷大与异号输入。
- **零依赖零堆**：纯 IEEE-754 位运算实现，适用于数值测试、矩阵/张量校验与物理仿真。
