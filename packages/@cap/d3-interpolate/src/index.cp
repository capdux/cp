// @cap/d3-interpolate — Numeric, RGB/HSL color, and B-spline interpolators in pure CP
// Clean-room ISC implementation of npm `d3-interpolate` (#503, 112.5M/wk).

pub struct RgbColor {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: f64,
}

pub struct HslColor {
    pub h: f64, // degrees [0, 360)
    pub s: f64, // [0, 1]
    pub l: f64, // [0, 1]
    pub a: f64, // [0, 1]
}

fn d3iClampU8(v: f64): u8 {
    if (v <= 0.0) { return 0; }
    if (v >= 255.0) { return 255; }
    return (v + 0.5) as u8;
}

fn d3iClamp01(v: f64): f64 {
    if (v < 0.0) { return 0.0; }
    if (v > 1.0) { return 1.0; }
    return v;
}

/// Linear numeric interpolation: `a * (1 - t) + b * t`.
pub fn interpolateNumber(a: f64, b: f64, t: f64): f64 {
    return a * (1.0 - t) + b * t;
}

/// Rounded integer interpolation between `a` and `b`.
pub fn interpolateRound(a: i64, b: i64, t: f64): i64 {
    const v = (a as f64) * (1.0 - t) + (b as f64) * t;
    if (v >= 0.0) {
        return (v + 0.5) as i64;
    }
    return (v - 0.5) as i64;
}

/// Discrete step interpolation picking an element from `values` for `t in [0, 1]`.
pub fn interpolateDiscrete(values: []const f64, t: f64): f64 {
    if (values.len == 0) {
        return 0.0;
    }
    if (t <= 0.0) {
        return values[0];
    }
    if (t >= 1.0) {
        return values[values.len - 1];
    }
    const idx = (t * (values.len as f64)) as usize;
    if (idx >= values.len) {
        return values[values.len - 1];
    }
    return values[idx];
}

/// Linear RGBA color interpolation.
pub fn interpolateRgb(c0: RgbColor, c1: RgbColor, t: f64): RgbColor {
    return RgbColor {
        r: d3iClampU8(interpolateNumber(c0.r as f64, c1.r as f64, t)),
        g: d3iClampU8(interpolateNumber(c0.g as f64, c1.g as f64, t)),
        b: d3iClampU8(interpolateNumber(c0.b as f64, c1.b as f64, t)),
        a: d3iClamp01(interpolateNumber(c0.a, c1.a, t)),
    };
}

/// Shortest-arc hue interpolation in degrees `[0, 360)`.
pub fn interpolateHue(a: f64, b: f64, t: f64): f64 {
    let d = b - a;
    if (d > 180.0) {
        d -= 360.0;
    } else if (d < -180.0) {
        d += 360.0;
    }
    let res = a + d * t;
    while (res < 0.0) { res += 360.0; }
    while (res >= 360.0) { res -= 360.0; }
    return res;
}

/// Shortest-arc HSL color interpolation.
pub fn interpolateHsl(c0: HslColor, c1: HslColor, t: f64): HslColor {
    return HslColor {
        h: interpolateHue(c0.h, c1.h, t),
        s: d3iClamp01(interpolateNumber(c0.s, c1.s, t)),
        l: d3iClamp01(interpolateNumber(c0.l, c1.l, t)),
        a: d3iClamp01(interpolateNumber(c0.a, c1.a, t)),
    };
}

/// Uniform cubic B-spline basis evaluation for 4 control points `(v0, v1, v2, v3)` at `t in [0, 1]`.
pub fn basis(t1: f64, v0: f64, v1: f64, v2: f64, v3: f64): f64 {
    const t2 = t1 * t1;
    const t3 = t2 * t1;
    return ((1.0 - 3.0 * t1 + 3.0 * t2 - t3) * v0
        + (4.0 - 6.0 * t2 + 3.0 * t3) * v1
        + (1.0 + 3.0 * t1 + 3.0 * t2 - 3.0 * t3) * v2
        + t3 * v3) / 6.0;
}

/// Evaluates an open uniform cubic B-spline through `values` at `t in [0, 1]`.
pub fn interpolateBasis(values: []const f64, t: f64): f64 {
    if (values.len == 0) { return 0.0; }
    if (values.len == 1) { return values[0]; }
    const n = values.len - 1;
    if (t <= 0.0) { return values[0]; }
    if (t >= 1.0) { return values[n]; }
    const scaled = t * (n as f64);
    let i = scaled as usize;
    if (i >= n) { i = n - 1; }
    const local_t = scaled - (i as f64);
    const v1 = values[i];
    const v2 = values[i + 1];
    const v0 = if (i > 0) values[i - 1] else 2.0 * v1 - v2;
    const v3 = if (i < n - 1) values[i + 2] else 2.0 * v2 - v1;
    return basis(local_t, v0, v1, v2, v3);
}

/// Piecewise linear interpolation through `values` at `t in [0, 1]`.
pub fn interpolatePiecewise(values: []const f64, t: f64): f64 {
    if (values.len == 0) { return 0.0; }
    if (values.len == 1) { return values[0]; }
    if (t <= 0.0) { return values[0]; }
    if (t >= 1.0) { return values[values.len - 1]; }
    const n = values.len - 1;
    const scaled = t * (n as f64);
    let i = scaled as usize;
    if (i >= n) { i = n - 1; }
    const local_t = scaled - (i as f64);
    return interpolateNumber(values[i], values[i + 1], local_t);
}

export {
    RgbColor,
    HslColor,
    interpolateNumber,
    interpolateRound,
    interpolateDiscrete,
    interpolateRgb,
    interpolateHue,
    interpolateHsl,
    basis,
    interpolateBasis,
    interpolatePiecewise,
};
