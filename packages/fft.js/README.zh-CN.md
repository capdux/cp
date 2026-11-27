[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fft.js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation in-place iterative Cooley-Tukey Radix-2 FFT, IFFT, and magnitude spectrum for CP

## 安装与导入

```bash
cap add fft.js
```

```cp
import { absF64 } from "fft.js";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn reduceAngle(rad: f64): f64;`
- `export fn sinF64(rad: f64): f64;`
- `export fn cosF64(rad: f64): f64;`
- `export fn isPowerOfTwo(n: usize): bool;`
- `export fn fftInPlace(re: []f64, im: []f64): bool;`
- `export fn ifftInPlace(re: []f64, im: []f64): bool;`
- `export fn magnitudeSpectrum(re: &[]f64, im: &[]f64, out_mag: []f64): usize;`
- `export fn dominantBin(re: &[]f64, im: &[]f64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fft.js@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`microfft@1.0.0`)，许可证：MIT
