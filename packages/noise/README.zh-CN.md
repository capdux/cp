[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# noise

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap 2D/3D Improved Perlin Noise, Value Noise, and Fractal Brownian Motion (fBm) in pure CP

## 安装与导入

```bash
cap add noise
```

```cp
import { floorInt } from "noise";
```

## 核心导出 API

- `export fn floorInt(x: f64): i64;`
- `export fn fade(t: f64): f64;`
- `export fn lerp(a: f64, b: f64, t: f64): f64;`
- `export fn grad2(hash: u8, x: f64, y: f64): f64;`
- `export fn grad3(hash: u8, x: f64, y: f64, z: f64): f64;`
- `export fn splitmix64(state: &mut u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`noise@1.0.0`)，许可证：MIT
