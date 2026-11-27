[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# approx

零堆分配的 IEEE-754 浮点数近似相等比较库（支持 `f64` / `f32`），完整提供绝对误差（`absDiffEq`）、相对误差（`relativeEq`）、ULPs 距离（`ulpsEq`）、默认容差快捷函数与切片批量比较。净室移植自 Rust `approx`。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `approx` (`brendanzab/approx`) |
| **CP 包名** | `approx` (`v1.0.0`) |
| **许可证** | MIT（上游 Apache-2.0） |

CP 生态直接沿用原包名 `approx`，无需 `@cap/*` 前缀或重命名：

```rust
// Rust (crates.io: approx)
use approx::{abs_diff_eq, relative_eq, ulps_eq};
assert!(relative_eq!(1.0_f64, 1.0 + 1e-12, epsilon = 1e-12, max_relative = 1e-12));
```

```typescript
// CP（保持原包名 "approx"）
import { absDiffEq, relativeEq, ulpsEq } from "approx";
let ok = relativeEq(1.0, 1.0 + 1e-12, 1e-12, 1e-12);
```

## 安装

```bash
cap add approx
```

## 快速上手与实战示例

### 1. 绝对误差、相对误差与 ULPs 比较（`f64` / `f32`）

```typescript
import {
    F64_EPSILON,
    F32_EPSILON,
    DEFAULT_MAX_ULPS_F64,
    absDiffEq,
    absDiffNe,
    absDiffEqF32,
    relativeEq,
    relativeEqF32,
    ulpsEq,
    ulpsDistanceF64,
    ulpsEqF32,
    ulpsDistanceF32,
} from "approx";

fn compareScalars(): void {
    // 绝对误差：|a - b| <= epsilon
    let a_ok = absDiffEq(0.1 + 0.2, 0.3, 1e-12);
    let a_ne = absDiffNe(1.0, 2.0, 1e-6);

    // 相对误差：|a - b| <= epsilon || |a - b| <= max_relative * max(|a|, |b|)
    let r_ok = relativeEq(1000.0, 1000.0001, F64_EPSILON, 1e-6);
    let r32_ok = relativeEqF32(10.0 as f32, 10.00001 as f32, F32_EPSILON, 1e-5 as f32);

    // ULPs 比较：IEEE-754 可表示浮点数步长距离
    let u_ok = ulpsEq(1.0, 1.0 + F64_EPSILON, F64_EPSILON, DEFAULT_MAX_ULPS_F64);
    if (let dist = ulpsDistanceF64(1.0, 1.0 + F64_EPSILON)) {
        // dist == 1
    }
    let dist32 = ulpsDistanceF32(1.0 as f32, (1.0 as f32) + F32_EPSILON);
    let u32_ok = ulpsEqF32(1.0 as f32, 1.0 as f32, F32_EPSILON, 4);
}
```

### 2. 默认容差快捷函数与 NaN/Inf 检测

```typescript
import {
    absDiffEqDefaultF64,
    relativeEqDefaultF64,
    ulpsEqDefaultF64,
    apxAbsF64,
    apxIsNanF64,
    apxIsInfF64,
} from "approx";

fn checkDefaults(x: f64, y: f64): bool {
    if (apxIsNanF64(x) || apxIsInfF64(x)) {
        return false;
    }
    let mag = apxAbsF64(x);
    return absDiffEqDefaultF64(x, y) || relativeEqDefaultF64(x, y) || ulpsEqDefaultF64(x, y);
}
```

### 3. 切片逐元素比较

```typescript
import {
    absDiffEqSliceF64,
    relativeEqSliceF64,
    ulpsEqSliceF64,
} from "approx";

fn compareVectors(): bool {
    let a: [3]f64 = [1.0, 2.0, 3.0];
    let b: [3]f64 = [1.0 + 1e-13, 2.0 - 1e-13, 3.0];
    return absDiffEqSliceF64(a[0..3], b[0..3], 1e-12)
        && relativeEqSliceF64(a[0..3], b[0..3], 1e-12, 1e-12)
        && ulpsEqSliceF64(a[0..3], b[0..3], 1e-12, 4);
}
```

## API 速览

### 常量
- `F64_EPSILON: f64`（`2.220446049250313e-16`）、`F32_EPSILON: f32`（`1.1920929e-7`）
- `DEFAULT_MAX_ULPS_F64: u64`（`4`）、`DEFAULT_MAX_ULPS_F32: u32`（`4`）

### 基础工具
- `apxAbsF64` / `apxAbsF32`、`apxIsNanF64` / `apxIsNanF32`、`apxIsInfF64` / `apxIsInfF32`

### 标量比较（`f64` 默认别名省略 `F64` 后缀）
- `absDiffEq` / `absDiffEqF64`、`absDiffNe` / `absDiffNeF64`、`absDiffEqF32`、`absDiffNeF32`
- `relativeEq` / `relativeEqF64`、`relativeNe` / `relativeNeF64`、`relativeEqF32`、`relativeNeF32`
- `ulpsDistanceF64(a, b): u64?`、`ulpsDistanceF32(a, b): u32?`
- `ulpsEq` / `ulpsEqF64`、`ulpsNe` / `ulpsNeF64`、`ulpsEqF32`、`ulpsNeF32`
- 默认容差快捷函数：`absDiffEqDefaultF64/F32`、`relativeEqDefaultF64/F32`、`ulpsEqDefaultF64/F32`

### 切片比较
- `absDiffEqSliceF64` / `absDiffEqSliceF32`
- `relativeEqSliceF64` / `relativeEqSliceF32`
- `ulpsEqSliceF64` / `ulpsEqSliceF32`
