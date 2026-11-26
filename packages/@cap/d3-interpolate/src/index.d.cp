// Type definitions for CP

export class RgbColor {
    r: u8,
    g: u8,
    b: u8,
    a: f64,
}

export class HslColor {
    h: f64,
    s: f64,
    l: f64,
    a: f64,
}

export fn d3iClampU8(v: f64): u8;
export fn d3iClamp01(v: f64): f64;
export fn interpolateNumber(a: f64, b: f64, t: f64): f64;
export fn interpolateRound(a: i64, b: i64, t: f64): i64;
export fn interpolateDiscrete(values: &[]f64, t: f64): f64;
export fn interpolateRgb(c0: RgbColor, c1: RgbColor, t: f64): RgbColor;
export fn interpolateHue(a: f64, b: f64, t: f64): f64;
export fn interpolateHsl(c0: HslColor, c1: HslColor, t: f64): HslColor;
export fn basis(t1: f64, v0: f64, v1: f64, v2: f64, v3: f64): f64;
export fn interpolateBasis(values: &[]f64, t: f64): f64;
export fn interpolatePiecewise(values: &[]f64, t: f64): f64;
