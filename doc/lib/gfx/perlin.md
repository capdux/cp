# `cap:perlin` — Ken Perlin 2002 改进梯度噪声与分形程序化生成库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

零堆分配（Zero-Alloc）的纯 CP Ken Perlin 2002 Improved Perlin Noise 库，内含 1D/2D/3D $C^2$ 连续梯度噪声、多倍频分形布朗运动（fBm）、山脊噪声（Ridged）与湍流噪声（Turbulence），适用于地形高度图生成、云层/材质纹理合成与程序化世界构建。

## 核心特性

- **零堆分配**：`PerlinNoise` 仅占用 512 字节栈上置换表，无任何动态内存分配。
- **$C^2$ 五次淡化曲线**：采用 Ken Perlin 2002 官方 $6t^5 - 15t^4 + 10t^3$，二阶导数在格点处连续为 0，彻底消除块状网格伪影。
- **确定性种子**：支持 `PerlinNoise.classic()` 官方 2002 经典参考表与 `PerlinNoise.fromSeed(seed)` 确定性洗牌表。
- **归一化分形叠加**：`fbm2d`/`fbm3d` 归一化至 `[-1, 1]`，`ridged2d`/`turbulence2d` 归一化至 `[0, 1]`。

## 导出 API

### 独立平滑函数

- `fade(t: f64): f64`：$6t^5 - 15t^4 + 10t^3$
- `lerp(a: f64, b: f64, t: f64): f64`：线性插值

### `PerlinNoise`

- `PerlinNoise.classic(): PerlinNoise`
- `PerlinNoise.fromSeed(seed: u64): PerlinNoise`
- `noise1d(x: f64): f64` / `noise2d(x: f64, y: f64): f64` / `noise3d(x: f64, y: f64, z: f64): f64`
- `fbm2d(x, y, octaves, lacunarity, gain): f64` / `fbm3d(x, y, z, octaves, lacunarity, gain): f64`
- `ridged2d(x, y, octaves, lacunarity, gain): f64`
- `turbulence2d(x, y, octaves, lacunarity, gain): f64`

## 使用示例

```cp
import { PerlinNoise, fade, lerp } from "cap:gfx";

fn example() {
    const p = PerlinNoise.classic();
    const n2 = p.noise2d(1.25, 3.75);
    const height = p.fbm2d(0.5, 0.8, 5, 2.0, 0.5);
    const ridge = p.ridged2d(0.5, 0.8, 4, 2.0, 0.5);
    * = n2;
    * = height;
    * = ridge;
    * = lerp(0.0, 10.0, fade(0.5));
}
```
