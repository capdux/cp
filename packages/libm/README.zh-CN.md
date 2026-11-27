[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# libm

Pure CP zero-allocation IEEE 754 f64 math library (libm parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:libm` ([https://github.com/rust-lang/libm](https://github.com/rust-lang/libm))

| 元信息 | 值 |
| --- | --- |
| 包名 | `libm` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add libm
```

```cp
import {
    LmTrigRed,
    lmNan,
    lmInf,
    lmScalePow2,
    fabs,
    trunc,
    floor,
    ceil,
} from "libm";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LmTrigRed` | `class` | 字段: `r: f64, quad: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lmNan` | `(): f64` | 调用 `lmNan` 并返回 `f64`。 |
| `lmInf` | `(): f64` | 调用 `lmInf` 并返回 `f64`。 |
| `lmScalePow2` | `(val: f64, exp2: i64): f64` | 调用 `lmScalePow2` 并返回 `f64`。 |
| `fabs` | `(x: f64): f64` | 调用 `fabs` 并返回 `f64`。 |
| `trunc` | `(x: f64): f64` | 调用 `trunc` 并返回 `f64`。 |
| `floor` | `(x: f64): f64` | 调用 `floor` 并返回 `f64`。 |
| `ceil` | `(x: f64): f64` | 调用 `ceil` 并返回 `f64`。 |
| `round` | `(x: f64): f64` | 调用 `round` 并返回 `f64`。 |
| `fmod` | `(x: f64, y: f64): f64` | 调用 `fmod` 并返回 `f64`。 |
| `copysign` | `(x: f64, y: f64): f64` | 调用 `copysign` 并返回 `f64`。 |
| `fmin` | `(x: f64, y: f64): f64` | 调用 `fmin` 并返回 `f64`。 |
| `fmax` | `(x: f64, y: f64): f64` | 调用 `fmax` 并返回 `f64`。 |
| `sqrt` | `(x: f64): f64` | 调用 `sqrt` 并返回 `f64`。 |
| `cbrt` | `(x: f64): f64` | 调用 `cbrt` 并返回 `f64`。 |
| `hypot` | `(x: f64, y: f64): f64` | 调用 `hypot` 并返回 `f64`。 |
| `exp` | `(x: f64): f64` | 调用 `exp` 并返回 `f64`。 |
| `ln` | `(x: f64): f64` | 调用 `ln` 并返回 `f64`。 |
| `log2` | `(x: f64): f64` | 调用 `log2` 并返回 `f64`。 |
| `log10` | `(x: f64): f64` | 调用 `log10` 并返回 `f64`。 |
| `pow` | `(x: f64, y: f64): f64` | 调用 `pow` 并返回 `f64`。 |
| `lmSinCore` | `(r: f64): f64` | 调用 `lmSinCore` 并返回 `f64`。 |
| `lmCosCore` | `(r: f64): f64` | 调用 `lmCosCore` 并返回 `f64`。 |
| `lmReduceTrig` | `(x: f64): LmTrigRed` | 调用 `lmReduceTrig` 并返回 `LmTrigRed`。 |
| `sin` | `(x: f64): f64` | 调用 `sin` 并返回 `f64`。 |
| `cos` | `(x: f64): f64` | 调用 `cos` 并返回 `f64`。 |
| `tan` | `(x: f64): f64` | 调用 `tan` 并返回 `f64`。 |
| `lmAtanUnit` | `(u: f64): f64` | 调用 `lmAtanUnit` 并返回 `f64`。 |
| `atan` | `(x: f64): f64` | 调用 `atan` 并返回 `f64`。 |
| `atan2` | `(y: f64, x: f64): f64` | 调用 `atan2` 并返回 `f64`。 |
| `asin` | `(x: f64): f64` | 调用 `asin` 并返回 `f64`。 |
| `acos` | `(x: f64): f64` | 调用 `acos` 并返回 `f64`。 |
| `sinh` | `(x: f64): f64` | 调用 `sinh` 并返回 `f64`。 |
| `cosh` | `(x: f64): f64` | 调用 `cosh` 并返回 `f64`。 |
| `tanh` | `(x: f64): f64` | 调用 `tanh` 并返回 `f64`。 |

## 最小调用示例

```cp
import { lmNan } from "libm";

pub fn main() {
    let result = lmNan();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
