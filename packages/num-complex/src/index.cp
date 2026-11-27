// @cap/num-complex — Zero-allocation Complex64 mathematical engine for CP
// Clean-room MIT implementation inspired by Rust `num-complex` (crates.io #317, 496M+ downloads).
//
// Features:
// - `Complex64{ re: f64, im: f64 }` with constructors (`new`, `zero`, `one`, `i`, `fromPolar`).
// - Full complex algebra: `add`, `sub`, `mul`, Smith overflow-safe `div`, `neg`, `conj`, `scale`, `inv`, `powi`.
// - Geometry & elementary complex functions: `normSqr`, `norm` (hypot), `arg` (atan2), `exp` (Euler), `sqrt`, `approxEq`.
// - Zero-allocation `formatComplex(z, out) -> string`.

const std = @import("std");

fn fabs(x: f64) -> f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

/// Double-precision complex number `re + im * i`.
pub struct Complex64 {
    pub re: f64,
    pub im: f64,

    pub static fn new(re: f64, im: f64) -> Complex64 {
        return Complex64{ re: re, im: im };
    }

    pub static fn zero() -> Complex64 {
        return Complex64{ re: 0.0, im: 0.0 };
    }

    pub static fn one() -> Complex64 {
        return Complex64{ re: 1.0, im: 0.0 };
    }

    pub static fn i() -> Complex64 {
        return Complex64{ re: 0.0, im: 1.0 };
    }

    /// Constructs a complex number from polar coordinates `(r, theta)`: `r * (cos(theta) + i * sin(theta))`.
    pub static fn fromPolar(r: f64, theta: f64) -> Complex64 {
        const c: f64 = std.math.cos(theta);
        const s: f64 = std.math.sin(theta);
        return Complex64{ re: r * c, im: r * s };
    }

    pub fn add(other: Complex64) -> Complex64 {
        return Complex64{ re: _.re + other.re, im: _.im + other.im };
    }

    pub fn sub(other: Complex64) -> Complex64 {
        return Complex64{ re: _.re - other.re, im: _.im - other.im };
    }

    pub fn mul(other: Complex64) -> Complex64 {
        return Complex64{
            re: (_.re * other.re) - (_.im * other.im),
            im: (_.re * other.im) + (_.im * other.re),
        };
    }

    /// Overflow-safe complex division using Smith's scaling formula.
    pub fn div(other: Complex64) -> Complex64 {
        if (fabs(other.re) >= fabs(other.im)) {
            const r = other.im / other.re;
            const denom = other.re + (other.im * r);
            return Complex64{
                re: (_.re + (_.im * r)) / denom,
                im: (_.im - (_.re * r)) / denom,
            };
        }
        const r = other.re / other.im;
        const denom = other.im + (other.re * r);
        return Complex64{
            re: ((_.re * r) + _.im) / denom,
            im: ((_.im * r) - _.re) / denom,
        };
    }

    pub fn neg() -> Complex64 {
        return Complex64{ re: -_.re, im: -_.im };
    }

    pub fn conj() -> Complex64 {
        return Complex64{ re: _.re, im: -_.im };
    }

    pub fn scale(s: f64) -> Complex64 {
        return Complex64{ re: _.re * s, im: _.im * s };
    }

    pub fn inv() -> Complex64 {
        return Complex64.one().div(Complex64{ re: _.re, im: _.im });
    }

    /// Integer exponentiation by squaring (supports negative, zero, and positive exponents).
    pub fn powi(exponent: i32) -> Complex64 {
        if (exponent == 0) {
            return Complex64.one();
        }
        const neg_exp = exponent < 0;
        let n: u32 = if (neg_exp) {
            ((0 - (exponent + 1)) as u32) + 1
        } else {
            exponent as u32
        };

        let base = Complex64{ re: _.re, im: _.im };
        let acc = Complex64.one();
        while (n > 0) {
            if ((n & 1) == 1) {
                acc = acc.mul(base);
            }
            base = base.mul(base);
            n = n >> 1;
        }
        if (neg_exp) {
            return acc.inv();
        }
        return acc;
    }

    /// Squared Euclidean norm `re^2 + im^2`.
    pub fn normSqr() -> f64 {
        return (_.re * _.re) + (_.im * _.im);
    }

    /// Overflow-safe Euclidean norm `hypot(re, im)`.
    pub fn norm() -> f64 {
        const h: f64 = std.math.hypot(_.re, _.im);
        return h;
    }

    /// Principal argument (phase angle in radians, `[-pi, pi]`).
    pub fn arg() -> f64 {
        const a: f64 = std.math.atan2(_.im, _.re);
        return a;
    }

    /// Complex exponential via Euler's formula: `exp(a + bi) = exp(a) * (cos(b) + i*sin(b))`.
    pub fn exp() -> Complex64 {
        const ea: f64 = std.math.exp(_.re);
        const cb: f64 = std.math.cos(_.im);
        const sb: f64 = std.math.sin(_.im);
        return Complex64{ re: ea * cb, im: ea * sb };
    }

    /// Principal square root with non-negative real part (`sqrt(-1) == i`).
    pub fn sqrt() -> Complex64 {
        const r = _.norm();
        if (r == 0.0) {
            return Complex64.zero();
        }
        const re_rad: f64 = (r + _.re) * 0.5;
        const im_rad: f64 = (r - _.re) * 0.5;
        const re_part: f64 = if (re_rad > 0.0) { std.math.sqrt(re_rad) } else { 0.0 };
        const im_mag: f64 = if (im_rad > 0.0) { std.math.sqrt(im_rad) } else { 0.0 };
        const im_part: f64 = if (_.im < 0.0) { -im_mag } else { im_mag };
        return Complex64{ re: re_part, im: im_part };
    }

    pub fn approxEq(other: Complex64, eps: f64) -> bool {
        return fabs(_.re - other.re) <= eps && fabs(_.im - other.im) <= eps;
    }
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushU64(out: []u8, pos: usize, val: u64) -> usize {
    let digits: [20]u8 = undefined;
    let count: usize = 0;
    let v = val;
    if (v == 0) {
        return pushByte(out, pos, '0');
    }
    while (v > 0 && count < 20) {
        digits[count] = (48 as u8) + ((v % 10) as u8);
        count += 1;
        v = v / 10;
    }
    let p = pos;
    let k = count;
    while (k > 0) {
        k -= 1;
        p = pushByte(out, p, digits[k]);
    }
    return p;
}

/// Formats a non-negative finite `f64` with up to 4 trimmed decimal places into `out`.
fn pushAbsFloat(out: []u8, pos: usize, v: f64) -> usize {
    const scaled: u64 = ((v * 10000.0) + 0.5) as u64;
    const whole: u64 = scaled / 10000;
    let frac: u64 = scaled % 10000;
    let p = pushU64(out, pos, whole);
    if (frac == 0) {
        return p;
    }
    p = pushByte(out, p, '.');
    let fbuf: [4]u8 = undefined;
    fbuf[0] = (48 as u8) + (((frac / 1000) % 10) as u8);
    fbuf[1] = (48 as u8) + (((frac / 100) % 10) as u8);
    fbuf[2] = (48 as u8) + (((frac / 10) % 10) as u8);
    fbuf[3] = (48 as u8) + ((frac % 10) as u8);
    let flen: usize = 4;
    while (flen > 1 && fbuf[flen - 1] == '0') {
        flen -= 1;
    }
    let i: usize = 0;
    while (i < flen) {
        p = pushByte(out, p, fbuf[i]);
        i += 1;
    }
    return p;
}

/// Formats a `Complex64` as `<re>+<im>i` or `<re>-<im>i` into caller-provided `out` (zero heap allocation).
pub fn formatComplex(z: Complex64, out: []u8) -> string {
    let pos: usize = 0;
    if (z.re < 0.0) {
        pos = pushByte(out, pos, '-');
        pos = pushAbsFloat(out, pos, -z.re);
    } else {
        pos = pushAbsFloat(out, pos, z.re);
    }

    if (z.im < 0.0) {
        pos = pushByte(out, pos, '-');
        pos = pushAbsFloat(out, pos, -z.im);
    } else {
        pos = pushByte(out, pos, '+');
        pos = pushAbsFloat(out, pos, z.im);
    }
    pos = pushByte(out, pos, 'i');
    return out[0..pos];
}
