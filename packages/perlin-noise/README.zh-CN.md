[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# perlin-noise

Zero-heap Ken Perlin improved 1D/2D/3D gradient noise, fBm, and ridge noise in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:perlin-noise` ([https://github.com/andrewrk/node-perlin-noise](https://github.com/andrewrk/node-perlin-noise))

| 元信息 | 值 |
| --- | --- |
| 包名 | `perlin-noise` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add perlin-noise
```

```cp
import {
    floorF64,
    permAt,
    lerpF64,
    grad1D,
    grad2D,
    grad3D,
    fade,
    noise1D,
} from "perlin-noise";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `floorF64` | `(x: f64): i64` | 调用 `floorF64` 并返回 `i64`。 |
| `permAt` | `(idx: usize, seed: u32): usize` | 调用 `permAt` 并返回 `usize`。 |
| `lerpF64` | `(a: f64, b: f64, t: f64): f64` | 调用 `lerpF64` 并返回 `f64`。 |
| `grad1D` | `(hash: usize, x: f64): f64` | 调用 `grad1D` 并返回 `f64`。 |
| `grad2D` | `(hash: usize, x: f64, y: f64): f64` | 调用 `grad2D` 并返回 `f64`。 |
| `grad3D` | `(hash: usize, x: f64, y: f64, z: f64): f64` | 调用 `grad3D` 并返回 `f64`。 |
| `fade` | `(t: f64): f64` | 调用 `fade` 并返回 `f64`。 |
| `noise1D` | `(x: f64, seed: u32): f64` | 调用 `noise1D` 并返回 `f64`。 |
| `noise2D` | `(x: f64, y: f64, seed: u32): f64` | 调用 `noise2D` 并返回 `f64`。 |
| `noise3D` | `(x: f64, y: f64, z: f64, seed: u32): f64` | 调用 `noise3D` 并返回 `f64`。 |
| `fbm2D` | `(x: f64, y: f64, seed: u32, octaves: u32, lacunarity: f64, gain: f64): f64` | 调用 `fbm2D` 并返回 `f64`。 |
| `ridge2D` | `(x: f64, y: f64, seed: u32, octaves: u32): f64` | 调用 `ridge2D` 并返回 `f64`。 |

## 最小调用示例

```cp
import { floorF64 } from "perlin-noise";

pub fn main() {
    let result = floorF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
