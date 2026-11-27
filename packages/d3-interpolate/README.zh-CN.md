[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-interpolate

Zero-allocation numeric, integer, RGB/HSL color, gamma-corrected, and uniform B-spline interpolators

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-interpolate` |
| 版本 | `3.0.1` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-interpolate
```

```cp
import {
    RgbColor,
    HslColor,
    d3iClampU8,
    d3iClamp01,
    interpolateNumber,
    interpolateRound,
    interpolateDiscrete,
    interpolateRgb,
} from "d3-interpolate";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RgbColor` | `class` | 字段: `r: u8, g: u8, b: u8, a: f64` |
| `HslColor` | `class` | 字段: `h: f64, s: f64, l: f64, a: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `d3iClampU8` | `(v: f64): u8` | 调用 `d3iClampU8` 并返回 `u8`。 |
| `d3iClamp01` | `(v: f64): f64` | 调用 `d3iClamp01` 并返回 `f64`。 |
| `interpolateNumber` | `(a: f64, b: f64, t: f64): f64` | 调用 `interpolateNumber` 并返回 `f64`。 |
| `interpolateRound` | `(a: i64, b: i64, t: f64): i64` | 调用 `interpolateRound` 并返回 `i64`。 |
| `interpolateDiscrete` | `(values: &[]f64, t: f64): f64` | 调用 `interpolateDiscrete` 并返回 `f64`。 |
| `interpolateRgb` | `(c0: RgbColor, c1: RgbColor, t: f64): RgbColor` | 调用 `interpolateRgb` 并返回 `RgbColor`。 |
| `interpolateHue` | `(a: f64, b: f64, t: f64): f64` | 调用 `interpolateHue` 并返回 `f64`。 |
| `interpolateHsl` | `(c0: HslColor, c1: HslColor, t: f64): HslColor` | 调用 `interpolateHsl` 并返回 `HslColor`。 |
| `basis` | `(t1: f64, v0: f64, v1: f64, v2: f64, v3: f64): f64` | 调用 `basis` 并返回 `f64`。 |
| `interpolateBasis` | `(values: &[]f64, t: f64): f64` | 调用 `interpolateBasis` 并返回 `f64`。 |
| `interpolatePiecewise` | `(values: &[]f64, t: f64): f64` | 调用 `interpolatePiecewise` 并返回 `f64`。 |

## 最小调用示例

```cp
import { d3iClampU8 } from "d3-interpolate";

pub fn main() {
    let result = d3iClampU8(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
