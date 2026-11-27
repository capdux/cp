[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# noise

Zero-heap 2D/3D Improved Perlin Noise, Value Noise, and Fractal Brownian Motion (fBm) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:noise` ([https://github.com/Razaekel/noise-rs](https://github.com/Razaekel/noise-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `noise` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add noise
```

```cp
import {
    PerlinNoise,
    floorInt,
    fade,
    lerp,
    grad2,
    grad3,
    splitmix64,
} from "noise";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PerlinNoise` | `class` | 字段: `perm: [512]u8` · 方法: `init, noise2D, noise3D, fbm2D, ridged2D` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `floorInt` | `(x: f64): i64` | 调用 `floorInt` 并返回 `i64`。 |
| `fade` | `(t: f64): f64` | 调用 `fade` 并返回 `f64`。 |
| `lerp` | `(a: f64, b: f64, t: f64): f64` | 调用 `lerp` 并返回 `f64`。 |
| `grad2` | `(hash: u8, x: f64, y: f64): f64` | 调用 `grad2` 并返回 `f64`。 |
| `grad3` | `(hash: u8, x: f64, y: f64, z: f64): f64` | 调用 `grad3` 并返回 `f64`。 |
| `splitmix64` | `(state: &mut u64): u64` | 调用 `splitmix64` 并返回 `u64`。 |

## 最小调用示例

```cp
import { floorInt } from "noise";

pub fn main() {
    let result = floorInt(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
