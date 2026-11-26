# @cap/color-hsl

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:palette` ([https://github.com/Ogeon/palette](https://github.com/Ogeon/palette)) / `npm:tinycolor2` ([https://github.com/bgrins/TinyColor](https://github.com/bgrins/TinyColor))（原库名 `palette` → CP 包名 `@cap/color-hsl`）

Zero-allocation RGB / HSL / HSV / CMYK color space conversion, hue rotation, lighten/darken, and linear RGB mixing library for CP.

## Features

- **Zero heap allocation**: normalized `[0.0, 1.0]` value types `Rgb`, `Hsl`, `Hsv`, `Cmyk`.
- **Full conversions**: `rgbToHsl`, `hslToRgb`, `rgbToHsv`, `hsvToRgb`, `rgbToCmyk`, `cmykToRgb`, `rgbFromU8`.
- **Manipulation helpers**: `rotateHue` (360° wrap), `lighten`, `darken` (clamped), and `mixRgb`.

## Usage

```cp
import { rgbFromU8, rgbToHsl, rotateHue, hslToRgb } from "@cap/color-hsl";

const hsl = rgbToHsl(rgbFromU8(255 as u8, 0 as u8, 0 as u8));
const green = hslToRgb(rotateHue(hsl, 120.0 as f64));
```
