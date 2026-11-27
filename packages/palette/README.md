[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# palette

Zero-allocation RGB / HSL / HSV / CMYK color space conversions, hue rotation, lighten/darken, and linear RGB mixing for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:palette` ([https://github.com/Ogeon/palette](https://github.com/Ogeon/palette)) / `npm:tinycolor2` ([https://github.com/bgrins/TinyColor](https://github.com/bgrins/TinyColor))（原库名 `palette` → CP 包名 `palette`）

| Metadata | Value |
| --- | --- |
| Package | `palette` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rgb` | `class` | Fields: `r: f64, g: f64, b: f64` |
| `Hsl` | `class` | Fields: `h: f64, s: f64, l: f64` |
| `Hsv` | `class` | Fields: `h: f64, s: f64, v: f64` |
| `Cmyk` | `class` | Fields: `c: f64, m: f64, y: f64, k: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `chAbs` | `(x: f64): f64` | Invokes `chAbs` and returns `f64`. |
| `chClamp01` | `(x: f64): f64` | Invokes `chClamp01` and returns `f64`. |
| `chNormHue` | `(h: f64): f64` | Invokes `chNormHue` and returns `f64`. |
| `chSectorRgb` | `(hn: f64, chroma: f64, m: f64): Rgb` | Invokes `chSectorRgb` and returns `Rgb`. |
| `rgbFromU8` | `(r: u8, g: u8, b: u8): Rgb` | Invokes `rgbFromU8` and returns `Rgb`. |
| `rgbToHsl` | `(c: Rgb): Hsl` | Invokes `rgbToHsl` and returns `Hsl`. |
| `hslToRgb` | `(c: Hsl): Rgb` | Invokes `hslToRgb` and returns `Rgb`. |
| `rgbToHsv` | `(c: Rgb): Hsv` | Invokes `rgbToHsv` and returns `Hsv`. |
| `hsvToRgb` | `(c: Hsv): Rgb` | Invokes `hsvToRgb` and returns `Rgb`. |
| `rgbToCmyk` | `(c: Rgb): Cmyk` | Invokes `rgbToCmyk` and returns `Cmyk`. |
| `cmykToRgb` | `(c: Cmyk): Rgb` | Invokes `cmykToRgb` and returns `Rgb`. |
| `rotateHue` | `(c: Hsl, degrees: f64): Hsl` | Invokes `rotateHue` and returns `Hsl`. |
| `lighten` | `(c: Hsl, amount: f64): Hsl` | Invokes `lighten` and returns `Hsl`. |
| `darken` | `(c: Hsl, amount: f64): Hsl` | Invokes `darken` and returns `Hsl`. |
| `mixRgb` | `(a: Rgb, b: Rgb, t: f64): Rgb` | Invokes `mixRgb` and returns `Rgb`. |

## Minimal Example

```cp
import { chAbs } from "palette";

pub fn main() {
    let result = chAbs(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
