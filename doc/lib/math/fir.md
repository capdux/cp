# `cap:fir` — 数字信号处理 FIR 滤波器、窗函数与 RBJ Biquad IIR 滤波器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

纯 CP、零堆分配的数字信号处理（DSP）滤波标准库，提供 Hann/Hamming/Blackman 对称窗函数、Hamming 加权 Windowed-Sinc 低通 FIR 系数设计器、零拷贝环形延迟线流式 FIR 滤波器 `FirFilter` 以及 RBJ Audio EQ Cookbook 二阶 Direct Form II Transposed IIR 滤波器 `Biquad`（含解析频率响应 `magnitudeAt`）。

## 导入

```cp
import {
    windowHann,
    windowHamming,
    windowBlackman,
    designLowpassFir,
    FirFilter,
    Biquad,
} from "cap:math";
```

## 核心 API

- **窗函数**：
  - `windowHann(out: []f64): bool`、`windowHamming(out: []f64): bool`、`windowBlackman(out: []f64): bool`
- **Windowed-Sinc 低通 FIR 系数设计**：
  - `designLowpassFir(cutoff_norm: f64, coeffs: []f64): bool`（`0 < cutoff_norm < 0.5`，直流增益归一化为 `1.0`）
- **流式 FIR 滤波器 `FirFilter`**：
  - `FirFilter.init(coeffs: []const f64, delay: []f64): FirFilter?`
  - `process(*_, sample: f64): f64`、`processSlice(*_, input: []const f64, output: []f64): bool`、`reset(*_)`
- **RBJ 二阶 IIR 滤波器 `Biquad`**：
  - 工厂：`Biquad.lowpass(fs, fc, q)`、`highpass(fs, fc, q)`、`bandpass(fs, fc, q)`、`notch(fs, fc, q)`
  - 方法：`process(*_, x: f64): f64`、`processInPlace(*_, buf: []f64)`、`reset(*_)`、`magnitudeAt(fs: f64, freq: f64): f64`

## 示例

```cp
import std from "std";
import { designLowpassFir, FirFilter, Biquad } from "cap:math";

fn filterSample(x: f64): f64? {
    let bq = Biquad.lowpass(48000.0, 1000.0, 0.70710678) ?? return null;
    return bq.process(x);
}
```
