// Source: cp/lib/color/spaces.cp (sRGB, HSL, HSV, OKLab & OKLCH Color Spaces)
// Pure CP color space representations, exact bidirectional conversions (sRGB, HSL, HSV, OKLab, OKLCH),
// perceptual OKLab mixing, and color manipulation utilities.

const std = @import("std");

const HEX_DIGITS: string = "0123456789abcdef";
const DEG_TO_RAD: f64 = std.math.pi / 180.0;
const RAD_TO_DEG: f64 = 180.0 / std.math.pi;

/// Error set for `cap:color` parsing and buffer formatting operations.
pub err ColorError {
    InvalidHex,
    OutOfMemory,
}

fn clampF64(v: f64, lo: f64, hi: f64): f64 {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

fn clampU8FromF64(v: f64): u8 {
    if (v <= 0.0) return 0;
    if (v >= 255.0) return 255;
    return @intFromFloat(@round(v));
}

fn wrapDegrees(deg: f64): f64 {
    if (std.math.isNan(deg) || std.math.isInf(deg)) return 0.0;
    let w = @mod(deg, 360.0);
    if (w < 0.0) w += 360.0;
    if (w >= 360.0) w = 0.0;
    return w;
}

fn hexNibble(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return null;
}

fn parseHexByte(hi: u8, lo: u8): u8 ! ColorError {
    if (h := hexNibble(hi)) {
        if (l := hexNibble(lo)) {
            return (h << 4) | l;
        }
    }
    return ColorError.InvalidHex;
}

fn parseHexNibbleDoubled(ch: u8): u8 ! ColorError {
    if (n := hexNibble(ch)) {
        return (n << 4) | n;
    }
    return ColorError.InvalidHex;
}

fn writeHexByte(buf: []u8, offset: usize, val: u8) {
    buf[offset] = HEX_DIGITS[(val >> 4) & 0x0F];
    buf[offset + 1] = HEX_DIGITS[val & 0x0F];
}

fn hueToRgbChannel(p: f64, q: f64, t_in: f64): f64 {
    let t = t_in;
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 0.5) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

/// Converts an 8-bit sRGB channel `[0, 255]` to linear light `[0.0, 1.0]` per IEC 61966-2-1 / WCAG 2.1.
pub fn srgbToLinear(c: u8): f64 {
    const cf: f64 = @floatFromInt(c);
    const v = cf / 255.0;
    if (v <= 0.04045) return v / 12.92;
    return std.math.pow(f64, (v + 0.055) / 1.055, 2.4);
}

/// Converts a linear light channel `[0.0, 1.0]` to an 8-bit sRGB channel `[0, 255]`.
pub fn linearToSrgb(x: f64): u8 {
    const c = clampF64(x, 0.0, 1.0);
    const encoded = if (c <= 0.0031308)
        (12.92 * c)
    else
        (1.055 * std.math.pow(f64, c, 1.0 / 2.4) - 0.055);
    return clampU8FromF64(encoded * 255.0);
}

/// 8-bit per channel sRGBA color value.
pub struct Rgba {
    r: u8 = 0;
    g: u8 = 0;
    b: u8 = 0;
    a: u8 = 255;

    /// Constructs an opaque `Rgba` color with `a = 255`.
    static fn rgb(r: u8, g: u8, b: u8): Rgba {
        return Rgba { r: r, g: g, b: b, a: 255 };
    }

    /// Constructs an `Rgba` color with explicit alpha.
    static fn rgba(r: u8, g: u8, b: u8, a: u8): Rgba {
        return Rgba { r: r, g: g, b: b, a: a };
    }

    /// Parses `#RGB`, `#RGBA`, `#RRGGBB`, or `#RRGGBBAA` hex color strings.
    static fn parseHex(hex_str: string): Rgba ! ColorError {
        const s = std.mem.trim(u8, hex_str, " \t\r\n");
        if (s.len < 4 || s[0] != '#') {
            return ColorError.InvalidHex;
        }
        const digits = s[1..];
        if (digits.len == 3) {
            const r = parseHexNibbleDoubled(digits[0])?;
            const g = parseHexNibbleDoubled(digits[1])?;
            const b = parseHexNibbleDoubled(digits[2])?;
            return Rgba { r: r, g: g, b: b, a: 255 };
        }
        if (digits.len == 4) {
            const r = parseHexNibbleDoubled(digits[0])?;
            const g = parseHexNibbleDoubled(digits[1])?;
            const b = parseHexNibbleDoubled(digits[2])?;
            const a = parseHexNibbleDoubled(digits[3])?;
            return Rgba { r: r, g: g, b: b, a: a };
        }
        if (digits.len == 6) {
            const r = parseHexByte(digits[0], digits[1])?;
            const g = parseHexByte(digits[2], digits[3])?;
            const b = parseHexByte(digits[4], digits[5])?;
            return Rgba { r: r, g: g, b: b, a: 255 };
        }
        if (digits.len == 8) {
            const r = parseHexByte(digits[0], digits[1])?;
            const g = parseHexByte(digits[2], digits[3])?;
            const b = parseHexByte(digits[4], digits[5])?;
            const a = parseHexByte(digits[6], digits[7])?;
            return Rgba { r: r, g: g, b: b, a: a };
        }
        return ColorError.InvalidHex;
    }

    /// Formats `#rrggbb` into `buf` (requires `buf.len >= 7`).
    fn toHex(buf: []u8): string ! ColorError {
        if (buf.len < 7) return ColorError.OutOfMemory;
        buf[0] = '#';
        writeHexByte(buf, 1, _.r);
        writeHexByte(buf, 3, _.g);
        writeHexByte(buf, 5, _.b);
        return buf[0..7];
    }

    /// Formats `#rrggbbaa` into `buf` (requires `buf.len >= 9`).
    fn toHexAlpha(buf: []u8): string ! ColorError {
        if (buf.len < 9) return ColorError.OutOfMemory;
        buf[0] = '#';
        writeHexByte(buf, 1, _.r);
        writeHexByte(buf, 3, _.g);
        writeHexByte(buf, 5, _.b);
        writeHexByte(buf, 7, _.a);
        return buf[0..9];
    }

    /// Exact component-wise equality check.
    fn eql(other: Rgba): bool {
        return _.r == other.r && _.g == other.g && _.b == other.b && _.a == other.a;
    }

    /// Returns a copy of this color with alpha replaced by `alpha`.
    fn withAlpha(alpha: u8): Rgba {
        return Rgba { r: _.r, g: _.g, b: _.b, a: alpha };
    }

    /// Converts this color to HSL.
    fn toHsl(): Hsl {
        return rgbToHsl(_);
    }

    /// Converts this color to HSV.
    fn toHsv(): Hsv {
        return rgbToHsv(_);
    }

    /// Converts this color to perceptual OKLab.
    fn toOklab(): Oklab {
        return rgbToOklab(_);
    }

    /// Converts this color to perceptual OKLCH.
    fn toOklch(): Oklch {
        return oklabToOklch(rgbToOklab(_));
    }
}

/// HSL cylindrical color representation (`h` in `[0, 360)`, `s` and `l` in `[0.0, 1.0]`).
pub struct Hsl {
    h: f64 = 0.0;
    s: f64 = 0.0;
    l: f64 = 0.0;
    a: u8 = 255;

    /// Constructs an `Hsl` color value with normalized hue and clamped saturation/lightness.
    static fn init(h: f64, s: f64, l: f64, a: u8 = 255): Hsl {
        return Hsl {
            h: wrapDegrees(h),
            s: clampF64(s, 0.0, 1.0),
            l: clampF64(l, 0.0, 1.0),
            a: a,
        };
    }

    /// Converts this HSL value back to sRGBA.
    fn toRgba(): Rgba {
        return hslToRgb(_);
    }

    /// Approximate equality within tolerance `eps`.
    fn approxEql(other: Hsl, eps: f64 = 0.001): bool {
        const dh_raw = @abs(_.h - other.h);
        const dh = if (dh_raw > 180.0) (360.0 - dh_raw) else dh_raw;
        return dh <= eps * 360.0 && @abs(_.s - other.s) <= eps && @abs(_.l - other.l) <= eps && _.a == other.a;
    }
}

/// HSV / HSB cylindrical color representation (`h` in `[0, 360)`, `s` and `v` in `[0.0, 1.0]`).
pub struct Hsv {
    h: f64 = 0.0;
    s: f64 = 0.0;
    v: f64 = 0.0;
    a: u8 = 255;

    /// Constructs an `Hsv` color value with normalized hue and clamped saturation/value.
    static fn init(h: f64, s: f64, v: f64, a: u8 = 255): Hsv {
        return Hsv {
            h: wrapDegrees(h),
            s: clampF64(s, 0.0, 1.0),
            v: clampF64(v, 0.0, 1.0),
            a: a,
        };
    }

    /// Converts this HSV value back to sRGBA.
    fn toRgba(): Rgba {
        return hsvToRgb(_);
    }

    /// Approximate equality within tolerance `eps`.
    fn approxEql(other: Hsv, eps: f64 = 0.001): bool {
        const dh_raw = @abs(_.h - other.h);
        const dh = if (dh_raw > 180.0) (360.0 - dh_raw) else dh_raw;
        return dh <= eps * 360.0 && @abs(_.s - other.s) <= eps && @abs(_.v - other.v) <= eps && _.a == other.a;
    }
}

/// Björn Ottosson perceptual OKLab color space (`l` in `[0.0, 1.0]`, opponent axes `a`, `b`).
pub struct Oklab {
    l: f64 = 0.0;
    a: f64 = 0.0;
    b: f64 = 0.0;
    alpha: u8 = 255;

    /// Constructs an `Oklab` color value.
    static fn init(l: f64, a: f64, b: f64, alpha: u8 = 255): Oklab {
        return Oklab { l: l, a: a, b: b, alpha: alpha };
    }

    /// Converts this OKLab color back to sRGBA.
    fn toRgba(): Rgba {
        return oklabToRgb(_);
    }

    /// Converts this OKLab color to cylindrical OKLCH.
    fn toOklch(): Oklch {
        return oklabToOklch(_);
    }

    /// Approximate component-wise equality within tolerance `eps`.
    fn approxEql(other: Oklab, eps: f64 = 0.0001): bool {
        return @abs(_.l - other.l) <= eps &&
            @abs(_.a - other.a) <= eps &&
            @abs(_.b - other.b) <= eps &&
            _.alpha == other.alpha;
    }
}

/// Cylindrical perceptual OKLCH color space (`l` lightness, `c` chroma, `h` hue in `[0, 360)`).
pub struct Oklch {
    l: f64 = 0.0;
    c: f64 = 0.0;
    h: f64 = 0.0;
    alpha: u8 = 255;

    /// Constructs an `Oklch` color value.
    static fn init(l: f64, c: f64, h: f64, alpha: u8 = 255): Oklch {
        return Oklch {
            l: l,
            c: if (c < 0.0) 0.0 else c,
            h: wrapDegrees(h),
            alpha: alpha,
        };
    }

    /// Converts this OKLCH color to Cartesian OKLab.
    fn toOklab(): Oklab {
        return oklchToOklab(_);
    }

    /// Converts this OKLCH color to sRGBA.
    fn toRgba(): Rgba {
        return oklabToRgb(oklchToOklab(_));
    }

    /// Approximate component-wise equality within tolerance `eps`.
    fn approxEql(other: Oklch, eps: f64 = 0.0001): bool {
        const dh_raw = @abs(_.h - other.h);
        const dh = if (dh_raw > 180.0) (360.0 - dh_raw) else dh_raw;
        return @abs(_.l - other.l) <= eps &&
            @abs(_.c - other.c) <= eps &&
            dh <= eps * 360.0 &&
            _.alpha == other.alpha;
    }
}

/// Converts an `Rgba` color to `Hsl`.
pub fn rgbToHsl(c: Rgba): Hsl {
    const rf: f64 = @as(f64, @floatFromInt(c.r)) / 255.0;
    const gf: f64 = @as(f64, @floatFromInt(c.g)) / 255.0;
    const bf: f64 = @as(f64, @floatFromInt(c.b)) / 255.0;

    const max_c = @max(rf, @max(gf, bf));
    const min_c = @min(rf, @min(gf, bf));
    const l = (max_c + min_c) * 0.5;

    if (max_c == min_c) {
        return Hsl { h: 0.0, s: 0.0, l: l, a: c.a };
    }

    const d = max_c - min_c;
    const s = if (l > 0.5) (d / (2.0 - max_c - min_c)) else (d / (max_c + min_c));

    let h: f64 = 0.0;
    if (max_c == rf) {
        const offset: f64 = if (gf < bf) 6.0 else 0.0;
        h = (gf - bf) / d + offset;
    } else if (max_c == gf) {
        h = (bf - rf) / d + 2.0;
    } else {
        h = (rf - gf) / d + 4.0;
    }
    return Hsl { h: wrapDegrees(h * 60.0), s: clampF64(s, 0.0, 1.0), l: clampF64(l, 0.0, 1.0), a: c.a };
}

/// Converts an `Hsl` color back to `Rgba`.
pub fn hslToRgb(hsl: Hsl): Rgba {
    const h_norm = wrapDegrees(hsl.h) / 360.0;
    const s = clampF64(hsl.s, 0.0, 1.0);
    const l = clampF64(hsl.l, 0.0, 1.0);

    if (s == 0.0) {
        const gray = clampU8FromF64(l * 255.0);
        return Rgba { r: gray, g: gray, b: gray, a: hsl.a };
    }

    const q = if (l < 0.5) (l * (1.0 + s)) else (l + s - l * s);
    const p = 2.0 * l - q;
    const r = clampU8FromF64(hueToRgbChannel(p, q, h_norm + 1.0 / 3.0) * 255.0);
    const g = clampU8FromF64(hueToRgbChannel(p, q, h_norm) * 255.0);
    const b = clampU8FromF64(hueToRgbChannel(p, q, h_norm - 1.0 / 3.0) * 255.0);
    return Rgba { r: r, g: g, b: b, a: hsl.a };
}

/// Converts an `Rgba` color to `Hsv`.
pub fn rgbToHsv(c: Rgba): Hsv {
    const rf: f64 = @as(f64, @floatFromInt(c.r)) / 255.0;
    const gf: f64 = @as(f64, @floatFromInt(c.g)) / 255.0;
    const bf: f64 = @as(f64, @floatFromInt(c.b)) / 255.0;

    const max_c = @max(rf, @max(gf, bf));
    const min_c = @min(rf, @min(gf, bf));
    const d = max_c - min_c;
    const v = max_c;
    const s = if (max_c == 0.0) 0.0 else (d / max_c);

    if (d == 0.0) {
        return Hsv { h: 0.0, s: 0.0, v: v, a: c.a };
    }

    let h: f64 = 0.0;
    if (max_c == rf) {
        const offset: f64 = if (gf < bf) 6.0 else 0.0;
        h = (gf - bf) / d + offset;
    } else if (max_c == gf) {
        h = (bf - rf) / d + 2.0;
    } else {
        h = (rf - gf) / d + 4.0;
    }
    return Hsv { h: wrapDegrees(h * 60.0), s: clampF64(s, 0.0, 1.0), v: clampF64(v, 0.0, 1.0), a: c.a };
}

/// Converts an `Hsv` color back to `Rgba`.
pub fn hsvToRgb(hsv: Hsv): Rgba {
    const h = wrapDegrees(hsv.h);
    const s = clampF64(hsv.s, 0.0, 1.0);
    const v = clampF64(hsv.v, 0.0, 1.0);

    if (s == 0.0) {
        const gray = clampU8FromF64(v * 255.0);
        return Rgba { r: gray, g: gray, b: gray, a: hsv.a };
    }

    const sector_f = h / 60.0;
    const sector: i32 = @intFromFloat(@floor(sector_f));
    const f = sector_f - @as(f64, @floatFromInt(sector));
    const p = v * (1.0 - s);
    const q = v * (1.0 - s * f);
    const t = v * (1.0 - s * (1.0 - f));

    let rf: f64 = 0.0;
    let gf: f64 = 0.0;
    let bf: f64 = 0.0;

    switch (sector) {
        0 => { rf = v; gf = t; bf = p; },
        1 => { rf = q; gf = v; bf = p; },
        2 => { rf = p; gf = v; bf = t; },
        3 => { rf = p; gf = q; bf = v; },
        4 => { rf = t; gf = p; bf = v; },
        else => { rf = v; gf = p; bf = q; },
    }

    return Rgba {
        r: clampU8FromF64(rf * 255.0),
        g: clampU8FromF64(gf * 255.0),
        b: clampU8FromF64(bf * 255.0),
        a: hsv.a,
    };
}

/// Converts an `Rgba` color to Björn Ottosson's perceptual `Oklab` color space.
pub fn rgbToOklab(c: Rgba): Oklab {
    const r = srgbToLinear(c.r);
    const g = srgbToLinear(c.g);
    const b = srgbToLinear(c.b);

    const l = 0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b;
    const m = 0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b;
    const s = 0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b;

    const l_ = std.math.cbrt(l);
    const m_ = std.math.cbrt(m);
    const s_ = std.math.cbrt(s);

    return Oklab {
        l: 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
        a: 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
        b: 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_,
        alpha: c.a,
    };
}

/// Converts an `Oklab` color back to `Rgba` using Björn Ottosson's inverse LMS matrix and sRGB transfer.
pub fn oklabToRgb(lab: Oklab): Rgba {
    const l_ = lab.l + 0.3963377774 * lab.a + 0.2158037573 * lab.b;
    const m_ = lab.l - 0.1055613458 * lab.a - 0.0638541728 * lab.b;
    const s_ = lab.l - 0.0894841775 * lab.a - 1.2914855480 * lab.b;

    const l = l_ * l_ * l_;
    const m = m_ * m_ * m_;
    const s = s_ * s_ * s_;

    const lin_r = 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    const lin_g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    const lin_b = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;

    return Rgba {
        r: linearToSrgb(lin_r),
        g: linearToSrgb(lin_g),
        b: linearToSrgb(lin_b),
        a: lab.alpha,
    };
}

/// Converts Cartesian `Oklab` to cylindrical `Oklch`.
pub fn oklabToOklch(lab: Oklab): Oklch {
    const c = @sqrt(lab.a * lab.a + lab.b * lab.b);
    const h = if (c < 0.0000001) 0.0 else wrapDegrees(std.math.atan2(lab.b, lab.a) * RAD_TO_DEG);
    return Oklch { l: lab.l, c: c, h: h, alpha: lab.alpha };
}

/// Converts cylindrical `Oklch` to Cartesian `Oklab`.
pub fn oklchToOklab(lch: Oklch): Oklab {
    const rad = lch.h * DEG_TO_RAD;
    const c = if (lch.c < 0.0) 0.0 else lch.c;
    return Oklab {
        l: lch.l,
        a: c * @cos(rad),
        b: c * @sin(rad),
        alpha: lch.alpha,
    };
}

/// Perceptually uniform linear interpolation between `a` and `b` in OKLab space (`t` clamped to `[0.0, 1.0]`).
pub fn mixOklab(a: Rgba, b: Rgba, t: f64): Rgba {
    const w = clampF64(t, 0.0, 1.0);
    const inv = 1.0 - w;
    const la = rgbToOklab(a);
    const lb = rgbToOklab(b);
    const a_alpha: f64 = @floatFromInt(a.a);
    const b_alpha: f64 = @floatFromInt(b.a);

    const mixed = Oklab {
        l: la.l * inv + lb.l * w,
        a: la.a * inv + lb.a * w,
        b: la.b * inv + lb.b * w,
        alpha: clampU8FromF64(a_alpha * inv + b_alpha * w),
    };
    return oklabToRgb(mixed);
}

/// Rotates the hue of `c` by `deg` degrees in HSL space, preserving alpha.
pub fn rotateHue(c: Rgba, deg: f64): Rgba {
    const hsl = rgbToHsl(c);
    return hslToRgb(Hsl {
        h: wrapDegrees(hsl.h + deg),
        s: hsl.s,
        l: hsl.l,
        a: c.a,
    });
}

/// Increases HSL lightness of `c` by `amount` (`[0.0, 1.0]`), clamped to `1.0`.
pub fn lighten(c: Rgba, amount: f64): Rgba {
    const hsl = rgbToHsl(c);
    return hslToRgb(Hsl {
        h: hsl.h,
        s: hsl.s,
        l: clampF64(hsl.l + amount, 0.0, 1.0),
        a: c.a,
    });
}

/// Decreases HSL lightness of `c` by `amount` (`[0.0, 1.0]`), clamped to `0.0`.
pub fn darken(c: Rgba, amount: f64): Rgba {
    const hsl = rgbToHsl(c);
    return hslToRgb(Hsl {
        h: hsl.h,
        s: hsl.s,
        l: clampF64(hsl.l - amount, 0.0, 1.0),
        a: c.a,
    });
}

/// Adjusts HSL saturation of `c` by `amount` (clamped to `[0.0, 1.0]`).
pub fn saturate(c: Rgba, amount: f64): Rgba {
    const hsl = rgbToHsl(c);
    return hslToRgb(Hsl {
        h: hsl.h,
        s: clampF64(hsl.s + amount, 0.0, 1.0),
        l: hsl.l,
        a: c.a,
    });
}

/// Inverts the RGB channels (`255 - channel`) while preserving alpha.
pub fn invert(c: Rgba): Rgba {
    return Rgba {
        r: 255 - c.r,
        g: 255 - c.g,
        b: 255 - c.b,
        a: c.a,
    };
}
