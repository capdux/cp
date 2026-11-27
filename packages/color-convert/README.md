[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# color-convert

Zero-allocation color space conversions (RGB, HSV, HWB, HSL, CMYK, ANSI256, ANSI16, Hex, CSS keywords) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`color-convert`](https://github.com/Qix-/color-convert)（`MIT`）（包名 `color-convert`）。

| Metadata | Value |
| --- | --- |
| Package | `color-convert` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add color-convert
```

```cp
import {
    Rgb,
    Hsv,
    Hwb,
    Hsl,
    Cmyk,
    ccAbs,
    ccClamp01,
    ccNormHue,
} from "color-convert";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rgb` | `class` | Fields: `r: u8, g: u8, b: u8` |
| `Hsv` | `class` | Fields: `h: f64, s: f64, v: f64` |
| `Hwb` | `class` | Fields: `h: f64, w: f64, b: f64` |
| `Hsl` | `class` | Fields: `h: f64, s: f64, l: f64` |
| `Cmyk` | `class` | Fields: `c: f64, m: f64, y: f64, k: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ccAbs` | `(x: f64): f64` | Invokes `ccAbs` and returns `f64`. |
| `ccClamp01` | `(x: f64): f64` | Invokes `ccClamp01` and returns `f64`. |
| `ccNormHue` | `(h: f64): f64` | Invokes `ccNormHue` and returns `f64`. |
| `ccRoundU8` | `(x: f64): u8` | Invokes `ccRoundU8` and returns `u8`. |
| `ccHexVal` | `(ch: u8): i16` | Invokes `ccHexVal` and returns `i16`. |
| `ccAnsiStd16` | `(idx: u8): Rgb` | Invokes `ccAnsiStd16` and returns `Rgb`. |
| `rgbToHsv` | `(r: u8, g: u8, b: u8): Hsv` | Invokes `rgbToHsv` and returns `Hsv`. |
| `hsvToRgb` | `(h: f64, s: f64, v: f64): Rgb` | Invokes `hsvToRgb` and returns `Rgb`. |
| `rgbToHsl` | `(r: u8, g: u8, b: u8): Hsl` | Invokes `rgbToHsl` and returns `Hsl`. |
| `hslToRgb` | `(h: f64, s: f64, l: f64): Rgb` | Invokes `hslToRgb` and returns `Rgb`. |
| `rgbToHwb` | `(r: u8, g: u8, b: u8): Hwb` | Invokes `rgbToHwb` and returns `Hwb`. |
| `hwbToRgb` | `(h: f64, w: f64, b: f64): Rgb` | Invokes `hwbToRgb` and returns `Rgb`. |
| `rgbToCmyk` | `(r: u8, g: u8, b: u8): Cmyk` | Invokes `rgbToCmyk` and returns `Cmyk`. |
| `cmykToRgb` | `(c: f64, m: f64, y: f64, k: f64): Rgb` | Invokes `cmykToRgb` and returns `Rgb`. |
| `rgbToAnsi256` | `(r: u8, g: u8, b: u8): u8` | Invokes `rgbToAnsi256` and returns `u8`. |
| `ansi256ToRgb` | `(code: u8): Rgb` | Invokes `ansi256ToRgb` and returns `Rgb`. |
| `rgbToAnsi16` | `(r: u8, g: u8, b: u8): u8` | Invokes `rgbToAnsi16` and returns `u8`. |
| `ansi256ToAnsi16` | `(code: u8): u8` | Invokes `ansi256ToAnsi16` and returns `u8`. |
| `ansi16ToRgb` | `(code: u8): Rgb` | Invokes `ansi16ToRgb` and returns `Rgb`. |
| `rgbToHex` | `(r: u8, g: u8, b: u8, out: []u8): usize` | Invokes `rgbToHex` and returns `usize`. |
| `hexToRgb` | `(hex: string): RgbColor` | Invokes `hexToRgb` and returns `RgbColor`. |
| `keywordToRgb` | `(name: string): RgbColor` | Invokes `keywordToRgb` and returns `RgbColor`. |
| `rgbToKeyword` | `(r: u8, g: u8, b: u8): string` | Invokes `rgbToKeyword` and returns `string`. |
| `rgbToGray` | `(r: u8, g: u8, b: u8): f64` | Invokes `rgbToGray` and returns `f64`. |
| `grayToRgb` | `(gray: f64): Rgb` | Invokes `grayToRgb` and returns `Rgb`. |

## Minimal Example

```cp
import { hexToRgb } from "color-convert";

pub fn main() {
    let result = hexToRgb("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
