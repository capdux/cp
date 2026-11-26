// packages/@cap/vec2/src/index.cp
// Zero-allocation 2D vector and planar geometry library (zero external dependencies).
// MIT clean-room pure CP implementation.

/// 2D double-precision Cartesian vector.
pub struct Vec2 {
    pub x: f64,
    pub y: f64,
}

fn v2Abs(x: f64): f64 {
    if (x < (0.0 as f64)) {
        return -x;
    }
    return x;
}

/// High-precision Newton-Raphson square root for non-negative `f64`.
fn v2Sqrt(x: f64): f64 {
    if (x <= (0.0 as f64)) {
        return 0.0 as f64;
    }
    // Range-reduce x into [0.5, 2.0) by powers of 4 for fast uniform convergence.
    let m: f64 = x;
    let factor: f64 = 1.0;
    while (m >= (4.0 as f64)) {
        m = m * (0.25 as f64);
        factor = factor * (2.0 as f64);
    }
    while (m < (0.25 as f64)) {
        m = m * (4.0 as f64);
        factor = factor * (0.5 as f64);
    }
    let g: f64 = (0.5 as f64) * (m + (1.0 as f64));
    let i: usize = 0;
    while (i < 12) {
        g = (0.5 as f64) * (g + (m / g));
        i += 1;
    }
    return g * factor;
}

/// Reduce angle in radians into `[-PI, PI]`.
fn v2WrapPi(rad: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    const twoPi: f64 = 6.28318530717958647692;
    let a: f64 = rad;
    if (a > pi || a < -pi) {
        const k: f64 = (((a + pi) / twoPi) as i64) as f64;
        a = a - k * twoPi;
        while (a > pi) { a = a - twoPi; }
        while (a < -pi) { a = a + twoPi; }
    }
    return a;
}

fn v2Sin(rad: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    const halfPi: f64 = 1.57079632679489661923;
    let x: f64 = v2WrapPi(rad);
    if (x > halfPi) {
        x = pi - x;
    } else if (x < -halfPi) {
        x = -pi - x;
    }
    const x2: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let n: usize = 1;
    while (n <= 10) {
        const d1: f64 = (2 * n) as f64;
        const d2: f64 = (2 * n + 1) as f64;
        term = -term * x2 / (d1 * d2);
        sum = sum + term;
        n += 1;
    }
    return sum;
}

fn v2Cos(rad: f64): f64 {
    const halfPi: f64 = 1.57079632679489661923;
    return v2Sin(rad + halfPi);
}

/// Construct a `Vec2(x, y)`.
pub fn new(x: f64, y: f64): Vec2 {
    return Vec2{ x: x, y: y };
}

/// Construct the zero vector `(0, 0)`.
pub fn zero(): Vec2 {
    return Vec2{ x: 0.0 as f64, y: 0.0 as f64 };
}

/// Construct a vector `(v, v)` with both components equal to `v`.
pub fn splat(v: f64): Vec2 {
    return Vec2{ x: v, y: v };
}

/// Component-wise addition `a + b`.
pub fn add(a: Vec2, b: Vec2): Vec2 {
    return Vec2{ x: a.x + b.x, y: a.y + b.y };
}

/// Component-wise subtraction `a - b`.
pub fn sub(a: Vec2, b: Vec2): Vec2 {
    return Vec2{ x: a.x - b.x, y: a.y - b.y };
}

/// Component-wise multiplication `(a.x * b.x, a.y * b.y)`.
pub fn mul(a: Vec2, b: Vec2): Vec2 {
    return Vec2{ x: a.x * b.x, y: a.y * b.y };
}

/// Scalar multiplication `v * s`.
pub fn scale(v: Vec2, s: f64): Vec2 {
    return Vec2{ x: v.x * s, y: v.y * s };
}

/// Vector negation `(-v.x, -v.y)`.
pub fn neg(v: Vec2): Vec2 {
    return Vec2{ x: -v.x, y: -v.y };
}

/// Dot product `a.x * b.x + a.y * b.y`.
pub fn dot(a: Vec2, b: Vec2): f64 {
    return (a.x * b.x) + (a.y * b.y);
}

/// 2D scalar cross product (z-component) `a.x * b.y - a.y * b.x`.
pub fn cross(a: Vec2, b: Vec2): f64 {
    return (a.x * b.y) - (a.y * b.x);
}

/// Squared Euclidean length `v.x^2 + v.y^2`.
pub fn lengthSq(v: Vec2): f64 {
    return (v.x * v.x) + (v.y * v.y);
}

/// Euclidean length `sqrt(v.x^2 + v.y^2)`.
pub fn length(v: Vec2): f64 {
    return v2Sqrt(lengthSq(v));
}

/// Euclidean distance between `a` and `b`.
pub fn distance(a: Vec2, b: Vec2): f64 {
    return length(sub(a, b));
}

/// Unit-length direction of `v`, or `(0, 0)` safely when `v` is near zero.
pub fn normalize(v: Vec2): Vec2 {
    const l: f64 = length(v);
    if (l <= (0.000000000000001 as f64)) {
        return zero();
    }
    return scale(v, (1.0 as f64) / l);
}

/// Linear interpolation `a + (b - a) * t`.
pub fn lerp(a: Vec2, b: Vec2, t: f64): Vec2 {
    return Vec2{
        x: a.x + (b.x - a.x) * t,
        y: a.y + (b.y - a.y) * t,
    };
}

/// Counter-clockwise 90-degree perpendicular vector `(-v.y, v.x)`.
pub fn perp(v: Vec2): Vec2 {
    return Vec2{ x: -v.y, y: v.x };
}

/// Reflect incident vector `v` across unit surface `normal`: `v - 2 * dot(v, normal) * normal`.
pub fn reflect(v: Vec2, normal: Vec2): Vec2 {
    const k: f64 = (2.0 as f64) * dot(v, normal);
    return sub(v, scale(normal, k));
}

/// Orthogonal projection of `a` onto `onto` (returns `(0, 0)` safely if `onto` is zero).
pub fn project(a: Vec2, onto: Vec2): Vec2 {
    const denom: f64 = lengthSq(onto);
    if (denom <= (0.000000000000001 as f64)) {
        return zero();
    }
    return scale(onto, dot(a, onto) / denom);
}

/// Rotate `v` counter-clockwise by `radians`.
pub fn rotate(v: Vec2, radians: f64): Vec2 {
    const c: f64 = v2Cos(radians);
    const s: f64 = v2Sin(radians);
    return Vec2{
        x: (v.x * c) - (v.y * s),
        y: (v.x * s) + (v.y * c),
    };
}

/// Clamp the magnitude of `v` to at most `max_len` (returns `(0, 0)` if `max_len <= 0`).
pub fn clampLength(v: Vec2, max_len: f64): Vec2 {
    if (max_len <= (0.0 as f64)) {
        return zero();
    }
    const lsq: f64 = lengthSq(v);
    if (lsq > (max_len * max_len)) {
        return scale(v, max_len / v2Sqrt(lsq));
    }
    return v;
}

/// Component-wise approximate equality within tolerance `eps`.
pub fn approxEq(a: Vec2, b: Vec2, eps: f64): bool {
    return v2Abs(a.x - b.x) <= eps && v2Abs(a.y - b.y) <= eps;
}

export {
    Vec2,
    new,
    zero,
    splat,
    add,
    sub,
    mul,
    scale,
    neg,
    dot,
    cross,
    lengthSq,
    length,
    distance,
    normalize,
    lerp,
    perp,
    reflect,
    project,
    rotate,
    clampLength,
    approxEq,
};
