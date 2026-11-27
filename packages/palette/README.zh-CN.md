[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# palette

Zero-allocation RGB / HSL / HSV / CMYK color space conversions, hue rotation, lighten/darken, and linear RGB mixing for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:palette` ([https://github.com/Ogeon/palette](https://github.com/Ogeon/palette)) / `npm:tinycolor2` ([https://github.com/bgrins/TinyColor](https://github.com/bgrins/TinyColor))（原库名 `palette` → CP 包名 `palette`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `palette` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add palette
```

```cp
import {
    Rgb,
    Hsl,
    Hsv,
    Cmyk,
    chAbs,
    chClamp01,
    chNormHue,
    chSectorRgb,
} from "palette";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rgb` | `class` | 字段: `r: f64, g: f64, b: f64` |
| `Hsl` | `class` | 字段: `h: f64, s: f64, l: f64` |
| `Hsv` | `class` | 字段: `h: f64, s: f64, v: f64` |
| `Cmyk` | `class` | 字段: `c: f64, m: f64, y: f64, k: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `chAbs` | `(x: f64): f64` | 调用 `chAbs` 并返回 `f64`。 |
| `chClamp01` | `(x: f64): f64` | 调用 `chClamp01` 并返回 `f64`。 |
| `chNormHue` | `(h: f64): f64` | 调用 `chNormHue` 并返回 `f64`。 |
| `chSectorRgb` | `(hn: f64, chroma: f64, m: f64): Rgb` | 调用 `chSectorRgb` 并返回 `Rgb`。 |
| `rgbFromU8` | `(r: u8, g: u8, b: u8): Rgb` | 调用 `rgbFromU8` 并返回 `Rgb`。 |
| `rgbToHsl` | `(c: Rgb): Hsl` | 调用 `rgbToHsl` 并返回 `Hsl`。 |
| `hslToRgb` | `(c: Hsl): Rgb` | 调用 `hslToRgb` 并返回 `Rgb`。 |
| `rgbToHsv` | `(c: Rgb): Hsv` | 调用 `rgbToHsv` 并返回 `Hsv`。 |
| `hsvToRgb` | `(c: Hsv): Rgb` | 调用 `hsvToRgb` 并返回 `Rgb`。 |
| `rgbToCmyk` | `(c: Rgb): Cmyk` | 调用 `rgbToCmyk` 并返回 `Cmyk`。 |
| `cmykToRgb` | `(c: Cmyk): Rgb` | 调用 `cmykToRgb` 并返回 `Rgb`。 |
| `rotateHue` | `(c: Hsl, degrees: f64): Hsl` | 调用 `rotateHue` 并返回 `Hsl`。 |
| `lighten` | `(c: Hsl, amount: f64): Hsl` | 调用 `lighten` 并返回 `Hsl`。 |
| `darken` | `(c: Hsl, amount: f64): Hsl` | 调用 `darken` 并返回 `Hsl`。 |
| `mixRgb` | `(a: Rgb, b: Rgb, t: f64): Rgb` | 调用 `mixRgb` 并返回 `Rgb`。 |

## 最小调用示例

```cp
import { chAbs } from "palette";

pub fn main() {
    let result = chAbs(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
