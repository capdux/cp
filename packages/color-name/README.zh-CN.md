[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# color-name

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 148 W3C CSS Color 4 named colors lookup and exact RGB/HSL/HSV/HEX color space conversion engine for CP

## 安装与导入

```bash
cap add color-name
```

```cp
import { toLowerAscii } from "color-name";
```

## 核心导出 API

- `export fn toLowerAscii(c: u8): u8;`
- `export fn isWs(c: u8): bool;`
- `export fn trimWs(s: string): string;`
- `export fn cmpIgnoreCase(a: string, target: string): i32;`
- `export fn clamp01(v: f64): f64;`
- `export fn normalizeHue(h: f64): f64;`
- `export fn roundToU8(v: f64): u8;`
- `export fn absF64(x: f64): f64;`
- `export fn cssColorCount(): usize;`
- `export fn rgbFromCssName(name: string): Rgb?;`
- `export fn cssNameFromRgb(c: Rgb): string?;`
- `export fn rgbToHsl(c: Rgb): Hsl;`
- `export fn hslToRgb(c: Hsl): Rgb;`
- `export fn rgbToHsv(c: Rgb): Hsv;`
- `export fn hsvToRgb(c: Hsv): Rgb;`
- `export fn hexNibbleChar(v: u8): u8;`
- `export fn parseHexNibble(c: u8): u8?;`
- `export fn rgbToHex(c: Rgb, out: []u8): string;`
- `export fn hexToRgb(hex: string): Rgb?;`
- `export fn mixRgb(a: Rgb, b: Rgb, weight: f64): Rgb;`
- `export fn lighten(c: Rgb, amount: f64): Rgb;`
- `export fn darken(c: Rgb, amount: f64): Rgb;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`color-name@1.0.0`)，许可证：MIT
