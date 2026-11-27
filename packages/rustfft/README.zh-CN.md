[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rustfft

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation in-place radix-2 Cooley-Tukey FFT, IFFT, magnitude spectrum, and Hann/Hamming windowing (Rust rustfft / microfft parity) for CP

## 安装与导入

```bash
cap add rustfft
```

```cp
import { isPowerOfTwo } from "rustfft";
```

## 核心导出 API

- `export fn isPowerOfTwo(n: usize): bool;`
- `export fn magnitude(c: Complex): f64;`
- `export fn fftInPlace(buf: []Complex): bool;`
- `export fn ifftInPlace(buf: []Complex): bool;`
- `export fn applyHannWindow(samples: []f64);`
- `export fn applyHammingWindow(samples: []f64);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rustfft@1.0.0`)，许可证：MIT
