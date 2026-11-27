[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# palette

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RGB / HSL / HSV / CMYK color space conversions, hue rotation, lighten/darken, and linear RGB mixing for CP.

## 安装与导入

```bash
cap add palette
```

```cp
import { rgbFromU8 } from "palette";
```

## 核心导出 API

- `export fn rgbFromU8(r: u8, g: u8, b: u8): Rgb;`
- `export fn rgbToHsl(c: Rgb): Hsl;`
- `export fn hslToRgb(c: Hsl): Rgb;`
- `export fn rgbToHsv(c: Rgb): Hsv;`
- `export fn hsvToRgb(c: Hsv): Rgb;`
- `export fn rgbToCmyk(c: Rgb): Cmyk;`
- `export fn cmykToRgb(c: Cmyk): Rgb;`
- `export fn rotateHue(c: Hsl, degrees: f64): Hsl;`
- `export fn lighten(c: Hsl, amount: f64): Hsl;`
- `export fn darken(c: Hsl, amount: f64): Hsl;`
- `export fn mixRgb(a: Rgb, b: Rgb, t: f64): Rgb;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`palette@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`tinycolor2@1.0.0`)，许可证：MIT
