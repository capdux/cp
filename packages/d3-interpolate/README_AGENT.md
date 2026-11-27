# d3-interpolate@3.0.1 (Agent Quickref)

> Zero-allocation numeric, integer, RGB/HSL color, gamma-corrected, and uniform B-spline interpolators | License: ISC
> Import: `import { ... } from "d3-interpolate";` | Full types: `src/index.d.cp`

## Functions
- `export fn interpolateNumber(a: f64, b: f64, t: f64): f64;`
- `export fn interpolateRound(a: i64, b: i64, t: f64): i64;`
- `export fn interpolateDiscrete(values: &[]f64, t: f64): f64;`
- `export fn interpolateRgb(c0: RgbColor, c1: RgbColor, t: f64): RgbColor;`
- `export fn interpolateHue(a: f64, b: f64, t: f64): f64;`
- `export fn interpolateHsl(c0: HslColor, c1: HslColor, t: f64): HslColor;`
- `export fn basis(t1: f64, v0: f64, v1: f64, v2: f64, v3: f64): f64;`
- `export fn interpolateBasis(values: &[]f64, t: f64): f64;`
- `export fn interpolatePiecewise(values: &[]f64, t: f64): f64;`
