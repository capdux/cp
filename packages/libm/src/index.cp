// @cap/libm — Pure CP zero-allocation IEEE 754 f64 math library
// Parity with Rust `libm` / C `<math.h>`, zero external dependencies.

pub const PI: f64 = 3.14159265358979323846;
pub const TAU: f64 = 6.28318530717958647692;
pub const FRAC_PI_2: f64 = 1.57079632679489661923;
pub const FRAC_PI_4: f64 = 0.78539816339744830962;
pub const E: f64 = 2.71828182845904523536;
pub const LN_2: f64 = 0.69314718055994530942;
pub const LN_10: f64 = 2.30258509299404568402;

const LM_TWO52: f64 = 4503599627370496.0;
const LM_LN2_HI: f64 = 0.69314718036912381649;
const LM_LN2_LO: f64 = 0.00000000019082149292705877;
const LM_PI2_HI: f64 = 1.57079632673412561417;
const LM_PI2_LO: f64 = 0.00000000006077100506506192;
const LM_SQRT2: f64 = 1.41421356237309504880;
const LM_SQRT05: f64 = 0.70710678118654752440;
const LM_SQRT3: f64 = 1.73205080756887729353;
const LM_TAN_PI12: f64 = 0.26794919243112270647;
const LM_FRAC_PI_6: f64 = 0.52359877559829887308;

fn lmNan(): f64 {
    let z: f64 = 0.0;
    return z / z;
}

fn lmInf(): f64 {
    let z: f64 = 0.0;
    return 1.0 / z;
}

fn lmScalePow2(val: f64, exp2: i64): f64 {
    let res = val;
    let k = exp2;
    if (k > 0) {
        let base: f64 = 2.0;
        while (k > 0) {
            if ((k & 1) != 0) {
                res = res * base;
            }
            k = k >> 1;
            if (k > 0) {
                base = base * base;
            }
        }
    } else if (k < 0) {
        let neg_k = -k;
        let base: f64 = 0.5;
        while (neg_k > 0) {
            if ((neg_k & 1) != 0) {
                res = res * base;
            }
            neg_k = neg_k >> 1;
            if (neg_k > 0) {
                base = base * base;
            }
        }
    }
    return res;
}

// ============================================================================
// Basic Operations
// ============================================================================

pub fn fabs(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (x < 0.0) {
        return -x;
    }
    if (x == 0.0) {
        return 0.0;
    }
    return x;
}

pub fn trunc(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (fabs(x) >= LM_TWO52) {
        return x;
    }
    let iv: i64 = x as i64;
    return iv as f64;
}

pub fn floor(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (fabs(x) >= LM_TWO52) {
        return x;
    }
    let t = trunc(x);
    if (x < t) {
        return t - 1.0;
    }
    return t;
}

pub fn ceil(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (fabs(x) >= LM_TWO52) {
        return x;
    }
    let t = trunc(x);
    if (x > t) {
        return t + 1.0;
    }
    return t;
}

pub fn round(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (fabs(x) >= LM_TWO52) {
        return x;
    }
    if (x >= 0.0) {
        return floor(x + 0.5);
    }
    return ceil(x - 0.5);
}

pub fn fmod(x: f64, y: f64): f64 {
    if (x != x || y != y || y == 0.0) {
        return lmNan();
    }
    let ax = fabs(x);
    let ay = fabs(y);
    if (ax < ay) {
        return x;
    }
    let rem = ax;
    if (ax / ay >= LM_TWO52) {
        let s = ay;
        while (s <= rem * 0.5) {
            s = s * 2.0;
        }
        while (rem >= ay) {
            if (rem >= s) {
                rem = rem - s;
            }
            s = s * 0.5;
        }
    } else {
        let q = trunc(ax / ay);
        rem = ax - q * ay;
        while (rem >= ay) {
            rem = rem - ay;
        }
        while (rem < 0.0) {
            rem = rem + ay;
        }
    }
    if (x < 0.0) {
        return -rem;
    }
    return rem;
}

pub fn copysign(x: f64, y: f64): f64 {
    let ax = fabs(x);
    if (y < 0.0) {
        return -ax;
    }
    if (y == 0.0) {
        if (1.0 / y < 0.0) {
            return -ax;
        }
    }
    return ax;
}

pub fn fmin(x: f64, y: f64): f64 {
    if (x != x) {
        return y;
    }
    if (y != y) {
        return x;
    }
    if (x < y) {
        return x;
    }
    return y;
}

pub fn fmax(x: f64, y: f64): f64 {
    if (x != x) {
        return y;
    }
    if (y != y) {
        return x;
    }
    if (x > y) {
        return x;
    }
    return y;
}

// ============================================================================
// Powers, Roots, Exponentials & Logarithms
// ============================================================================

pub fn sqrt(x: f64): f64 {
    if (x != x || x < 0.0) {
        return lmNan();
    }
    if (x == 0.0) {
        return 0.0;
    }
    if (x == lmInf()) {
        return x;
    }
    let m = x;
    let sc: f64 = 1.0;
    while (m >= 1048576.0) {
        m = m / 1048576.0;
        sc = sc * 1024.0;
    }
    while (m >= 4.0) {
        m = m * 0.25;
        sc = sc * 2.0;
    }
    while (m < 0.00000095367431640625) {
        m = m * 1048576.0;
        sc = sc / 1024.0;
    }
    while (m < 0.5) {
        m = m * 4.0;
        sc = sc * 0.5;
    }
    let g: f64 = 0.5 * (m + 1.0);
    let i: usize = 0;
    while (i < 8) {
        g = 0.5 * (g + m / g);
        i = i + 1;
    }
    return g * sc;
}

pub fn cbrt(x: f64): f64 {
    if (x != x || x == 0.0) {
        return x;
    }
    let neg = x < 0.0;
    let m = fabs(x);
    let sc: f64 = 1.0;
    while (m >= 16777216.0) {
        m = m / 16777216.0;
        sc = sc * 256.0;
    }
    while (m >= 8.0) {
        m = m * 0.125;
        sc = sc * 2.0;
    }
    while (m < 0.000000059604644775390625) {
        m = m * 16777216.0;
        sc = sc / 256.0;
    }
    while (m < 0.5) {
        m = m * 8.0;
        sc = sc * 0.5;
    }
    let g: f64 = 1.0;
    let i: usize = 0;
    while (i < 12) {
        g = (2.0 * g + m / (g * g)) / 3.0;
        i = i + 1;
    }
    let res = g * sc;
    if (neg) {
        return -res;
    }
    return res;
}

pub fn hypot(x: f64, y: f64): f64 {
    if (x != x || y != y) {
        return lmNan();
    }
    let ax = fabs(x);
    let ay = fabs(y);
    let hi = fmax(ax, ay);
    let lo = fmin(ax, ay);
    if (hi == 0.0) {
        return 0.0;
    }
    let r = lo / hi;
    return hi * sqrt(1.0 + r * r);
}

pub fn exp(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (x > 709.782712893384) {
        return lmInf();
    }
    if (x < -745.1332191019412) {
        return 0.0;
    }
    let kf = round(x / LN_2);
    let k: i64 = kf as i64;
    let r = (x - kf * LM_LN2_HI) - kf * LM_LN2_LO;
    let term: f64 = 1.0;
    let sum: f64 = 1.0;
    let i: usize = 1;
    while (i <= 20) {
        term = term * r / (i as f64);
        sum = sum + term;
        i = i + 1;
    }
    if (k < -1022) {
        let part1 = lmScalePow2(sum, -1000);
        return lmScalePow2(part1, k + 1000);
    }
    return lmScalePow2(sum, k);
}

pub fn ln(x: f64): f64 {
    if (x != x || x < 0.0) {
        return lmNan();
    }
    if (x == 0.0) {
        return -lmInf();
    }
    if (x == lmInf()) {
        return x;
    }
    if (x == 1.0) {
        return 0.0;
    }
    let m = x;
    let k: i64 = 0;
    while (m >= 1048576.0) {
        m = m / 1048576.0;
        k = k + 20;
    }
    while (m >= LM_SQRT2) {
        m = m * 0.5;
        k = k + 1;
    }
    while (m < 0.00000095367431640625) {
        m = m * 1048576.0;
        k = k - 20;
    }
    while (m < LM_SQRT05) {
        m = m * 2.0;
        k = k - 1;
    }
    let u = (m - 1.0) / (m + 1.0);
    let u_sq = u * u;
    let term = u;
    let sum = u;
    let d: f64 = 3.0;
    let i: usize = 0;
    while (i < 12) {
        term = term * u_sq;
        sum = sum + term / d;
        d = d + 2.0;
        i = i + 1;
    }
    return 2.0 * sum + (k as f64) * LN_2;
}

pub fn log2(x: f64): f64 {
    return ln(x) / LN_2;
}

pub fn log10(x: f64): f64 {
    return ln(x) / LN_10;
}

pub fn pow(x: f64, y: f64): f64 {
    if (y == 0.0) {
        return 1.0;
    }
    if (x == 1.0) {
        return 1.0;
    }
    if (x != x || y != y) {
        return lmNan();
    }
    let ay = fabs(y);
    if (y == trunc(y) && ay <= 1073741824.0) {
        let n: i64 = ay as i64;
        let base = x;
        let acc: f64 = 1.0;
        while (n > 0) {
            if ((n & 1) != 0) {
                acc = acc * base;
            }
            n = n >> 1;
            if (n > 0) {
                base = base * base;
            }
        }
        if (y < 0.0) {
            return 1.0 / acc;
        }
        return acc;
    }
    if (x == 0.0) {
        if (y > 0.0) {
            return 0.0;
        }
        return lmInf();
    }
    if (x < 0.0) {
        if (y != trunc(y)) {
            return lmNan();
        }
        return exp(y * ln(-x));
    }
    return exp(y * ln(x));
}

// ============================================================================
// Trigonometric & Inverse Trigonometric Functions
// ============================================================================

fn lmSinCore(r: f64): f64 {
    let r2 = r * r;
    let term = r;
    let sum = r;
    let i: usize = 1;
    while (i <= 9) {
        let f1 = (2 * i) as f64;
        let f2 = (2 * i + 1) as f64;
        term = -term * r2 / (f1 * f2);
        sum = sum + term;
        i = i + 1;
    }
    return sum;
}

fn lmCosCore(r: f64): f64 {
    let r2 = r * r;
    let term: f64 = 1.0;
    let sum: f64 = 1.0;
    let i: usize = 1;
    while (i <= 9) {
        let f1 = (2 * i - 1) as f64;
        let f2 = (2 * i) as f64;
        term = -term * r2 / (f1 * f2);
        sum = sum + term;
        i = i + 1;
    }
    return sum;
}

struct LmTrigRed {
    r: f64,
    quad: i64,
}

fn lmReduceTrig(x: f64): LmTrigRed {
    let ax = x;
    if (fabs(ax) > 10000000.0) {
        ax = fmod(ax, TAU);
    }
    let qf = round(ax / FRAC_PI_2);
    let q: i64 = qf as i64;
    let r = (ax - qf * LM_PI2_HI) - qf * LM_PI2_LO;
    let quad = q & 3;
    return LmTrigRed { r: r, quad: quad };
}

pub fn sin(x: f64): f64 {
    if (x != x) {
        return x;
    }
    let red = lmReduceTrig(x);
    if (red.quad == 0) {
        return lmSinCore(red.r);
    }
    if (red.quad == 1) {
        return lmCosCore(red.r);
    }
    if (red.quad == 2) {
        return -lmSinCore(red.r);
    }
    return -lmCosCore(red.r);
}

pub fn cos(x: f64): f64 {
    if (x != x) {
        return x;
    }
    let red = lmReduceTrig(x);
    if (red.quad == 0) {
        return lmCosCore(red.r);
    }
    if (red.quad == 1) {
        return -lmSinCore(red.r);
    }
    if (red.quad == 2) {
        return -lmCosCore(red.r);
    }
    return lmSinCore(red.r);
}

pub fn tan(x: f64): f64 {
    if (x != x) {
        return x;
    }
    return sin(x) / cos(x);
}

fn lmAtanUnit(u: f64): f64 {
    let v = u;
    let offset: f64 = 0.0;
    if (v > LM_TAN_PI12) {
        v = (v * LM_SQRT3 - 1.0) / (LM_SQRT3 + v);
        offset = LM_FRAC_PI_6;
    }
    let v2 = v * v;
    let term = v;
    let sum = v;
    let d: f64 = 3.0;
    let i: usize = 0;
    while (i < 14) {
        term = -term * v2;
        sum = sum + term / d;
        d = d + 2.0;
        i = i + 1;
    }
    return offset + sum;
}

pub fn atan(x: f64): f64 {
    if (x != x) {
        return x;
    }
    let neg = x < 0.0;
    let ax = fabs(x);
    let res: f64 = 0.0;
    if (ax > 1.0) {
        res = FRAC_PI_2 - lmAtanUnit(1.0 / ax);
    } else {
        res = lmAtanUnit(ax);
    }
    if (neg) {
        return -res;
    }
    return res;
}

pub fn atan2(y: f64, x: f64): f64 {
    if (x != x || y != y) {
        return lmNan();
    }
    if (x > 0.0) {
        return atan(y / x);
    }
    if (x < 0.0) {
        if (y >= 0.0) {
            return atan(y / x) + PI;
        }
        return atan(y / x) - PI;
    }
    if (y > 0.0) {
        return FRAC_PI_2;
    }
    if (y < 0.0) {
        return -FRAC_PI_2;
    }
    return 0.0;
}

pub fn asin(x: f64): f64 {
    if (x != x || x < -1.0 || x > 1.0) {
        return lmNan();
    }
    if (x == 1.0) {
        return FRAC_PI_2;
    }
    if (x == -1.0) {
        return -FRAC_PI_2;
    }
    return atan2(x, sqrt(1.0 - x * x));
}

pub fn acos(x: f64): f64 {
    if (x != x || x < -1.0 || x > 1.0) {
        return lmNan();
    }
    return atan2(sqrt(1.0 - x * x), x);
}

// ============================================================================
// Hyperbolic Functions
// ============================================================================

pub fn sinh(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (fabs(x) < 0.0001) {
        let x2 = x * x;
        return x * (1.0 + x2 / 6.0 + x2 * x2 / 120.0);
    }
    return 0.5 * (exp(x) - exp(-x));
}

pub fn cosh(x: f64): f64 {
    if (x != x) {
        return x;
    }
    return 0.5 * (exp(x) + exp(-x));
}

pub fn tanh(x: f64): f64 {
    if (x != x) {
        return x;
    }
    if (x > 20.0) {
        return 1.0;
    }
    if (x < -20.0) {
        return -1.0;
    }
    if (fabs(x) < 0.0001) {
        let x2 = x * x;
        return x * (1.0 - x2 / 3.0 + 2.0 * x2 * x2 / 15.0);
    }
    let e2 = exp(2.0 * x);
    return (e2 - 1.0) / (e2 + 1.0);
}

export {
    PI,
    TAU,
    FRAC_PI_2,
    FRAC_PI_4,
    E,
    LN_2,
    LN_10,
    fabs,
    floor,
    ceil,
    trunc,
    round,
    fmod,
    copysign,
    fmin,
    fmax,
    sqrt,
    cbrt,
    hypot,
    exp,
    ln,
    log2,
    log10,
    pow,
    sin,
    cos,
    tan,
    asin,
    acos,
    atan,
    atan2,
    sinh,
    cosh,
    tanh
};
