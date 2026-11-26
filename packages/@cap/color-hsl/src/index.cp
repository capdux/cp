// packages/@cap/color-hsl/src/index.cp
// Zero-allocation RGB / HSL / HSV / CMYK color space conversion & manipulation library.
// MIT clean-room pure CP implementation.

/// Normalized RGB color with channels in `[0.0, 1.0]`.
pub struct Rgb {
    pub r: f64,
    pub g: f64,
    pub b: f64,
}

/// HSL color with `h` in `[0.0, 360.0)` degrees and `s`, `l` in `[0.0, 1.0]`.
pub struct Hsl {
    pub h: f64,
    pub s: f64,
    pub l: f64,
}

/// HSV color with `h` in `[0.0, 360.0)` degrees and `s`, `v` in `[0.0, 1.0]`.
pub struct Hsv {
    pub h: f64,
    pub s: f64,
    pub v: f64,
}

/// CMYK color with `c`, `m`, `y`, `k` in `[0.0, 1.0]`.
pub struct Cmyk {
    pub c: f64,
    pub m: f64,
    pub y: f64,
    pub k: f64,
}

fn chAbs(x: f64): f64 {
    if (x < (0.0 as f64)) { return -x; }
    return x;
}

fn chClamp01(x: f64): f64 {
    if (x < (0.0 as f64)) { return 0.0 as f64; }
    if (x > (1.0 as f64)) { return 1.0 as f64; }
    return x;
}

fn chNormHue(h: f64): f64 {
    let cur: f64 = h;
    if (cur < (0.0 as f64)) {
        const wraps: u64 = (((-cur) / (360.0 as f64)) as u64) + (1 as u64);
        cur = cur + ((wraps as f64) * (360.0 as f64));
    }
    if (cur >= (360.0 as f64)) {
        const wraps: u64 = (cur / (360.0 as f64)) as u64;
        cur = cur - ((wraps as f64) * (360.0 as f64));
    }
    if (cur >= (360.0 as f64) || cur < (0.0 as f64)) {
        return 0.0 as f64;
    }
    return cur;
}

fn chSectorRgb(hn: f64, chroma: f64, m: f64): Rgb {
    const hp: f64 = hn / (60.0 as f64);
    const pairs: u64 = (hp / (2.0 as f64)) as u64;
    const hMod2: f64 = hp - ((pairs as f64) * (2.0 as f64));
    const x: f64 = chroma * ((1.0 as f64) - chAbs(hMod2 - (1.0 as f64)));

    let r1: f64 = 0.0;
    let g1: f64 = 0.0;
    let b1: f64 = 0.0;
    if (hp < (1.0 as f64)) {
        r1 = chroma; g1 = x; b1 = 0.0 as f64;
    } else if (hp < (2.0 as f64)) {
        r1 = x; g1 = chroma; b1 = 0.0 as f64;
    } else if (hp < (3.0 as f64)) {
        r1 = 0.0 as f64; g1 = chroma; b1 = x;
    } else if (hp < (4.0 as f64)) {
        r1 = 0.0 as f64; g1 = x; b1 = chroma;
    } else if (hp < (5.0 as f64)) {
        r1 = x; g1 = 0.0 as f64; b1 = chroma;
    } else {
        r1 = chroma; g1 = 0.0 as f64; b1 = x;
    }

    return Rgb{
        r: chClamp01(r1 + m),
        g: chClamp01(g1 + m),
        b: chClamp01(b1 + m),
    };
}

/// Convert 8-bit sRGB channels `(0..255)` to normalized `Rgb` (`[0.0, 1.0]`).
pub fn rgbFromU8(r: u8, g: u8, b: u8): Rgb {
    return Rgb{
        r: (r as f64) / (255.0 as f64),
        g: (g as f64) / (255.0 as f64),
        b: (b as f64) / (255.0 as f64),
    };
}

/// Convert normalized `Rgb` to `Hsl`.
pub fn rgbToHsl(c: Rgb): Hsl {
    const r: f64 = chClamp01(c.r);
    const g: f64 = chClamp01(c.g);
    const b: f64 = chClamp01(c.b);

    let maxC: f64 = r;
    if (g > maxC) { maxC = g; }
    if (b > maxC) { maxC = b; }

    let minC: f64 = r;
    if (g < minC) { minC = g; }
    if (b < minC) { minC = b; }

    const d: f64 = maxC - minC;
    const l: f64 = (maxC + minC) * (0.5 as f64);
    let h: f64 = 0.0;
    let s: f64 = 0.0;

    if (d > (0.000000001 as f64)) {
        const denom: f64 = (1.0 as f64) - chAbs(((2.0 as f64) * l) - (1.0 as f64));
        if (denom > (0.000000001 as f64)) {
            s = d / denom;
        }
        if (maxC == r) {
            h = (60.0 as f64) * ((g - b) / d);
        } else if (maxC == g) {
            h = (60.0 as f64) * (((b - r) / d) + (2.0 as f64));
        } else {
            h = (60.0 as f64) * (((r - g) / d) + (4.0 as f64));
        }
    }

    return Hsl{
        h: chNormHue(h),
        s: chClamp01(s),
        l: chClamp01(l),
    };
}

/// Convert `Hsl` to normalized `Rgb`.
pub fn hslToRgb(c: Hsl): Rgb {
    const hn: f64 = chNormHue(c.h);
    const sn: f64 = chClamp01(c.s);
    const ln: f64 = chClamp01(c.l);
    const chroma: f64 = ((1.0 as f64) - chAbs(((2.0 as f64) * ln) - (1.0 as f64))) * sn;
    const m: f64 = ln - (chroma * (0.5 as f64));
    return chSectorRgb(hn, chroma, m);
}

/// Convert normalized `Rgb` to `Hsv`.
pub fn rgbToHsv(c: Rgb): Hsv {
    const r: f64 = chClamp01(c.r);
    const g: f64 = chClamp01(c.g);
    const b: f64 = chClamp01(c.b);

    let maxC: f64 = r;
    if (g > maxC) { maxC = g; }
    if (b > maxC) { maxC = b; }

    let minC: f64 = r;
    if (g < minC) { minC = g; }
    if (b < minC) { minC = b; }

    const d: f64 = maxC - minC;
    const v: f64 = maxC;
    let s: f64 = 0.0;
    let h: f64 = 0.0;

    if (maxC > (0.000000001 as f64)) {
        s = d / maxC;
    }
    if (d > (0.000000001 as f64)) {
        if (maxC == r) {
            h = (60.0 as f64) * ((g - b) / d);
        } else if (maxC == g) {
            h = (60.0 as f64) * (((b - r) / d) + (2.0 as f64));
        } else {
            h = (60.0 as f64) * (((r - g) / d) + (4.0 as f64));
        }
    }

    return Hsv{
        h: chNormHue(h),
        s: chClamp01(s),
        v: chClamp01(v),
    };
}

/// Convert `Hsv` to normalized `Rgb`.
pub fn hsvToRgb(c: Hsv): Rgb {
    const hn: f64 = chNormHue(c.h);
    const sn: f64 = chClamp01(c.s);
    const vn: f64 = chClamp01(c.v);
    const chroma: f64 = vn * sn;
    const m: f64 = vn - chroma;
    return chSectorRgb(hn, chroma, m);
}

/// Convert normalized `Rgb` to `Cmyk` (pure black returns `Cmyk{ 0, 0, 0, 1 }`).
pub fn rgbToCmyk(c: Rgb): Cmyk {
    const r: f64 = chClamp01(c.r);
    const g: f64 = chClamp01(c.g);
    const b: f64 = chClamp01(c.b);

    let maxC: f64 = r;
    if (g > maxC) { maxC = g; }
    if (b > maxC) { maxC = b; }

    const k: f64 = (1.0 as f64) - maxC;
    if (maxC <= (0.000000001 as f64)) {
        return Cmyk{
            c: 0.0 as f64,
            m: 0.0 as f64,
            y: 0.0 as f64,
            k: 1.0 as f64,
        };
    }

    return Cmyk{
        c: chClamp01(((1.0 as f64) - r - k) / maxC),
        m: chClamp01(((1.0 as f64) - g - k) / maxC),
        y: chClamp01(((1.0 as f64) - b - k) / maxC),
        k: chClamp01(k),
    };
}

/// Convert `Cmyk` to normalized `Rgb`.
pub fn cmykToRgb(c: Cmyk): Rgb {
    const invK: f64 = (1.0 as f64) - chClamp01(c.k);
    return Rgb{
        r: chClamp01(((1.0 as f64) - chClamp01(c.c)) * invK),
        g: chClamp01(((1.0 as f64) - chClamp01(c.m)) * invK),
        b: chClamp01(((1.0 as f64) - chClamp01(c.y)) * invK),
    };
}

/// Rotate the hue of `c` by `degrees`, wrapping modulo 360 into `[0.0, 360.0)`.
pub fn rotateHue(c: Hsl, degrees: f64): Hsl {
    return Hsl{
        h: chNormHue(c.h + degrees),
        s: c.s,
        l: c.l,
    };
}

/// Increase lightness `c.l` by `amount`, clamped to `[0.0, 1.0]`.
pub fn lighten(c: Hsl, amount: f64): Hsl {
    return Hsl{
        h: c.h,
        s: c.s,
        l: chClamp01(c.l + amount),
    };
}

/// Decrease lightness `c.l` by `amount`, clamped to `[0.0, 1.0]`.
pub fn darken(c: Hsl, amount: f64): Hsl {
    return Hsl{
        h: c.h,
        s: c.s,
        l: chClamp01(c.l - amount),
    };
}

/// Linearly mix two `Rgb` colors by factor `t` clamped to `[0.0, 1.0]`.
pub fn mixRgb(a: Rgb, b: Rgb, t: f64): Rgb {
    const tt: f64 = chClamp01(t);
    return Rgb{
        r: a.r + (b.r - a.r) * tt,
        g: a.g + (b.g - a.g) * tt,
        b: a.b + (b.b - a.b) * tt,
    };
}

export {
    Rgb,
    Hsl,
    Hsv,
    Cmyk,
    rgbToHsl,
    hslToRgb,
    rgbToHsv,
    hsvToRgb,
    rgbToCmyk,
    cmykToRgb,
    rgbFromU8,
    rotateHue,
    lighten,
    darken,
    mixRgb,
};
