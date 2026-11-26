// @cap/math-interp — Zero-heap interpolation, clamping, remapping & easing curves
// Clean-room MIT implementation in pure CP.

pub const PI: f64 = 3.14159265358979323846;
pub const TAU: f64 = 6.28318530717958647692;

pub fn absF64(x: f64): f64 {
    if (x < 0.0) { return -x; }
    return x;
}

pub fn minF64(a: f64, b: f64): f64 {
    if (a < b) { return a; }
    return b;
}

pub fn maxF64(a: f64, b: f64): f64 {
    if (a > b) { return a; }
    return b;
}

pub fn clamp(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) { return lo; }
    if (x > hi) { return hi; }
    return x;
}

pub fn clamp01(x: f64): f64 {
    return clamp(x, 0.0, 1.0);
}

pub fn approxEq(a: f64, b: f64, eps: f64): bool {
    return absF64(a - b) <= eps;
}

pub fn lerp(a: f64, b: f64, t: f64): f64 {
    return a + (b - a) * t;
}

pub fn lerpClamped(a: f64, b: f64, t: f64): f64 {
    return lerp(a, b, clamp01(t));
}

pub fn inverseLerp(a: f64, b: f64, value: f64): f64 {
    let denom = b - a;
    if (denom == 0.0) { return 0.0; }
    return (value - a) / denom;
}

pub fn remap(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64 {
    let t = inverseLerp(in_min, in_max, value);
    return lerp(out_min, out_max, t);
}

pub fn remapClamped(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64 {
    let t = clamp01(inverseLerp(in_min, in_max, value));
    return lerp(out_min, out_max, t);
}

pub fn smoothstep(edge0: f64, edge1: f64, x: f64): f64 {
    let t = clamp01(inverseLerp(edge0, edge1, x));
    return t * t * (3.0 - 2.0 * t);
}

pub fn smootherstep(edge0: f64, edge1: f64, x: f64): f64 {
    let t = clamp01(inverseLerp(edge0, edge1, x));
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

pub fn step(edge: f64, x: f64): f64 {
    if (x < edge) { return 0.0; }
    return 1.0;
}

pub fn wrap(x: f64, lo: f64, hi: f64): f64 {
    let span = hi - lo;
    if (span <= 0.0) { return lo; }
    let v = x - lo;
    while (v < 0.0) {
        v = v + span;
    }
    while (v >= span) {
        v = v - span;
    }
    return lo + v;
}

pub fn pingPong(t: f64, length: f64): f64 {
    if (length <= 0.0) { return 0.0; }
    let cycle = length * 2.0;
    let r = wrap(t, 0.0, cycle);
    if (r <= length) { return r; }
    return cycle - r;
}

pub fn degToRad(deg: f64): f64 {
    return deg * (PI / 180.0);
}

pub fn radToDeg(rad: f64): f64 {
    return rad * (180.0 / PI);
}

pub fn easeInQuad(t: f64): f64 {
    let u = clamp01(t);
    return u * u;
}

pub fn easeOutQuad(t: f64): f64 {
    let u = clamp01(t);
    return 1.0 - (1.0 - u) * (1.0 - u);
}

pub fn easeInOutQuad(t: f64): f64 {
    let u = clamp01(t);
    if (u < 0.5) {
        return 2.0 * u * u;
    }
    let f = -2.0 * u + 2.0;
    return 1.0 - (f * f) / 2.0;
}

pub fn easeInCubic(t: f64): f64 {
    let u = clamp01(t);
    return u * u * u;
}

pub fn easeOutCubic(t: f64): f64 {
    let u = 1.0 - clamp01(t);
    return 1.0 - u * u * u;
}

pub fn easeInOutCubic(t: f64): f64 {
    let u = clamp01(t);
    if (u < 0.5) {
        return 4.0 * u * u * u;
    }
    let f = -2.0 * u + 2.0;
    return 1.0 - (f * f * f) / 2.0;
}
