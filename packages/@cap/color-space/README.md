# @cap/color-space

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:color-convert` ([https://github.com/Qix-/color-convert](https://github.com/Qix-/color-convert))（原库名 `color-convert` → CP 包名 `@cap/color-space`）

Zero-allocation color space conversion engine (`sRGB` ↔ `Linear RGB` ↔ `HSL` ↔ `HSV` ↔ `OKLab` ↔ `OKLCH`) and perceptual color mixing for CP — clean-room MIT parity with Rust `palette` and npm `color-convert`.

纯 CP 零堆分配专业色彩空间转换与感知色差引擎，零外部依赖内置高精度数学函数与 Björn Ottosson 官方 OKLab 矩阵，适用于 UI 主题生成、图像处理与 CSS Color 4 渲染。

## Install / 安装

```bash
cap add @cap/color-space
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Rgb` / `Hsl` / `Hsv` / `Oklab` / `Oklch` | Color structs (`f64` components, `h` in `[0, 360)`) | Color space representations |
| `srgbToLinear` / `linearToSrgb` | `(c: f64) -> f64` | IEC 61966-2-1 piecewise sRGB gamma |
| `rgbToHsl` / `hslToRgb` | `Rgb <-> Hsl` | sRGB ↔ HSL conversion |
| `rgbToHsv` / `hsvToRgb` | `Rgb <-> Hsv` | sRGB ↔ HSV conversion |
| `rgbToOklab` / `oklabToRgb` | `Rgb <-> Oklab` | Perceptually uniform OKLab conversion |
| `oklabToOklch` / `oklchToOklab` | `Oklab <-> Oklch` | Cylindrical OKLCH conversion |
| `oklabDistance` | `(a: Rgb, b: Rgb) -> f64` | Perceptual Euclidean distance in OKLab |
| `mixOklab` | `(a: Rgb, b: Rgb, t: f64) -> Rgb` | Perceptually uniform color interpolation |

## Usage / 使用示例

```cp
import { Rgb, rgbToOklab, oklabToOklch, mixOklab } from "@cap/color-space";

const red = Rgb{ r: 1.0, g: 0.0, b: 0.0 };
const blue = Rgb{ r: 0.0, g: 0.0, b: 1.0 };
const mid = mixOklab(red, blue, 0.5);
```
