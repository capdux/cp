[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# postcss-value-parser

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postcss-value-parser` ([https://github.com/TrySound/postcss-value-parser](https://github.com/TrySound/postcss-value-parser))（原库名 `postcss-value-parser` → CP 包名 `postcss-value-parser`）

Zero-allocation CSS dimension, length, angle, time, and resolution parser and unit converter for CP — clean-room MIT parity with `postcss-value-parser` (120M/wk) dimension subset, `parse-css-dimension`, and `css-unit-converter`.

纯 CP 零堆分配 CSS 维度/长度/角度/时间/分辨率单位解析与换算引擎，完整支持 CSS 96dpi 绝对长度、视口/字号相对长度、角度、时间与分辨率单位。

## Install / 安装

```bash
cap add postcss-value-parser
```

## Supported Units / 支持单位

- **Absolute Length (`"length"`)**: `px`, `cm`, `mm`, `in`, `pt`, `pc`, `q`
- **Relative Length (`"length"`)**: `rem`, `em`, `vh`, `vw`, `vmin`, `vmax`, `ch`
- **Percentage (`"percentage"`)**: `%`
- **Angle (`"angle"`)**: `deg`, `rad`, `grad`, `turn`
- **Time (`"time"`)**: `ms`, `s`
- **Resolution (`"resolution"`)**: `dpi`, `dpcm`, `dppx`

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CssUnitKind` | `enum { Px, Cm, Mm, In, Pt, Pc, Q, Rem, Em, Vh, Vw, Vmin, Vmax, Percent, Ch, Deg, Rad, Grad, Turn, Ms, S, Dpi, Dpcm, Dppx, Unitless, Unknown }` | CSS unit classification |
| `CssDimension` | `struct { valid: bool, value: f64, unit: CssUnitKind, unit_str: string }` | Parsed CSS dimension value |
| `CssContext` | `struct { root_font_px, parent_font_px, viewport_w, viewport_h }` | Conversion context (`CssContext.defaultCtx()` = 16px / 16px / 1920x1080) |
| `parseCssDimension` | `(raw: string) -> CssDimension` | Parse signed, decimal (`.5rem`), scientific notation, case-insensitive dimensions |
| `toPixels` | `(dim: CssDimension, ctx: CssContext) -> f64?` | Convert absolute/relative lengths to CSS pixels (96dpi) |
| `toDegrees` | `(dim: CssDimension) -> f64?` | Convert `deg`/`rad`/`grad`/`turn` to degrees |
| `toMilliseconds` | `(dim: CssDimension) -> f64?` | Convert `ms`/`s` to milliseconds |
| `unitCategory` | `(u: CssUnitKind) -> string` | Return `"length"`, `"angle"`, `"time"`, `"resolution"`, `"percentage"`, or `"none"` |

## Usage / 使用示例

```cp
import {
    CssContext,
    parseCssDimension,
    toPixels,
    toDegrees,
    toMilliseconds,
} from "postcss-value-parser";

const ctx = CssContext.defaultCtx();
const px = toPixels(parseCssDimension("1.5rem"), ctx);   // 24.0
const deg = toDegrees(parseCssDimension("0.25turn"));    // 90.0
const ms = toMilliseconds(parseCssDimension("1.5s"));    // 1500.0
```
