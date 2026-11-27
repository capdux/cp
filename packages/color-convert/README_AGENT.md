# color-convert — AI Agent Quick Ref

Zero-allocation color space conversions (RGB, HSV, HWB, HSL, CMYK, ANSI256, ANSI16, Hex, CSS keywords) in pure CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class Rgb { r: u8, g: u8, b: u8 }
class Hsv { h: f64, s: f64, v: f64 }
class Hwb { h: f64, w: f64, b: f64 }
class Hsl { h: f64, s: f64, l: f64 }
class Cmyk { c: f64, m: f64, y: f64, k: f64 }
```

## API Signatures

```cp
fn ccAbs(x: f64): f64;
fn ccClamp01(x: f64): f64;
fn ccNormHue(h: f64): f64;
fn ccRoundU8(x: f64): u8;
fn ccHexVal(ch: u8): i16;
fn ccAnsiStd16(idx: u8): Rgb;
fn rgbToHsv(r: u8, g: u8, b: u8): Hsv;
fn hsvToRgb(h: f64, s: f64, v: f64): Rgb;
fn rgbToHsl(r: u8, g: u8, b: u8): Hsl;
fn hslToRgb(h: f64, s: f64, l: f64): Rgb;
fn rgbToHwb(r: u8, g: u8, b: u8): Hwb;
fn hwbToRgb(h: f64, w: f64, b: f64): Rgb;
fn rgbToCmyk(r: u8, g: u8, b: u8): Cmyk;
fn cmykToRgb(c: f64, m: f64, y: f64, k: f64): Rgb;
fn rgbToAnsi256(r: u8, g: u8, b: u8): u8;
fn ansi256ToRgb(code: u8): Rgb;
fn rgbToAnsi16(r: u8, g: u8, b: u8): u8;
fn ansi256ToAnsi16(code: u8): u8;
fn ansi16ToRgb(code: u8): Rgb;
fn rgbToHex(r: u8, g: u8, b: u8, out: []u8): usize;
fn hexToRgb(hex: string): RgbColor;
fn keywordToRgb(name: string): RgbColor;
fn rgbToKeyword(r: u8, g: u8, b: u8): string;
fn rgbToGray(r: u8, g: u8, b: u8): f64;
fn grayToRgb(gray: f64): Rgb;
```

## Minimal Usage

```cp
import { hexToRgb } from "color-convert";

pub fn main() {
    let result = hexToRgb("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"color-convert"`; install via `cap add color-convert`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
