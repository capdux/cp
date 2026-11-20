# CP 标准库：快速傅里叶变换与频谱分析引擎手册 (`cap:fft` / `cap.fft`)

## 一、 架构总览与设计准则

`cap:fft` 是 CP 语言官方内置的纯原生、零 C 依赖的 **Cooley-Tukey Radix-2 快速傅里叶变换（FFT / IFFT）、经典窗函数、单边实数幅值谱（Parseval 能量守恒）、主频提取与频域快速卷积引擎**。

全模块可与 `cap:audio` 无缝协同，用于语音/音频频谱分析、音高检测与 FIR 滤波卷积，支持 `import ... from "cap:fft"` 与 `import cap from "cap"; cap.fft.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/fft/fft.cp` | `// Source: cp/lib/fft/fft.cp (...)` | `FftError`、`Complex`、`WindowKind`、`FrequencyPeak` 与 `Fft` 引擎 |
| `lib/fft/index.cp` | `// Source: cp/lib/fft/index.cp (...)` | `cap:fft` 与 `cap.fft` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础类型

- **`FftError`**：
  - `NotPowerOfTwo`：输入长度不是 2 的正整数次幂
  - `EmptyInput`：输入切片为空
  - `LengthMismatch`：输入缓冲区长度不匹配
  - `InvalidSampleRate`：采样率为非正数或 `NaN`/`Inf`
  - `OutOfMemory`：分配器内存不足
- **`Complex`**：
  - 字段：`re: f64`、`im: f64`
  - 构造与运算：`Complex.init(re, im)`、`add(other)`、`sub(other)`、`mul(other)`、`abs()`、`absSq()`、`conj()`
- **`WindowKind`**：
  - `Rectangular`、`Hann`、`Hamming`、`Blackman`
- **`FrequencyPeak`**：
  - 字段：`bin: usize`、`freq_hz: f64`、`magnitude: f64`

### 2. `Fft` 核心引擎

- `isPowerOfTwo(n: usize): bool`：判断 `n > 0` 且为 2 的幂。
- `nextPowerOfTwo(n: usize): usize`：返回不小于 `n` 的最小 2 的幂。
- `forward(alloc, input: []const Complex): []Complex ! FftError` & `forwardInPlace(buf: []Complex): void ! FftError`：
  - 迭代式位反转置换 + Cooley-Tukey Radix-2 蝶形正变换。
- `inverse(alloc, spectrum: []const Complex): []Complex ! FftError` & `inverseInPlace(buf: []Complex): void ! FftError`：
  - 基于共轭正变换并缩放 `1/N` 实现精确逆变换，往返重建精度 `< 1e-9`。
- `applyWindow(alloc, signal: []const f64, kind: WindowKind): []f64 ! FftError`：
  - 对实数时域信号施加 Rectangular / Hann / Hamming / Blackman 窗函数抑制频谱泄漏。
- `rfftMagnitude(alloc, signal: []const f64): []f64 ! FftError`：
  - 返回 `0..N/2`（长度 `N/2 + 1`）的单边实数幅值谱，满足 Parseval 能量守恒 `sum(mag[k]^2) / N == sum(signal[n]^2)`。
- `dominantFrequency(alloc, signal: []const f64, sample_rate_hz: f64): FrequencyPeak ! FftError`：
  - 忽略直流分量（DC bin 0），在 `1..N/2` 范围内定位最强频率峰值及对应 Hz。
- `convolve(alloc, a: []const f64, b: []const f64): []f64 ! FftError`：
  - 自动零填充至 `nextPowerOfTwo(a.len + b.len - 1)` 并通过频域点乘实现线性快速卷积。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Fft, Complex, WindowKind } from "cap:fft";

pub fn analyzeSignal(alloc: std.mem.Allocator, signal: []const f64, sample_rate: f64): ! {
    const windowed = Fft.applyWindow(alloc, signal, WindowKind.Hann)?;
    defer alloc.free(windowed);

    const peak = Fft.dominantFrequency(alloc, windowed, sample_rate)?;
    const mag = cap.fft.Fft.rfftMagnitude(alloc, windowed)?;
    defer alloc.free(mag);
    * = peak;
}
```
