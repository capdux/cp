[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# complex.js

Zero-allocation complex number arithmetic, polar form, exponential, integer powers, and principal square root for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:complex.js` ([https://github.com/rawify/Complex.js](https://github.com/rawify/Complex.js))（原库名 `complex.js` → CP 包名 `complex.js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `complex.js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add complex.js
```

```cp
import {
    Complex,
    absF64,
    sqrtF64,
    reduceAngle,
    sinF64,
    cosF64,
    expF64,
    atanSmall,
} from "complex.js";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Complex` | `class` | 字段: `re: f64, im: f64` · 方法: `new, zero, one, iUnit, fromPolar, add, +12 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `reduceAngle` | `(rad: f64): f64` | 调用 `reduceAngle` 并返回 `f64`。 |
| `sinF64` | `(rad: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(rad: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |
| `expF64` | `(x: f64): f64` | 调用 `expF64` 并返回 `f64`。 |
| `atanSmall` | `(x: f64): f64` | 调用 `atanSmall` 并返回 `f64`。 |
| `atanF64` | `(x: f64): f64` | 调用 `atanF64` 并返回 `f64`。 |
| `atan2F64` | `(y: f64, x: f64): f64` | 调用 `atan2F64` 并返回 `f64`。 |
| `new` | `(re: f64, im: f64): Complex` | 创建或初始化实例，返回 `Complex`。 |
| `zero` | `(): Complex` | 调用 `zero` 并返回 `Complex`。 |
| `one` | `(): Complex` | 调用 `one` 并返回 `Complex`。 |
| `iUnit` | `(): Complex` | 调用 `iUnit` 并返回 `Complex`。 |
| `fromPolar` | `(r: f64, theta: f64): Complex` | 调用 `fromPolar` 并返回 `Complex`。 |
| `add` | `(a: Complex, b: Complex): Complex` | 写入或更新状态数据。 |
| `sub` | `(a: Complex, b: Complex): Complex` | 调用 `sub` 并返回 `Complex`。 |
| `mul` | `(a: Complex, b: Complex): Complex` | 调用 `mul` 并返回 `Complex`。 |
| `div` | `(a: Complex, b: Complex): Complex` | 调用 `div` 并返回 `Complex`。 |
| `conj` | `(z: Complex): Complex` | 调用 `conj` 并返回 `Complex`。 |
| `neg` | `(z: Complex): Complex` | 调用 `neg` 并返回 `Complex`。 |
| `scale` | `(z: Complex, s: f64): Complex` | 调用 `scale` 并返回 `Complex`。 |
| `normSq` | `(z: Complex): f64` | 调用 `normSq` 并返回 `f64`。 |
| `norm` | `(z: Complex): f64` | 调用 `norm` 并返回 `f64`。 |
| `arg` | `(z: Complex): f64` | 调用 `arg` 并返回 `f64`。 |
| `exp` | `(z: Complex): Complex` | 调用 `exp` 并返回 `Complex`。 |
| `powInt` | `(z: Complex, n: i32): Complex` | 调用 `powInt` 并返回 `Complex`。 |
| `sqrt` | `(z: Complex): Complex` | 调用 `sqrt` 并返回 `Complex`。 |
| `approxEq` | `(a: Complex, b: Complex, eps: f64): bool` | 调用 `approxEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { absF64 } from "complex.js";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
