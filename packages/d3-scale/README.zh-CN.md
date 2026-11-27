[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-scale

Zero-allocation D3 quantitative scales (LinearScale, PowScale, SqrtScale, LogScale, QuantizeScale) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-scale` ([https://github.com/d3/d3-scale](https://github.com/d3/d3-scale))

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-scale` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-scale
```

```cp
import {
    LinearScale,
    PowScale,
    SqrtScale,
    LogScale,
    QuantizeScale,
    d3scClamp01,
    d3scLerp,
    d3scSignedPow,
} from "d3-scale";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LinearScale` | `class` | 字段: `d0: f64, d1: f64, r0: f64, r1: f64, clamped: bool` · 方法: `clamp, apply, invert, ticksCount, ticks` |
| `PowScale` | `class` | 字段: `d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64, clamped: bool` · 方法: `clamp, apply, invert` |
| `SqrtScale` | `class` | 字段: `d0: f64, d1: f64, r0: f64, r1: f64, clamped: bool` · 方法: `clamp, apply, invert` |
| `LogScale` | `class` | 字段: `d0: f64, d1: f64, r0: f64, r1: f64, base: f64, clamped: bool` · 方法: `clamp, apply, invert` |
| `QuantizeScale` | `class` | 字段: `d0: f64, d1: f64, buckets: usize` · 方法: `applyIndex, applyValue, invertExtentMin, invertExtentMax` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `d3scClamp01` | `(t: f64): f64` | 调用 `d3scClamp01` 并返回 `f64`。 |
| `d3scLerp` | `(a: f64, b: f64, t: f64): f64` | 调用 `d3scLerp` 并返回 `f64`。 |
| `d3scSignedPow` | `(x: f64, exp: f64): f64` | 调用 `d3scSignedPow` 并返回 `f64`。 |
| `d3scTickIncrement` | `(start: f64, stop: f64, count: usize): f64` | 调用 `d3scTickIncrement` 并返回 `f64`。 |
| `linearScale` | `(d0: f64, d1: f64, r0: f64, r1: f64): LinearScale` | 调用 `linearScale` 并返回 `LinearScale`。 |
| `powScale` | `(d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64): PowScale` | 调用 `powScale` 并返回 `PowScale`。 |
| `sqrtScale` | `(d0: f64, d1: f64, r0: f64, r1: f64): SqrtScale` | 调用 `sqrtScale` 并返回 `SqrtScale`。 |
| `logScale` | `(d0: f64, d1: f64, r0: f64, r1: f64, base: f64): LogScale` | 调用 `logScale` 并返回 `LogScale`。 |
| `quantizeScale` | `(d0: f64, d1: f64, buckets: usize): QuantizeScale` | 调用 `quantizeScale` 并返回 `QuantizeScale`。 |

## 最小调用示例

```cp
import { d3scClamp01 } from "d3-scale";

pub fn main() {
    let result = d3scClamp01(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
