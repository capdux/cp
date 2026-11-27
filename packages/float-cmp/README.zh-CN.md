[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# float-cmp

零堆分配的 IEEE-754 浮点数近似相等、比例容差、近似排序（`approxOrd`）、有符号 ULPs 差值及相邻可表示浮点步进（`nextUp` / `nextDown`）库。净室移植自 Rust `float-cmp`。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `float-cmp` (`mikedilger/float-cmp`) |
| **CP 包名** | `float-cmp` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `float-cmp` 不变，无需 `@cap/*` 前缀或改名：

```rust
// Rust (crates.io: float-cmp)
use float_cmp::{approx_eq, F64Margin};
let ok = approx_eq!(f64, 0.1 + 0.2, 0.3, F64Margin { epsilon: 1e-12, ulps: 2 });
```

```typescript
// CP（保持原包名 "float-cmp"）
import { approxEqF64, F64Margin } from "float-cmp";
let margin = F64Margin.fromTuple(1e-12, 2);
let ok = approxEqF64(0.1 + 0.2, 0.3, margin);
```

## 安装

```bash
cap add float-cmp
```

## 快速上手与实战示例

### 1. 双容差配置（`F64Margin` / `F32Margin`）

```typescript
import {
    F64Margin,
    F32Margin,
    approxEqF64,
    approxNeF64,
    approxEqF32,
    approxNeF32,
} from "float-cmp";

fn marginChecks(): void {
    let m_def = F64Margin.defaultMargin();
    let m_zero = F64Margin.zero();
    let m_custom = F64Margin.fromTuple(1e-12, 4);
    let m_eps = F64Margin.withEpsilon(1e-9);
    let m_ulps = F64Margin.withUlps(2);

    let eq = approxEqF64(0.1 + 0.2, 0.3, m_custom);
    let ne = approxNeF64(1.0, 2.0, m_def);

    let m32 = F32Margin.fromTuple(1e-5 as f32, 4);
    let eq32 = approxEqF32(1.0 as f32, 1.000001 as f32, m32);
    let ne32 = approxNeF32(1.0 as f32, 2.0 as f32, m32);
}
```

### 2. 比例容差与近似排序

```typescript
import {
    F64Margin,
    approxEqRatioF64,
    approxEqRatioF32,
    approxOrdF64,
    approxOrdF32,
} from "float-cmp";

fn ratioAndOrder(): void {
    // 比例比较：|a - b| <= ratio * max(|a|, |b|)
    let close = approxEqRatioF64(1000.0, 1000.5, 0.001);
    let close32 = approxEqRatioF32(100.0 as f32, 100.05 as f32, 0.001 as f32);

    // 近似三路排序：-1（小于）、0（近似相等）、+1（大于）
    let m = F64Margin.fromTuple(1e-9, 4);
    let ord = approxOrdF64(1.0, 1.0 + 1e-10, m); // 0
}
```

### 3. 有符号 ULPs 差值与相邻浮点步进（`nextUp` / `nextDown`）

```typescript
import {
    ulpsDiffF64,
    ulpsDiffF32,
    nextUp,
    nextDown,
    nextUpF32,
    nextDownF32,
    f64ToOrderedI64,
    f32ToOrderedI32,
} from "float-cmp";

fn ulpsStepping(): void {
    let up = nextUp(1.0);       // nextUpF64 别名
    let down = nextDown(1.0);   // nextDownF64 别名
    if (let diff = ulpsDiffF64(1.0, up)) {
        // diff == -1
    }
    let up32 = nextUpF32(1.0 as f32);
    let down32 = nextDownF32(1.0 as f32);
    let diff32 = ulpsDiffF32(up32, 1.0 as f32); // +1

    // 保持字典序的整数映射键（可用于基数排序或二进制索引）
    let key64 = f64ToOrderedI64(1.0);
    let key32 = f32ToOrderedI32(1.0 as f32);
}
```

## API 速览

### 容差类
- `F64Margin { epsilon: f64, ulps: u64 }`：`defaultMargin()`、`zero()`、`fromTuple(epsilon, ulps)`、`withEpsilon(eps)`、`withUlps(u)`
- `F32Margin { epsilon: f32, ulps: u32 }`：`defaultMargin()`、`zero()`、`fromTuple(epsilon, ulps)`、`withEpsilon(eps)`、`withUlps(u)`

### 近似比较与排序
- `approxEqF64` / `approxNeF64`、`approxEqF32` / `approxNeF32`
- `approxEqRatioF64` / `approxEqRatioF32`
- `approxOrdF64` / `approxOrdF32`（返回 `-1`、`0`、`+1`）

### ULPs、步进与分类
- `ulpsDiffF64(a, b): i64?`、`ulpsDiffF32(a, b): i32?`
- `nextUp` / `nextUpF64`、`nextDown` / `nextDownF64`、`nextUpF32`、`nextDownF32`
- `f64ToOrderedI64`、`f32ToOrderedI32`、`fcmpIsNanF64/F32`、`fcmpIsInfF64/F32`
