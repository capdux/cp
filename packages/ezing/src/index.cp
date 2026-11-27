// @cap/easing — Zero-allocation Robert Penner easing functions & CSS cubic-bezier solver for CP
// Clean-room MIT implementation inspired by Rust `ezing` / `keyframe` and npm `eases`.

const PI: f64 = 3.14159265358979323846;
const HALF_PI: f64 = 1.57079632679489661923;
const TWO_PI: f64 = 6.28318530717958647692;

fn clamp01(t: f64) -> f64 {
    if (t <= 0.0) {
        return 0.0;
    }
    if (t >= 1.0) {
        return 1.0;
    }
    return t;
}

fn absF64(x: f64) -> f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

fn sqrtF64(x: f64) -> f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 64) {
        const next: f64 = 0.5 * (g + (x / g));
        const diff: f64 = absF64(next - g);
        g = next;
        if (diff <= 0.000000000000001 * g) {
            break;
        }
        i += 1;
    }
    return g;
}

fn sinF64(x_in: f64) -> f64 {
    let x: f64 = x_in;
    const turns: f64 = ((x / TWO_PI) as i64) as f64;
    x -= turns * TWO_PI;
    while (x > PI) {
        x -= TWO_PI;
    }
    while (x < -PI) {
        x += TWO_PI;
    }
    if (x > HALF_PI) {
        x = PI - x;
    } else if (x < -HALF_PI) {
        x = -PI - x;
    }
    const x2: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let k: usize = 1;
    while (k <= 10) {
        const d1: f64 = (2 * k) as f64;
        const d2: f64 = ((2 * k) + 1) as f64;
        term = (-term * x2) / (d1 * d2);
        sum += term;
        k += 1;
    }
    return sum;
}

fn cosF64(x: f64) -> f64 {
    return sinF64(x + HALF_PI);
}

// --- Linear ---
pub fn linear(t: f64) -> f64 {
    return clamp01(t);
}

// --- Quadratic ---
pub fn quadIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    return t * t;
}

pub fn quadOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    const inv: f64 = 1.0 - t;
    return 1.0 - (inv * inv);
}

pub fn quadInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t < 0.5) {
        return 2.0 * t * t;
    }
    const f: f64 = (-2.0 * t) + 2.0;
    return 1.0 - ((f * f) * 0.5);
}

// --- Cubic ---
pub fn cubicIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    return t * t * t;
}

pub fn cubicOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    const inv: f64 = 1.0 - t;
    return 1.0 - (inv * inv * inv);
}

pub fn cubicInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t < 0.5) {
        return 4.0 * t * t * t;
    }
    const f: f64 = (-2.0 * t) + 2.0;
    return 1.0 - ((f * f * f) * 0.5);
}

// --- Quartic ---
pub fn quartIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    const t2: f64 = t * t;
    return t2 * t2;
}

pub fn quartOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    const inv: f64 = 1.0 - t;
    const inv2: f64 = inv * inv;
    return 1.0 - (inv2 * inv2);
}

pub fn quartInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t < 0.5) {
        const t2: f64 = t * t;
        return 8.0 * t2 * t2;
    }
    const f: f64 = (-2.0 * t) + 2.0;
    const f2: f64 = f * f;
    return 1.0 - ((f2 * f2) * 0.5);
}

// --- Quintic ---
pub fn quintIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    const t2: f64 = t * t;
    return t2 * t2 * t;
}

pub fn quintOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    const inv: f64 = 1.0 - t;
    const inv2: f64 = inv * inv;
    return 1.0 - (inv2 * inv2 * inv);
}

pub fn quintInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t < 0.5) {
        const t2: f64 = t * t;
        return 16.0 * t2 * t2 * t;
    }
    const f: f64 = (-2.0 * t) + 2.0;
    const f2: f64 = f * f;
    return 1.0 - ((f2 * f2 * f) * 0.5);
}

// --- Sine ---
pub fn sineIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t == 0.0) { return 0.0; }
    if (t == 1.0) { return 1.0; }
    return 1.0 - cosF64(t * HALF_PI);
}

pub fn sineOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t == 0.0) { return 0.0; }
    if (t == 1.0) { return 1.0; }
    return sinF64(t * HALF_PI);
}

pub fn sineInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t == 0.0) { return 0.0; }
    if (t == 1.0) { return 1.0; }
    return -(cosF64(PI * t) - 1.0) * 0.5;
}

// --- Circular ---
pub fn circIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    return 1.0 - sqrtF64(1.0 - (t * t));
}

pub fn circOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    const f: f64 = t - 1.0;
    return sqrtF64(1.0 - (f * f));
}

pub fn circInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t < 0.5) {
        const s: f64 = 2.0 * t;
        return (1.0 - sqrtF64(1.0 - (s * s))) * 0.5;
    }
    const s: f64 = (-2.0 * t) + 2.0;
    return (sqrtF64(1.0 - (s * s)) + 1.0) * 0.5;
}

// --- Back (overshoot) ---
pub fn backIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t == 0.0) { return 0.0; }
    if (t == 1.0) { return 1.0; }
    const c1: f64 = 1.70158;
    const c3: f64 = c1 + 1.0;
    return (c3 * t * t * t) - (c1 * t * t);
}

pub fn backOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t == 0.0) { return 0.0; }
    if (t == 1.0) { return 1.0; }
    const c1: f64 = 1.70158;
    const c3: f64 = c1 + 1.0;
    const f: f64 = t - 1.0;
    return 1.0 + (c3 * f * f * f) + (c1 * f * f);
}

pub fn backInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t == 0.0) { return 0.0; }
    if (t == 1.0) { return 1.0; }
    const c1: f64 = 1.70158;
    const c2: f64 = c1 * 1.525;
    if (t < 0.5) {
        const s: f64 = 2.0 * t;
        return ((s * s) * (((c2 + 1.0) * 2.0 * t) - c2)) * 0.5;
    }
    const s: f64 = (2.0 * t) - 2.0;
    return (((s * s) * (((c2 + 1.0) * s) + c2)) + 2.0) * 0.5;
}

// --- Bounce ---
pub fn bounceOut(t_in: f64) -> f64 {
    let t: f64 = clamp01(t_in);
    const n1: f64 = 7.5625;
    const d1: f64 = 2.75;
    if (t < (1.0 / d1)) {
        return n1 * t * t;
    }
    if (t < (2.0 / d1)) {
        t -= 1.5 / d1;
        return (n1 * t * t) + 0.75;
    }
    if (t < (2.5 / d1)) {
        t -= 2.25 / d1;
        return (n1 * t * t) + 0.9375;
    }
    t -= 2.625 / d1;
    return (n1 * t * t) + 0.984375;
}

pub fn bounceIn(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    return 1.0 - bounceOut(1.0 - t);
}

pub fn bounceInOut(t_in: f64) -> f64 {
    const t: f64 = clamp01(t_in);
    if (t < 0.5) {
        return (1.0 - bounceOut(1.0 - (2.0 * t))) * 0.5;
    }
    return (1.0 + bounceOut((2.0 * t) - 1.0)) * 0.5;
}

// --- CSS cubic-bezier(x1, y1, x2, y2) ---
fn sampleCurve(a: f64, b: f64, c: f64, u: f64) -> f64 {
    return ((((a * u) + b) * u) + c) * u;
}

fn sampleCurveDeriv(a: f64, b: f64, c: f64, u: f64) -> f64 {
    return (((3.0 * a * u) + (2.0 * b)) * u) + c;
}

pub fn cubicBezier(x1: f64, y1: f64, x2: f64, y2: f64, x_in: f64) -> f64 {
    const x: f64 = clamp01(x_in);
    if (x == 0.0) { return 0.0; }
    if (x == 1.0) { return 1.0; }
    if (x1 == y1 && x2 == y2) {
        return x;
    }

    const cx: f64 = 3.0 * x1;
    const bx: f64 = (3.0 * (x2 - x1)) - cx;
    const ax: f64 = 1.0 - cx - bx;

    const cy: f64 = 3.0 * y1;
    const by: f64 = (3.0 * (y2 - y1)) - cy;
    const ay: f64 = 1.0 - cy - by;

    // Newton-Raphson fast path
    let u: f64 = x;
    let converged = false;
    let iter: usize = 0;
    while (iter < 8) {
        const curX: f64 = sampleCurve(ax, bx, cx, u) - x;
        if (absF64(curX) < 0.000000000001) {
            converged = true;
            break;
        }
        const d: f64 = sampleCurveDeriv(ax, bx, cx, u);
        if (absF64(d) < 0.0000001) {
            break;
        }
        u = u - (curX / d);
        if (u < 0.0 || u > 1.0) {
            break;
        }
        iter += 1;
    }

    // Bisection fallback for guaranteed convergence
    if (!converged) {
        let lo: f64 = 0.0;
        let hi: f64 = 1.0;
        u = x;
        let bIter: usize = 0;
        while (bIter < 40) {
            const curX: f64 = sampleCurve(ax, bx, cx, u);
            if (absF64(curX - x) < 0.000000000001) {
                break;
            }
            if (x > curX) {
                lo = u;
            } else {
                hi = u;
            }
            u = (lo + hi) * 0.5;
            bIter += 1;
        }
    }

    return sampleCurve(ay, by, cy, u);
}

export {
    linear,
    quadIn,
    quadOut,
    quadInOut,
    cubicIn,
    cubicOut,
    cubicInOut,
    quartIn,
    quartOut,
    quartInOut,
    quintIn,
    quintOut,
    quintInOut,
    sineIn,
    sineOut,
    sineInOut,
    circIn,
    circOut,
    circInOut,
    backIn,
    backOut,
    backInOut,
    bounceIn,
    bounceOut,
    bounceInOut,
    cubicBezier,
};
