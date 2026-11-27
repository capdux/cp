[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rustfft

Zero-allocation in-place radix-2 Cooley-Tukey FFT, IFFT, magnitude spectrum, and Hann/Hamming windowing (Rust rustfft / microfft parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustfft` ([https://github.com/ejmahler/RustFFT](https://github.com/ejmahler/RustFFT))（原库名 `rustfft` → CP 包名 `rustfft`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rustfft` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rustfft
```

```cp
import {
    Complex,
    sqrtF64,
    sinF64,
    cosF64,
    isPowerOfTwo,
    magnitude,
    fftInPlace,
    ifftInPlace,
} from "rustfft";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Complex` | `class` | 字段: `re: f64, im: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `sinF64` | `(x_in: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(x: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |
| `isPowerOfTwo` | `(n: usize): bool` | 执行 `isPowerOfTwo` 条件判定，返回 `bool`。 |
| `magnitude` | `(c: Complex): f64` | 调用 `magnitude` 并返回 `f64`。 |
| `fftInPlace` | `(buf: []Complex): bool` | 调用 `fftInPlace` 并返回 `bool`。 |
| `ifftInPlace` | `(buf: []Complex): bool` | 调用 `ifftInPlace` 并返回 `bool`。 |
| `applyHannWindow` | `(samples: []f64)` | 就地执行 `applyHannWindow` 操作。 |
| `applyHammingWindow` | `(samples: []f64)` | 就地执行 `applyHammingWindow` 操作。 |

## 最小调用示例

```cp
import { isPowerOfTwo } from "rustfft";

pub fn main() {
    let result = isPowerOfTwo(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
