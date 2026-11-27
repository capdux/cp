# palette — AI Agent Quick Ref

Zero-allocation RGB / HSL / HSV / CMYK color space conversions, hue rotation, lighten/darken, and linear RGB mixing for CP. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class Rgb { r: f64, g: f64, b: f64 }
class Hsl { h: f64, s: f64, l: f64 }
class Hsv { h: f64, s: f64, v: f64 }
class Cmyk { c: f64, m: f64, y: f64, k: f64 }
```

## API Signatures

```cp
fn chAbs(x: f64): f64;
fn chClamp01(x: f64): f64;
fn chNormHue(h: f64): f64;
fn chSectorRgb(hn: f64, chroma: f64, m: f64): Rgb;
fn rgbFromU8(r: u8, g: u8, b: u8): Rgb;
fn rgbToHsl(c: Rgb): Hsl;
fn hslToRgb(c: Hsl): Rgb;
fn rgbToHsv(c: Rgb): Hsv;
fn hsvToRgb(c: Hsv): Rgb;
fn rgbToCmyk(c: Rgb): Cmyk;
fn cmykToRgb(c: Cmyk): Rgb;
fn rotateHue(c: Hsl, degrees: f64): Hsl;
fn lighten(c: Hsl, amount: f64): Hsl;
fn darken(c: Hsl, amount: f64): Hsl;
fn mixRgb(a: Rgb, b: Rgb, t: f64): Rgb;
```

## Minimal Usage

```cp
import { chAbs } from "palette";

pub fn main() {
    let result = chAbs(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"palette"`; install via `cap add palette`.
