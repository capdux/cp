[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-array

Zero-allocation array statistics, R-7 quantiles, median, variance, deviation, bisection search, and histogram binning (clean-room MIT implementation of npm d3-array, #345 / 153M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-array` ([https://github.com/d3/d3-array](https://github.com/d3/d3-array)) | 许可：`ISC`

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-array` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-array
```

```cp
import {
    ExtentF64,
    d3aIsNan,
    sumF64,
    meanF64,
    extentF64,
    varianceF64,
    deviationF64,
    quantileSorted,
} from "d3-array";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ExtentF64` | `class` | 字段: `valid: bool, min: f64, max: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `d3aIsNan` | `(v: f64): bool` | 调用 `d3aIsNan` 并返回 `bool`。 |
| `sumF64` | `(data: &[]f64): f64` | 调用 `sumF64` 并返回 `f64`。 |
| `meanF64` | `(data: &[]f64): f64?` | 调用 `meanF64` 并返回 `f64?`。 |
| `extentF64` | `(data: &[]f64): ExtentF64` | 调用 `extentF64` 并返回 `ExtentF64`。 |
| `varianceF64` | `(data: &[]f64): f64?` | 调用 `varianceF64` 并返回 `f64?`。 |
| `deviationF64` | `(data: &[]f64): f64?` | 调用 `deviationF64` 并返回 `f64?`。 |
| `quantileSorted` | `(sorted: &[]f64, p: f64): f64?` | 调用 `quantileSorted` 并返回 `f64?`。 |
| `medianSorted` | `(sorted: &[]f64): f64?` | 调用 `medianSorted` 并返回 `f64?`。 |
| `bisectLeft` | `(sorted: &[]f64, x: f64): usize` | 调用 `bisectLeft` 并返回 `usize`。 |
| `bisectRight` | `(sorted: &[]f64, x: f64): usize` | 调用 `bisectRight` 并返回 `usize`。 |
| `cumsumF64` | `(data: &[]f64, out: []f64): usize` | 调用 `cumsumF64` 并返回 `usize`。 |
| `binCounts` | `(data: &[]f64, domain_min: f64, domain_max: f64, out_counts: []usize): usize` | 调用 `binCounts` 并返回 `usize`。 |

## 最小调用示例

```cp
import { d3aIsNan } from "d3-array";

pub fn main() {
    let result = d3aIsNan(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
