[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# color-convert

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation color space conversions (RGB, HSV, HWB, HSL, CMYK, ANSI256, ANSI16, Hex, CSS keywords) in pure CP

## 安装与导入

```bash
cap add color-convert
```

```cp
import { rgbToHsv } from "color-convert";
```

## 核心导出 API

- `export fn rgbToHsv(r: u8, g: u8, b: u8): Hsv;`
- `export fn hsvToRgb(h: f64, s: f64, v: f64): Rgb;`
- `export fn rgbToHsl(r: u8, g: u8, b: u8): Hsl;`
- `export fn hslToRgb(h: f64, s: f64, l: f64): Rgb;`
- `export fn rgbToHwb(r: u8, g: u8, b: u8): Hwb;`
- `export fn hwbToRgb(h: f64, w: f64, b: f64): Rgb;`
- `export fn rgbToCmyk(r: u8, g: u8, b: u8): Cmyk;`
- `export fn cmykToRgb(c: f64, m: f64, y: f64, k: f64): Rgb;`
- `export fn rgbToAnsi256(r: u8, g: u8, b: u8): u8;`
- `export fn ansi256ToRgb(code: u8): Rgb;`
- `export fn rgbToAnsi16(r: u8, g: u8, b: u8): u8;`
- `export fn ansi256ToAnsi16(code: u8): u8;`
- `export fn ansi16ToRgb(code: u8): Rgb;`
- `export fn rgbToHex(r: u8, g: u8, b: u8, out: []u8): usize;`
- `export fn hexToRgb(hex: string): RgbColor;`
- `export fn keywordToRgb(name: string): RgbColor;`
- `export fn rgbToKeyword(r: u8, g: u8, b: u8): string;`
- `export fn rgbToGray(r: u8, g: u8, b: u8): f64;`
- `export fn grayToRgb(gray: f64): Rgb;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`color-convert@1.0.0`)，许可证：MIT
