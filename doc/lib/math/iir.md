# `cap:iir` — 纯 CP 零堆分配经典 IIR 滤波器设计库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:iir` 提供 100% 纯 CP、零堆分配的经典 IIR 数字滤波器设计与处理原语，基于频率预畸变双线性变换（Pre-warped Bilinear Transform）生成数值稳定的归一化二阶节（SOS / Biquad）级联，支持任意阶数 **Butterworth 低通/高通**、**Chebyshev Type-I 等纹波低通**、Transposed Direct Form II 流式滤波、零相位前后向滤波（`sosFiltFilt`）以及精确 dB 幅频响应评估。

## 特性概览

- **零堆分配（Zero-Alloc）**：`SosSection` 级联数组、`SosState` 延迟状态与 `filtfilt` 临时缓冲均由调用方提供。
- **任意阶数稳定 SOS 级联**：自动将 `order >= 1` 拆分为 `ceil(order / 2)` 个归一化二阶/一阶节，避免高阶直接型多项式系数舍入不稳定。
- **精确频点校准**：
  - Butterworth 低通/高通在 `cutoff_hz` 处精确达到 `-3.0103 dB`（$-10\log_{10} 2$），DC（低通）/ Nyquist（高通）增益精确归一化为 `1.0`。
  - Chebyshev-I 低通在 `cutoff_hz` 处精确达到 `-ripple_db`，偶阶自动施加 `10^(-ripple_db/20)` DC 增益缩放。
- **零相位滤波 `sosFiltFilt`**：前向 + 反向级联滤波，彻底消除群延迟与相位失真，幅频响应平方叠加。

## API 参考

### 数据结构
- `SosSection`：`b0`, `b1`, `b2`, `a1`, `a2`（`a0 = 1.0`）；提供 `SosSection.identity()`
- `SosState`：`z1`, `z2`（Transposed Direct Form II）；提供 `SosState.zero()`

### 滤波器设计
- `designButterworthLowpass(order: usize, cutoff_hz: f64, sample_rate: f64, sections_out: *[]SosSection): usize?`
- `designButterworthHighpass(order: usize, cutoff_hz: f64, sample_rate: f64, sections_out: *[]SosSection): usize?`
- `designChebyshev1Lowpass(order: usize, ripple_db: f64, cutoff_hz: f64, sample_rate: f64, sections_out: *[]SosSection): usize?`

### 滤波与幅频响应
- `sosFilter(sections: []const SosSection, states: *[]SosState, input: []const f64, output: *[]f64): bool`
- `sosFiltFilt(sections: []const SosSection, input: []const f64, output: *[]f64, tmp_buf: *[]f64, states_scratch: *[]SosState): bool`
- `sosMagnitudeDb(sections: []const SosSection, sample_rate: f64, freq_hz: f64): f64?`

## 使用示例

```cp
import {
    SosSection,
    SosState,
    designButterworthLowpass,
    sosFiltFilt,
    sosMagnitudeDb,
} from "cap:math";

let secs_b: [2]SosSection = [SosSection.identity(); 2];
let secs: []SosSection = secs_b[0..];
if (n_sec := designButterworthLowpass(4, 100.0, 1000.0, *secs)) {
    const db_fc = sosMagnitudeDb(secs[0..n_sec], 1000.0, 100.0); // ~ -3.0103 dB
    * = db_fc;
}
```
