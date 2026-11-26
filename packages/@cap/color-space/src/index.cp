// @cap/color-space — Zero-allocation sRGB/Linear/HSL/HSV/OKLab/OKLCH color space engine for CP
// Clean-room MIT implementation inspired by Rust `palette` and npm `color-convert`.

const PI: f64 = 3.14159265358979323846;
const HALF_PI: f64 = 1.57079632679489661923;
const TWO_PI: f64 = 6.28318530717958647692;
const LN2: f64 = 0.69314718055994530942;
const SQRT3: f64 = 1.73205080756887729352;
const TAN_PI_12: f64 = 0.26794919243112270647;
const PI_6: f64 = 0.52359877559829887307;

pub struct Rgb {
    pub r: f64,
    pub g: f64,
    pub b: f64,
}

pub struct Hsl {
    pub h: f64,
    pub s: f64,
    pub l: f64,
}

pub struct Hsv {
    pub h: f64,
    pub s: f64,
    pub v: f64,
}

pub struct Oklab {
    pub l: f64,
    pub a: f64,
    pub b: f64,
}

pub struct Oklch {
    pub l: f64,
    pub c: f64,
    pub h: f64,
}

fn clamp01(x: f64) -> f64 {
    if (x <= 0.0) { return 0.0; }
    if (x >= 1.0) { return 1.0; }
    return x;
}

fn absF64(x: f64) -> f64 {
    if (x < 0.0) { return -x; }
    return x;
}

fn sqrtF64(x: f64) -> f64 {
    if (x <= 0.0) { return 0.0; }
    let g: f64 = x;
    if (g < 1.0) { g = 1.0; }
    let i: usize = 0;
    while (i < 64) {
        const next: f64 = 0.5 * (g + (x / g));
        const diff: f64 = absF64(next - g);
        g = next;
        if (diff <= 0.000000000000001 * g) { break; }
        i += 1;
    }
    return g;
}

fn cbrtF64(x_in: f64) -> f64 {
    if (x_in == 0.0) { return 0.0; }
    let neg = false;
    let x: f64 = x_in;
    if (x < 0.0) {
        neg = true;
        x = -x;
    }
    let g: f64 = x;
    if (g < 1.0) { g = 1.0; }
    let i: usize = 0;
    while (i < 64) {
        const next: f64 = ((2.0 * g) + (x / (g * g))) / 3.0;
        const diff: f64 = absF64(next - g);
        g = next;
        if (diff <= 0.000000000000001 * g) { break; }
        i += 1;
    }
    if (neg) { return -g; }
    return g;
}

fn lnF64(x_in: f64) -> f64 {
    if (x_in <= 0.0) { return 0.0; }
    let m: f64 = x_in;
    let k: i32 = 0;
    while (m >= 1.4142135623730951) {
        m *= 0.5;
        k += 1 as i32;
    }
    while (m < 0.7071067811865476) {
        m *= 2.0;
        k -= 1 as i32;
    }
    const y: f64 = (m - 1.0) / (m + 1.0);
    const y2: f64 = y * y;
    let term: f64 = y;
    let sum: f64 = y;
    let n: usize = 1;
    while (n <= 12) {
        term *= y2;
        sum += term / (((2 * n) + 1) as f64);
        n += 1;
    }
    return (2.0 * sum) + ((k as f64) * LN2);
}

fn expF64(x_in: f64) -> f64 {
    if (x_in > 700.0) { return 1000000000000.0; }
    if (x_in < -700.0) { return 0.0; }
    let k: i32 = (x_in / LN2) as i32;
    let r: f64 = x_in - ((k as f64) * LN2);
    while (r > 0.35) {
        r -= LN2;
        k += 1 as i32;
    }
    while (r < -0.35) {
        r += LN2;
        k -= 1 as i32;
    }
    let term: f64 = 1.0;
    let sum: f64 = 1.0;
    let n: usize = 1;
    while (n <= 16) {
        term = (term * r) / (n as f64);
        sum += term;
        n += 1;
    }
    while (k > (0 as i32)) {
        sum *= 2.0;
        k -= 1 as i32;
    }
    while (k < (0 as i32)) {
        sum *= 0.5;
        k += 1 as i32;
    }
    return sum;
}

fn powF64(base: f64, exponent: f64) -> f64 {
    if (base <= 0.0) { return 0.0; }
    return expF64(exponent * lnF64(base));
}

fn sinF64(x_in: f64) -> f64 {
    let x: f64 = x_in;
    const turns: f64 = ((x / TWO_PI) as i64) as f64;
    x -= turns * TWO_PI;
    while (x > PI) { x -= TWO_PI; }
    while (x < -PI) { x += TWO_PI; }
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
        term = (-term * x2) / (((2 * k) as f64) * (((2 * k) + 1) as f64));
        sum += term;
        k += 1;
    }
    return sum;
}

fn cosF64(x: f64) -> f64 {
    return sinF64(x + HALF_PI);
}

fn atanF64(x_in: f64) -> f64 {
    let neg = false;
    let x: f64 = x_in;
    if (x < 0.0) { neg = true; x = -x; }
    let invert = false;
    if (x > 1.0) { invert = true; x = 1.0 / x; }
    let shifted = false;
    if (x > TAN_PI_12) {
        shifted = true;
        x = ((x * SQRT3) - 1.0) / (x + SQRT3);
    }
    const x2: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let k: usize = 1;
    while (k <= 12) {
        term = -term * x2;
        sum += term / (((2 * k) + 1) as f64);
        k += 1;
    }
    if (shifted) { sum += PI_6; }
    if (invert) { sum = HALF_PI - sum; }
    if (neg) { sum = -sum; }
    return sum;
}

fn atan2F64(y: f64, x: f64) -> f64 {
    if (x > 0.0) { return atanF64(y / x); }
    if (x < 0.0) {
        if (y >= 0.0) { return atanF64(y / x) + PI; }
        return atanF64(y / x) - PI;
    }
    if (y > 0.0) { return HALF_PI; }
    if (y < 0.0) { return -HALF_PI; }
    return 0.0;
}

fn normHue(h_in: f64) -> f64 {
    let h: f64 = h_in;
    while (h < 0.0) { h += 360.0; }
    while (h >= 360.0) { h -= 360.0; }
    return h;
}

// --- IEC 61966-2-1 sRGB Gamma ---
pub fn srgbToLinear(c_in: f64) -> f64 {
    const c: f64 = clamp01(c_in);
    if (c <= 0.04045) {
        return c / 12.92;
    }
    return powF64((c + 0.055) / 1.055, 2.4);
}

pub fn linearToSrgb(c_in: f64) -> f64 {
    const c: f64 = clamp01(c_in);
    if (c <= 0.0031308) {
        return 12.92 * c;
    }
    return (1.055 * powF64(c, 1.0 / 2.4)) - 0.055;
}

// --- sRGB <-> HSL ---
pub fn rgbToHsl(c: Rgb) -> Hsl {
    const r: f64 = clamp01(c.r);
    const g: f64 = clamp01(c.g);
    const b: f64 = clamp01(c.b);

    let maxC: f64 = r;
    if (g > maxC) { maxC = g; }
    if (b > maxC) { maxC = b; }

    let minC: f64 = r;
    if (g < minC) { minC = g; }
    if (b < minC) { minC = b; }

    const delta: f64 = maxC - minC;
    const l: f64 = (maxC + minC) * 0.5;
    let h: f64 = 0.0;
    let s: f64 = 0.0;

    if (delta > 0.0) {
        const denom: f64 = 1.0 - absF64((2.0 * l) - 1.0);
        if (denom > 0.0) {
            s = delta / denom;
        }
        if (maxC == r) {
            h = 60.0 * ((g - b) / delta);
        } else if (maxC == g) {
            h = 60.0 * (((b - r) / delta) + 2.0);
        } else {
            h = 60.0 * (((r - g) / delta) + 4.0);
        }
        h = normHue(h);
    }
    return Hsl{ h: h, s: clamp01(s), l: l };
}

pub fn hslToRgb(c: Hsl) -> Rgb {
    const h: f64 = normHue(c.h);
    const s: f64 = clamp01(c.s);
    const l: f64 = clamp01(c.l);

    const chroma: f64 = (1.0 - absF64((2.0 * l) - 1.0)) * s;
    const hPrime: f64 = h / 60.0;
    let mod2: f64 = hPrime;
    while (mod2 >= 2.0) { mod2 -= 2.0; }
    const x: f64 = chroma * (1.0 - absF64(mod2 - 1.0));
    const m: f64 = l - (chroma * 0.5);

    let r1: f64 = 0.0;
    let g1: f64 = 0.0;
    let b1: f64 = 0.0;
    if (hPrime < 1.0) {
        r1 = chroma; g1 = x; b1 = 0.0;
    } else if (hPrime < 2.0) {
        r1 = x; g1 = chroma; b1 = 0.0;
    } else if (hPrime < 3.0) {
        r1 = 0.0; g1 = chroma; b1 = x;
    } else if (hPrime < 4.0) {
        r1 = 0.0; g1 = x; b1 = chroma;
    } else if (hPrime < 5.0) {
        r1 = x; g1 = 0.0; b1 = chroma;
    } else {
        r1 = chroma; g1 = 0.0; b1 = x;
    }
    return Rgb{ r: clamp01(r1 + m), g: clamp01(g1 + m), b: clamp01(b1 + m) };
}

// --- sRGB <-> HSV ---
pub fn rgbToHsv(c: Rgb) -> Hsv {
    const r: f64 = clamp01(c.r);
    const g: f64 = clamp01(c.g);
    const b: f64 = clamp01(c.b);

    let maxC: f64 = r;
    if (g > maxC) { maxC = g; }
    if (b > maxC) { maxC = b; }

    let minC: f64 = r;
    if (g < minC) { minC = g; }
    if (b < minC) { minC = b; }

    const delta: f64 = maxC - minC;
    let h: f64 = 0.0;
    let s: f64 = 0.0;
    if (maxC > 0.0) {
        s = delta / maxC;
    }
    if (delta > 0.0) {
        if (maxC == r) {
            h = 60.0 * ((g - b) / delta);
        } else if (maxC == g) {
            h = 60.0 * (((b - r) / delta) + 2.0);
        } else {
            h = 60.0 * (((r - g) / delta) + 4.0);
        }
        h = normHue(h);
    }
    return Hsv{ h: h, s: s, v: maxC };
}

pub fn hsvToRgb(c: Hsv) -> Rgb {
    const h: f64 = normHue(c.h);
    const s: f64 = clamp01(c.s);
    const v: f64 = clamp01(c.v);

    const chroma: f64 = v * s;
    const hPrime: f64 = h / 60.0;
    let mod2: f64 = hPrime;
    while (mod2 >= 2.0) { mod2 -= 2.0; }
    const x: f64 = chroma * (1.0 - absF64(mod2 - 1.0));
    const m: f64 = v - chroma;

    let r1: f64 = 0.0;
    let g1: f64 = 0.0;
    let b1: f64 = 0.0;
    if (hPrime < 1.0) {
        r1 = chroma; g1 = x; b1 = 0.0;
    } else if (hPrime < 2.0) {
        r1 = x; g1 = chroma; b1 = 0.0;
    } else if (hPrime < 3.0) {
        r1 = 0.0; g1 = chroma; b1 = x;
    } else if (hPrime < 4.0) {
        r1 = 0.0; g1 = x; b1 = chroma;
    } else if (hPrime < 5.0) {
        r1 = x; g1 = 0.0; b1 = chroma;
    } else {
        r1 = chroma; g1 = 0.0; b1 = x;
    }
    return Rgb{ r: clamp01(r1 + m), g: clamp01(g1 + m), b: clamp01(b1 + m) };
}

// --- sRGB <-> OKLab (Björn Ottosson official matrices) ---
pub fn rgbToOklab(c: Rgb) -> Oklab {
    const lr: f64 = srgbToLinear(c.r);
    const lg: f64 = srgbToLinear(c.g);
    const lb: f64 = srgbToLinear(c.b);

    const l: f64 = (0.4122214708 * lr) + (0.5363325363 * lg) + (0.0514459929 * lb);
    const m: f64 = (0.2119034982 * lr) + (0.6806995451 * lg) + (0.1073969566 * lb);
    const s: f64 = (0.0883024619 * lr) + (0.2817188376 * lg) + (0.6299787005 * lb);

    const lC: f64 = cbrtF64(l);
    const mC: f64 = cbrtF64(m);
    const sC: f64 = cbrtF64(s);

    return Oklab{
        l: (0.2104542553 * lC) + (0.7936177850 * mC) - (0.0040720468 * sC),
        a: (1.9779984951 * lC) - (2.4285922050 * mC) + (0.4505937099 * sC),
        b: (0.0259040371 * lC) + (0.7827717662 * mC) - (0.8086757660 * sC),
    };
}

pub fn oklabToRgb(c: Oklab) -> Rgb {
    const lC: f64 = c.l + (0.3963377774 * c.a) + (0.2158037573 * c.b);
    const mC: f64 = c.l - (0.1055613458 * c.a) - (0.0638541728 * c.b);
    const sC: f64 = c.l - (0.0894841775 * c.a) - (1.2914855480 * c.b);

    const l: f64 = lC * lC * lC;
    const m: f64 = mC * mC * mC;
    const s: f64 = sC * sC * sC;

    const lr: f64 = (4.0767416621 * l) - (3.3077115913 * m) + (0.2309699292 * s);
    const lg: f64 = (-1.2684380046 * l) + (2.6097574011 * m) - (0.3413193965 * s);
    const lb: f64 = (-0.0041960863 * l) - (0.7034186147 * m) + (1.7076147010 * s);

    return Rgb{
        r: linearToSrgb(lr),
        g: linearToSrgb(lg),
        b: linearToSrgb(lb),
    };
}

// --- OKLab <-> OKLCH ---
pub fn oklabToOklch(c: Oklab) -> Oklch {
    const chroma: f64 = sqrtF64((c.a * c.a) + (c.b * c.b));
    let h: f64 = 0.0;
    if (chroma > 0.0000000001) {
        h = normHue(atan2F64(c.b, c.a) * (180.0 / PI));
    }
    return Oklch{ l: c.l, c: chroma, h: h };
}

pub fn oklchToOklab(c: Oklch) -> Oklab {
    const rad: f64 = c.h * (PI / 180.0);
    return Oklab{
        l: c.l,
        a: c.c * cosF64(rad),
        b: c.c * sinF64(rad),
    };
}

pub fn oklabDistance(a: Rgb, b: Rgb) -> f64 {
    const la = rgbToOklab(a);
    const lb = rgbToOklab(b);
    const dl: f64 = la.l - lb.l;
    const da: f64 = la.a - lb.a;
    const db: f64 = la.b - lb.b;
    return sqrtF64((dl * dl) + (da * da) + (db * db));
}

pub fn mixOklab(a: Rgb, b: Rgb, t_in: f64) -> Rgb {
    const t: f64 = clamp01(t_in);
    const la = rgbToOklab(a);
    const lb = rgbToOklab(b);
    const mixed = Oklab{
        l: la.l + ((lb.l - la.l) * t),
        a: la.a + ((lb.a - la.a) * t),
        b: la.b + ((lb.b - la.b) * t),
    };
    return oklabToRgb(mixed);
}

export {
    Rgb,
    Hsl,
    Hsv,
    Oklab,
    Oklch,
    srgbToLinear,
    linearToSrgb,
    rgbToHsl,
    hslToRgb,
    rgbToHsv,
    hsvToRgb,
    rgbToOklab,
    oklabToRgb,
    oklabToOklch,
    oklchToOklab,
    oklabDistance,
    mixOklab,
};
