[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fft.js

Zero-allocation in-place iterative Cooley-Tukey Radix-2 FFT, IFFT, and magnitude spectrum for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fft.js` ([https://github.com/indutny/fft.js](https://github.com/indutny/fft.js)) / `crates.io:microfft` ([https://gitlab.com/ra_kete/microfft-rs](https://gitlab.com/ra_kete/microfft-rs))（原库名 `fft.js` → CP 包名 `fft.js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fft.js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fft.js
```

```cp
import {
    absF64,
    sqrtF64,
    reduceAngle,
    sinF64,
    cosF64,
    isPowerOfTwo,
    fftInPlace,
    ifftInPlace,
} from "fft.js";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `reduceAngle` | `(rad: f64): f64` | 调用 `reduceAngle` 并返回 `f64`。 |
| `sinF64` | `(rad: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(rad: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |
| `isPowerOfTwo` | `(n: usize): bool` | 执行 `isPowerOfTwo` 条件判定，返回 `bool`。 |
| `fftInPlace` | `(re: []f64, im: []f64): bool` | 调用 `fftInPlace` 并返回 `bool`。 |
| `ifftInPlace` | `(re: []f64, im: []f64): bool` | 调用 `ifftInPlace` 并返回 `bool`。 |
| `magnitudeSpectrum` | `(re: &[]f64, im: &[]f64, out_mag: []f64): usize` | 调用 `magnitudeSpectrum` 并返回 `usize`。 |
| `dominantBin` | `(re: &[]f64, im: &[]f64): usize` | 调用 `dominantBin` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isPowerOfTwo } from "fft.js";

pub fn main() {
    let result = isPowerOfTwo(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
