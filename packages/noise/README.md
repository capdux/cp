[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/noise`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:noise` ([https://github.com/Razaekel/noise-rs](https://github.com/Razaekel/noise-rs))

Zero-heap 2D/3D Improved Perlin Noise, Fractal Brownian Motion (fBm), and Ridged Multifractal procedural noise generator in pure CP, modeled after Rust's `noise` crate.

零堆分配纯 CP 程序化噪声引擎，对标 Rust `noise` crate，内置可设种子的 2D/3D 改进 Perlin 梯度噪声、分形布朗运动 `fbm2D` 与山脊分形 `ridged2D`，适用于地形生成、纹理与图形特效。

## Installation

```bash
cap add @cap/noise
```

## API

| Method | Description |
| --- | --- |
| `PerlinNoise.init(seed)` | Initialize seeded 512-byte permutation table |
| `noise2D(x, y)` | Deterministic 2D improved Perlin gradient noise |
| `noise3D(x, y, z)` | Deterministic 3D improved Perlin gradient noise |
| `fbm2D(x, y, octaves, lacunarity, gain)` | Normalized 2D Fractal Brownian Motion |
| `ridged2D(x, y, octaves)` | Ridged multifractal 2D noise in `[0, 1]` |

## License

MIT
