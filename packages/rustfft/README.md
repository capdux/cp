[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/fft

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustfft` ([https://github.com/ejmahler/RustFFT](https://github.com/ejmahler/RustFFT))（原库名 `rustfft` → CP 包名 `@cap/fft`）

Zero-allocation in-place radix-2 Cooley-Tukey FFT, IFFT, magnitude spectrum, and Hann/Hamming windowing for CP — clean-room MIT parity with Rust `rustfft` and `microfft`.

纯 CP 零堆分配原地快速傅里叶变换（FFT / IFFT）与信号加窗引擎，零外部依赖内置高精度三角函数，适用于音频 DSP、语音特征提取、振动频谱分析与多项式卷积。

## Install / 安装

```bash
cap add @cap/fft
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Complex` | `struct { re: f64, im: f64 }` | Complex double-precision sample |
| `isPowerOfTwo` | `(n: usize) -> bool` | Validate power-of-two transform length |
| `fftInPlace` | `(buf: []Complex) -> bool` | In-place radix-2 DIT-FFT (`false` if not power-of-two) |
| `ifftInPlace` | `(buf: []Complex) -> bool` | In-place exact inverse FFT (`1/N` normalized) |
| `magnitude` | `(c: Complex) -> f64` | Euclidean magnitude `sqrt(re^2 + im^2)` |
| `applyHannWindow` | `(samples: []f64)` | In-place Hann windowing |
| `applyHammingWindow` | `(samples: []f64)` | In-place Hamming windowing |

## Usage / 使用示例

```cp
import { Complex, fftInPlace, ifftInPlace, magnitude } from "@cap/fft";

let buf: [4]Complex = [
    Complex{ re: 1.0, im: 0.0 },
    Complex{ re: 0.0, im: 0.0 },
    Complex{ re: 0.0, im: 0.0 },
    Complex{ re: 0.0, im: 0.0 },
];
* = fftInPlace(buf[0..4]);
* = ifftInPlace(buf[0..4]);
```
