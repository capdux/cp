# palette@1.0.0 (Agent Quickref)

> Zero-allocation RGB / HSL / HSV / CMYK color space conversions, hue rotation, lighten/darken, and linear RGB mixing for CP. | License: MIT
> Import: `import { ... } from "palette";` | Full types: `src/index.d.cp`

## Functions
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
