[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# perlin-noise

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 perlin-noise

## 安装与导入

```bash
cap add perlin-noise
```

```cp
import { floorF64 } from "perlin-noise";
```

## 核心导出 API

- `export fn floorF64(x: f64): i64;`
- `export fn permAt(idx: usize, seed: u32): usize;`
- `export fn lerpF64(a: f64, b: f64, t: f64): f64;`
- `export fn grad1D(hash: usize, x: f64): f64;`
- `export fn grad2D(hash: usize, x: f64, y: f64): f64;`
- `export fn grad3D(hash: usize, x: f64, y: f64, z: f64): f64;`
- `export fn fade(t: f64): f64;`
- `export fn noise1D(x: f64, seed: u32): f64;`
- `export fn noise2D(x: f64, y: f64, seed: u32): f64;`
- `export fn noise3D(x: f64, y: f64, z: f64, seed: u32): f64;`
- `export fn fbm2D(x: f64, y: f64, seed: u32, octaves: u32, lacunarity: f64, gain: f64): f64;`
- `export fn ridge2D(x: f64, y: f64, seed: u32, octaves: u32): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`perlin-noise@1.0.0`)，许可证：MIT
