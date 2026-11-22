# CP 标准库：确定性相干噪声引擎 (`cap:noise` / `cap.noise`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

## 一、 架构总览与设计准则

`cap:noise` 是 CP 语言官方内置的纯原生、零 C 依赖的 **确定性相干噪声（Coherent Noise）与程序化分形地形引擎**，面向程序化地形生成（配合 `cap:pathfinding` / `cap:ecs`）、图形纹理合成与物理仿真场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:gfx"` 与 `import cap from "cap"; cap.noise.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/noise/noise.cp` | `// Source: cp/lib/noise/noise.cp (...)` | `NoiseError`、`FractalConfig` 与 `NoiseGen` 核心噪声引擎 |
| `lib/noise/index.cp` | `// Source: cp/lib/noise/index.cp (...)` | `cap:noise` 与 `cap.noise` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与分形配置 (`NoiseError` & `FractalConfig`)

```typescript
pub err NoiseError {
    InvalidOctaves,
    InvalidDimensions,
    InvalidFrequency,
    OutOfMemory,
}

pub struct FractalConfig {
    octaves: u32 = 4;
    frequency: f64 = 1.0;
    lacunarity: f64 = 2.0;
    gain: f64 = 0.5;
}
```

- `FractalConfig.init(octaves, frequency, lacunarity, gain): FractalConfig`
- `FractalConfig.default(): FractalConfig` — 默认 4 倍频、`frequency = 1.0`、`lacunarity = 2.0`、`gain = 0.5`。

### 2. 核心噪声引擎 (`NoiseGen`)

`NoiseGen` 内部包含基于 **SplitMix64** 确定性打乱的 512 项排列置换表 `[512]u8`，**零堆分配结构体**，可直接在栈上构造与传递。

- `NoiseGen.init(seed: u64): NoiseGen` — 使用 SplitMix64 + Fisher-Yates 洗牌构建确定性排列置换表。
- `perlin2(x: f64, y: f64): f64` — 经典 2D Perlin 梯度噪声，采用 Quintic ($6t^5 - 15t^4 + 10t^3$) 二阶连续可微平滑插值，输出严格位于 `[-1.0, 1.0]`。
- `perlin3(x: f64, y: f64, z: f64): f64` — 经典 3D Perlin 梯度噪声，输出严格位于 `[-1.0, 1.0]`。
- `worley2(x: f64, y: f64): f64` — 2D Cellular / Worley Voronoi 最近特征点欧氏距离，输出位于 `[0.0, ~1.414]`。
- `fbm2(x: f64, y: f64, cfg: FractalConfig): f64 ! NoiseError` — 多倍频归一化 fBm（Fractional Brownian Motion），严格输出于 `[-1.0, 1.0]`；若 `octaves == 0` 返回 `InvalidOctaves`，`frequency <= 0.0` 返回 `InvalidFrequency`。
- `ridged2(x: f64, y: f64, cfg: FractalConfig): f64 ! NoiseError` — 山脊分形噪声（`1.0 - |perlin2|` 加权叠加），输出位于 `[0.0, 1.0]`。
- `generateGrid2D(alloc: std.mem.Allocator, width: usize, height: usize, x0: f64, y0: f64, step: f64, cfg: FractalConfig): []f64 ! NoiseError` — 批量生成行优先 `width * height` 归一化 `[0.0, 1.0]` 高度图切片（调用方负责 `alloc.free`）；维度为 0 或溢出返回 `InvalidDimensions`。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { NoiseGen, FractalConfig } from "cap:gfx";

pub fn demoNoise(): ! {
    const alloc = std.heap.page_allocator;
    const gen = NoiseGen.init(42);
    const cfg = FractalConfig.init(4, 0.05, 2.0, 0.5);

    const h = gen.perlin2(1.25, 3.75);
    const cell = gen.worley2(2.5, 4.1);
    const ridge = gen.ridged2(10.0, 20.0, cfg)?;

    const grid = gen.generateGrid2D(alloc, 32, 32, 0.0, 0.0, 1.0, cfg)?;
    defer alloc.free(grid);

    // 动态懒加载挂载等价用法
    const gen2 = cap.noise.NoiseGen.init(99);
    const v3 = gen2.perlin3(0.5, 1.5, 2.5);
    * = h;
    * = cell;
    * = ridge;
    * = v3;
}
```
