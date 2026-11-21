# `cap:biquad` — 纯 CP 零分配音频/DSP Biquad IIR 滤波器与 Goertzel 单频检测库

100% 纯 CP 零堆分配（Zero-Alloc）音频与数字信号处理标准库：
- 严格遵循 **Robert Bristow-Johnson (RBJ) Audio EQ Cookbook** 归一化二阶节（$a_0 = 1$）；
- 采用数值稳定性最优的 **Transposed Direct Form II**（纯值类型状态 `z1`, `z2`），支持原地块处理与级联二阶节（SOS）；
- 内置解析频率响应评估 `H(e^{j\omega})` 与 $O(N)$ 零分配 **Goertzel 单频能量/幅值检测**（DTMF 与单音检测核心）。

## 导入方式

```cp
import {
    FilterType,
    BiquadCoeffs,
    BiquadFilter,
    FreqResponse,
    GoertzelResult,
    designBiquad,
    processSosBlock,
    frequencyResponse,
    goertzelDetect,
} from "cap:biquad";
```

## API 契约

### 1. 滤波器设计

- `pub enum FilterType { LowPass, HighPass, BandPass, Notch, AllPass, PeakingEq, LowShelf, HighShelf }`
- `pub struct BiquadCoeffs { b0, b1, b2, a1, a2: f64 }`
- `pub fn designBiquad(kind: FilterType, sample_rate: f64, freq: f64, q: f64, gain_db: f64): BiquadCoeffs?`
  要求 `sample_rate > 0`、`0 < freq < sample_rate / 2`、`q > 0` 且全有限，否则返回 `null`。

### 2. 滤波状态与级联 SOS

- `pub struct BiquadFilter`
  - `BiquadFilter.init(coeffs: BiquadCoeffs): BiquadFilter`
  - `reset(*_)`
  - `processSample(*_, x: f64): f64`
  - `processBlock(*_, input: []const f64, output: *[]f64): bool`（支持原地处理）
- `pub fn processSosBlock(stages: *[]BiquadFilter, input: []const f64, output: *[]f64): bool`
  逐样本串行通过全部二阶节，支持原地处理。

### 3. 解析频率响应与 Goertzel 单频检测

- `pub struct FreqResponse { magnitude, magnitude_db, phase_rad: f64 }`
- `pub fn frequencyResponse(coeffs: BiquadCoeffs, sample_rate: f64, freq: f64): FreqResponse?`
  在 `freq ∈ [0, sample_rate / 2]` 上计算复频率响应 $H(e^{j\omega})$ 的线性幅值、dB 增益与相位（弧度）。
- `pub struct GoertzelResult { power, magnitude: f64 }`
- `pub fn goertzelDetect(samples: []const f64, sample_rate: f64, target_freq: f64): GoertzelResult?`
  $O(N)$ 零分配单频检测，返回归一化功率 $|X(\omega)|^2 / N^2$ 与单边正弦幅值估计（非 DC/Nyquist 频点为 $2|X(\omega)|/N$）。

## 示例

```cp
import { FilterType, BiquadFilter, designBiquad, goertzelDetect } from "cap:biquad";

fn demo() {
    if (coeffs := designBiquad(FilterType.Notch, 8000.0, 1000.0, 8.0, 0.0)) {
        let flt = BiquadFilter.init(coeffs);
        let buf: [64]f64 = [0.0; 64];
        let out: []f64 = buf[0..];
        * = flt.processBlock(buf[0..], *out);
        if (g := goertzelDetect(out, 8000.0, 1000.0)) {
            * = g;
        }
    }
}
```
