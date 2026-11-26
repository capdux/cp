// @cap/color-convert — Zero-allocation color space conversion engine for CP
// Clean-room MIT implementation aligned with npm `color-convert` v2.
// Reuses `@cap/color-name` directly for CSS keyword conversions with zero table duplication.

import {
    RgbColor,
    lookupColor,
    reverseLookupColor,
} from "../../color-name/src/index.cp";

/// 8-bit sRGB color with channels in `0..255`.
pub struct Rgb {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

/// HSV color with `h` in `[0.0, 360.0)` and `s`, `v` in `[0.0, 100.0]`.
pub struct Hsv {
    pub h: f64,
    pub s: f64,
    pub v: f64,
}

/// HWB color with `h` in `[0.0, 360.0)` and `w`, `b` in `[0.0, 100.0]`.
pub struct Hwb {
    pub h: f64,
    pub w: f64,
    pub b: f64,
}

/// HSL color with `h` in `[0.0, 360.0)` and `s`, `l` in `[0.0, 100.0]`.
pub struct Hsl {
    pub h: f64,
    pub s: f64,
    pub l: f64,
}

/// CMYK color with `c`, `m`, `y`, `k` in `[0.0, 100.0]`.
pub struct Cmyk {
    pub c: f64,
    pub m: f64,
    pub y: f64,
    pub k: f64,
}

const CC_HEX_UPPER: string = "0123456789ABCDEF";

fn ccAbs(x: f64): f64 {
    if (x < (0.0 as f64)) {
        return -x;
    }
    return x;
}

fn ccClamp01(x: f64): f64 {
    if (x < (0.0 as f64)) {
        return 0.0 as f64;
    }
    if (x > (1.0 as f64)) {
        return 1.0 as f64;
    }
    return x;
}

fn ccNormHue(h: f64): f64 {
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

fn ccRoundU8(x: f64): u8 {
    if (x <= (0.0 as f64)) {
        return 0;
    }
    if (x >= (255.0 as f64)) {
        return 255;
    }
    return (x + (0.5 as f64)) as u8;
}

fn ccHexVal(ch: u8): i16 {
    if (ch >= 48 && ch <= 57) {
        return (ch - 48) as i16;
    }
    if (ch >= 65 && ch <= 70) {
        return ((ch - 65) + 10) as i16;
    }
    if (ch >= 97 && ch <= 102) {
        return ((ch - 97) + 10) as i16;
    }
    return -1;
}

fn ccAnsiStd16(idx: u8): Rgb {
    if (idx == 0) { return Rgb { r: 0, g: 0, b: 0 }; }
    if (idx == 1) { return Rgb { r: 128, g: 0, b: 0 }; }
    if (idx == 2) { return Rgb { r: 0, g: 128, b: 0 }; }
    if (idx == 3) { return Rgb { r: 128, g: 128, b: 0 }; }
    if (idx == 4) { return Rgb { r: 0, g: 0, b: 128 }; }
    if (idx == 5) { return Rgb { r: 128, g: 0, b: 128 }; }
    if (idx == 6) { return Rgb { r: 0, g: 128, b: 128 }; }
    if (idx == 7) { return Rgb { r: 192, g: 192, b: 192 }; }
    if (idx == 8) { return Rgb { r: 128, g: 128, b: 128 }; }
    if (idx == 9) { return Rgb { r: 255, g: 0, b: 0 }; }
    if (idx == 10) { return Rgb { r: 0, g: 255, b: 0 }; }
    if (idx == 11) { return Rgb { r: 255, g: 255, b: 0 }; }
    if (idx == 12) { return Rgb { r: 0, g: 0, b: 255 }; }
    if (idx == 13) { return Rgb { r: 255, g: 0, b: 255 }; }
    if (idx == 14) { return Rgb { r: 0, g: 255, b: 255 }; }
    return Rgb { r: 255, g: 255, b: 255 };
}

/// Convert 8-bit RGB `(0..255)` to `Hsv` (`h` in `[0, 360)`, `s`/`v` in `[0, 100]`).
pub fn rgbToHsv(r: u8, g: u8, b: u8): Hsv {
    const rf: f64 = (r as f64) / (255.0 as f64);
    const gf: f64 = (g as f64) / (255.0 as f64);
    const bf: f64 = (b as f64) / (255.0 as f64);

    let maxC: f64 = rf;
    if (gf > maxC) { maxC = gf; }
    if (bf > maxC) { maxC = bf; }

    let minC: f64 = rf;
    if (gf < minC) { minC = gf; }
    if (bf < minC) { minC = bf; }

    const diff: f64 = maxC - minC;
    let h: f64 = 0.0;
    let s: f64 = 0.0;

    if (maxC > (0.000000001 as f64)) {
        s = diff / maxC;
    }
    if (diff > (0.000000001 as f64)) {
        if (maxC == rf) {
            h = (60.0 as f64) * ((gf - bf) / diff);
        } else if (maxC == gf) {
            h = (60.0 as f64) * (((bf - rf) / diff) + (2.0 as f64));
        } else {
            h = (60.0 as f64) * (((rf - gf) / diff) + (4.0 as f64));
        }
    }

    return Hsv {
        h: ccNormHue(h),
        s: ccClamp01(s) * (100.0 as f64),
        v: ccClamp01(maxC) * (100.0 as f64),
    };
}

/// Convert `Hsv` (`h` in `[0, 360)`, `s`/`v` in `[0, 100]`) to 8-bit `Rgb`.
pub fn hsvToRgb(h: f64, s: f64, v: f64): Rgb {
    const hn: f64 = ccNormHue(h) / (60.0 as f64);
    const sn: f64 = ccClamp01(s / (100.0 as f64));
    const vn: f64 = ccClamp01(v / (100.0 as f64));

    const hi: u32 = (hn as u32) % (6 as u32);
    const f: f64 = hn - ((hn as u32) as f64);

    const v255: f64 = vn * (255.0 as f64);
    const p: f64 = v255 * ((1.0 as f64) - sn);
    const q: f64 = v255 * ((1.0 as f64) - (sn * f));
    const t: f64 = v255 * ((1.0 as f64) - (sn * ((1.0 as f64) - f)));

    if (hi == 0) {
        return Rgb { r: ccRoundU8(v255), g: ccRoundU8(t), b: ccRoundU8(p) };
    }
    if (hi == 1) {
        return Rgb { r: ccRoundU8(q), g: ccRoundU8(v255), b: ccRoundU8(p) };
    }
    if (hi == 2) {
        return Rgb { r: ccRoundU8(p), g: ccRoundU8(v255), b: ccRoundU8(t) };
    }
    if (hi == 3) {
        return Rgb { r: ccRoundU8(p), g: ccRoundU8(q), b: ccRoundU8(v255) };
    }
    if (hi == 4) {
        return Rgb { r: ccRoundU8(t), g: ccRoundU8(p), b: ccRoundU8(v255) };
    }
    return Rgb { r: ccRoundU8(v255), g: ccRoundU8(p), b: ccRoundU8(q) };
}

/// Convert 8-bit RGB `(0..255)` to `Hsl` (`h` in `[0, 360)`, `s`/`l` in `[0, 100]`).
pub fn rgbToHsl(r: u8, g: u8, b: u8): Hsl {
    const rf: f64 = (r as f64) / (255.0 as f64);
    const gf: f64 = (g as f64) / (255.0 as f64);
    const bf: f64 = (b as f64) / (255.0 as f64);

    let maxC: f64 = rf;
    if (gf > maxC) { maxC = gf; }
    if (bf > maxC) { maxC = bf; }

    let minC: f64 = rf;
    if (gf < minC) { minC = gf; }
    if (bf < minC) { minC = bf; }

    const diff: f64 = maxC - minC;
    const l: f64 = (maxC + minC) * (0.5 as f64);
    let h: f64 = 0.0;
    let s: f64 = 0.0;

    if (diff > (0.000000001 as f64)) {
        const denom: f64 = (1.0 as f64) - ccAbs(((2.0 as f64) * l) - (1.0 as f64));
        if (denom > (0.000000001 as f64)) {
            s = diff / denom;
        }
        if (maxC == rf) {
            h = (60.0 as f64) * ((gf - bf) / diff);
        } else if (maxC == gf) {
            h = (60.0 as f64) * (((bf - rf) / diff) + (2.0 as f64));
        } else {
            h = (60.0 as f64) * (((rf - gf) / diff) + (4.0 as f64));
        }
    }

    return Hsl {
        h: ccNormHue(h),
        s: ccClamp01(s) * (100.0 as f64),
        l: ccClamp01(l) * (100.0 as f64),
    };
}

/// Convert `Hsl` (`h` in `[0, 360)`, `s`/`l` in `[0, 100]`) to 8-bit `Rgb`.
pub fn hslToRgb(h: f64, s: f64, l: f64): Rgb {
    const hn: f64 = ccNormHue(h);
    const sn: f64 = ccClamp01(s / (100.0 as f64));
    const ln: f64 = ccClamp01(l / (100.0 as f64));

    const chroma: f64 = ((1.0 as f64) - ccAbs(((2.0 as f64) * ln) - (1.0 as f64))) * sn;
    const hp: f64 = hn / (60.0 as f64);
    const pairs: u64 = (hp / (2.0 as f64)) as u64;
    const hMod2: f64 = hp - ((pairs as f64) * (2.0 as f64));
    const x: f64 = chroma * ((1.0 as f64) - ccAbs(hMod2 - (1.0 as f64)));
    const m: f64 = ln - (chroma * (0.5 as f64));

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

    return Rgb {
        r: ccRoundU8((r1 + m) * (255.0 as f64)),
        g: ccRoundU8((g1 + m) * (255.0 as f64)),
        b: ccRoundU8((b1 + m) * (255.0 as f64)),
    };
}

/// Convert 8-bit RGB `(0..255)` to `Hwb` (`h` in `[0, 360)`, `w`/`b` in `[0, 100]`).
pub fn rgbToHwb(r: u8, g: u8, b: u8): Hwb {
    const hsv: Hsv = rgbToHsv(r, g, b);

    let minU: u8 = r;
    if (g < minU) { minU = g; }
    if (b < minU) { minU = b; }

    let maxU: u8 = r;
    if (g > maxU) { maxU = g; }
    if (b > maxU) { maxU = b; }

    const w: f64 = ((minU as f64) / (255.0 as f64)) * (100.0 as f64);
    const bl: f64 = ((1.0 as f64) - ((maxU as f64) / (255.0 as f64))) * (100.0 as f64);

    return Hwb {
        h: hsv.h,
        w: w,
        b: bl,
    };
}

/// Convert `Hwb` (`h` in `[0, 360)`, `w`/`b` in `[0, 100]`) to 8-bit `Rgb`.
pub fn hwbToRgb(h: f64, w: f64, b: f64): Rgb {
    const hn: f64 = ccNormHue(h) / (360.0 as f64);
    let wh: f64 = ccClamp01(w / (100.0 as f64));
    let bl: f64 = ccClamp01(b / (100.0 as f64));

    const ratio: f64 = wh + bl;
    if (ratio > (1.0 as f64)) {
        wh = wh / ratio;
        bl = bl / ratio;
    }

    const scaled: f64 = (6.0 as f64) * hn;
    const i: u32 = (scaled as u32) % (6 as u32);
    const v: f64 = (1.0 as f64) - bl;
    let f: f64 = scaled - ((scaled as u32) as f64);
    if ((i % (2 as u32)) != (0 as u32)) {
        f = (1.0 as f64) - f;
    }
    const n: f64 = wh + (f * (v - wh));

    let rf: f64 = v;
    let gf: f64 = n;
    let bf: f64 = wh;
    if (i == 1) {
        rf = n; gf = v; bf = wh;
    } else if (i == 2) {
        rf = wh; gf = v; bf = n;
    } else if (i == 3) {
        rf = wh; gf = n; bf = v;
    } else if (i == 4) {
        rf = n; gf = wh; bf = v;
    } else if (i == 5) {
        rf = v; gf = wh; bf = n;
    }

    return Rgb {
        r: ccRoundU8(rf * (255.0 as f64)),
        g: ccRoundU8(gf * (255.0 as f64)),
        b: ccRoundU8(bf * (255.0 as f64)),
    };
}

/// Convert 8-bit RGB `(0..255)` to `Cmyk` (`c`, `m`, `y`, `k` in `[0, 100]`).
pub fn rgbToCmyk(r: u8, g: u8, b: u8): Cmyk {
    const rf: f64 = (r as f64) / (255.0 as f64);
    const gf: f64 = (g as f64) / (255.0 as f64);
    const bf: f64 = (b as f64) / (255.0 as f64);

    let maxC: f64 = rf;
    if (gf > maxC) { maxC = gf; }
    if (bf > maxC) { maxC = bf; }

    if (maxC <= (0.000000001 as f64)) {
        return Cmyk {
            c: 0.0 as f64,
            m: 0.0 as f64,
            y: 0.0 as f64,
            k: 100.0 as f64,
        };
    }

    const k: f64 = (1.0 as f64) - maxC;
    const c: f64 = ((1.0 as f64) - rf - k) / maxC;
    const m: f64 = ((1.0 as f64) - gf - k) / maxC;
    const y: f64 = ((1.0 as f64) - bf - k) / maxC;

    return Cmyk {
        c: ccClamp01(c) * (100.0 as f64),
        m: ccClamp01(m) * (100.0 as f64),
        y: ccClamp01(y) * (100.0 as f64),
        k: ccClamp01(k) * (100.0 as f64),
    };
}

/// Convert `Cmyk` (`c`, `m`, `y`, `k` in `[0, 100]`) to 8-bit `Rgb`.
pub fn cmykToRgb(c: f64, m: f64, y: f64, k: f64): Rgb {
    const cf: f64 = ccClamp01(c / (100.0 as f64));
    const mf: f64 = ccClamp01(m / (100.0 as f64));
    const yf: f64 = ccClamp01(y / (100.0 as f64));
    const kf: f64 = ccClamp01(k / (100.0 as f64));
    const invK: f64 = (1.0 as f64) - kf;

    return Rgb {
        r: ccRoundU8(((1.0 as f64) - cf) * invK * (255.0 as f64)),
        g: ccRoundU8(((1.0 as f64) - mf) * invK * (255.0 as f64)),
        b: ccRoundU8(((1.0 as f64) - yf) * invK * (255.0 as f64)),
    };
}

/// Convert 8-bit RGB to xterm 256-color index (`16..231` 6x6x6 cube, `232..255` 24-step grayscale).
pub fn rgbToAnsi256(r: u8, g: u8, b: u8): u8 {
    if (r == g && g == b) {
        if (r < 8) {
            return 16;
        }
        if (r > 248) {
            return 231;
        }
        const grayStep: u8 = ccRoundU8(((((r as f64) - (8.0 as f64)) / (247.0 as f64)) * (24.0 as f64)));
        return grayStep + 232;
    }

    const ri: u8 = ccRoundU8(((r as f64) / (255.0 as f64)) * (5.0 as f64));
    const gi: u8 = ccRoundU8(((g as f64) / (255.0 as f64)) * (5.0 as f64));
    const bi: u8 = ccRoundU8(((b as f64) / (255.0 as f64)) * (5.0 as f64));
    return (16 as u8) + ((36 as u8) * ri) + ((6 as u8) * gi) + bi;
}

/// Convert xterm 256-color index (`0..255`) to 8-bit `Rgb`.
pub fn ansi256ToRgb(code: u8): Rgb {
    if (code >= 232) {
        const gray: u8 = ((code - 232) * 10) + 8;
        return Rgb { r: gray, g: gray, b: gray };
    }
    if (code < 16) {
        return ccAnsiStd16(code);
    }

    const idx: u8 = code - 16;
    const ri: u8 = idx / 36;
    const rem: u8 = idx % 36;
    const gi: u8 = rem / 6;
    const bi: u8 = rem % 6;

    return Rgb {
        r: ri * 51,
        g: gi * 51,
        b: bi * 51,
    };
}

/// Convert 8-bit RGB to standard ANSI 16 SGR foreground code (`30..37` normal, `90..97` bright).
pub fn rgbToAnsi16(r: u8, g: u8, b: u8): u8 {
    let maxU: u8 = r;
    if (g > maxU) { maxU = g; }
    if (b > maxU) { maxU = b; }

    const vPct: f64 = ((maxU as f64) / (255.0 as f64)) * (100.0 as f64);
    const val: u8 = ccRoundU8(vPct / (50.0 as f64));
    if (val == 0) {
        return 30;
    }

    const rb: u8 = ccRoundU8((b as f64) / (255.0 as f64));
    const rg: u8 = ccRoundU8((g as f64) / (255.0 as f64));
    const rr: u8 = ccRoundU8((r as f64) / (255.0 as f64));

    let ansi: u8 = (30 as u8) + ((rb * 4) + (rg * 2) + rr);
    if (val == 2) {
        ansi = ansi + 60;
    }
    return ansi;
}

/// Convert xterm 256-color index (`0..255`) to standard ANSI 16 SGR code (`30..37` / `90..97`).
pub fn ansi256ToAnsi16(code: u8): u8 {
    const c: Rgb = ansi256ToRgb(code);
    return rgbToAnsi16(c.r, c.g, c.b);
}

/// Convert ANSI 16 SGR code (`30..37` / `90..97`) to 8-bit `Rgb`.
pub fn ansi16ToRgb(code: u8): Rgb {
    if (code >= 30 && code <= 37) {
        return ccAnsiStd16(code - 30);
    }
    if (code >= 90 && code <= 97) {
        return ccAnsiStd16((code - 90) + 8);
    }
    return Rgb { r: 0, g: 0, b: 0 };
}

/// Format 8-bit RGB into 6 uppercase hex ASCII characters (e.g. `"FF8800"`) into `out`.
/// Returns `6` on success, or `0` if `out.len < 6`.
pub fn rgbToHex(r: u8, g: u8, b: u8, out: []u8): usize {
    if (out.len < 6) {
        return 0;
    }
    out[0] = CC_HEX_UPPER[(r / 16) as usize];
    out[1] = CC_HEX_UPPER[(r % 16) as usize];
    out[2] = CC_HEX_UPPER[(g / 16) as usize];
    out[3] = CC_HEX_UPPER[(g % 16) as usize];
    out[4] = CC_HEX_UPPER[(b / 16) as usize];
    out[5] = CC_HEX_UPPER[(b % 16) as usize];
    return 6;
}

/// Parse a CSS hex color (`#RGB`, `RGB`, `#RRGGBB`, or `RRGGBB`, case-insensitive).
/// Returns `RgbColor { r, g, b, found: true }` on success, or `found: false` on invalid input.
pub fn hexToRgb(hex: string): RgbColor {
    let start: usize = 0;
    if (hex.len > 0 && hex[0] == 35) {
        start = 1;
    }
    const rem: usize = hex.len - start;
    if (rem == 3) {
        const d0: i16 = ccHexVal(hex[start]);
        const d1: i16 = ccHexVal(hex[start + 1]);
        const d2: i16 = ccHexVal(hex[start + 2]);
        if (d0 < 0 || d1 < 0 || d2 < 0) {
            return RgbColor { r: 0, g: 0, b: 0, found: false };
        }
        return RgbColor {
            r: (d0 * 17) as u8,
            g: (d1 * 17) as u8,
            b: (d2 * 17) as u8,
            found: true,
        };
    }
    if (rem == 6) {
        const d0: i16 = ccHexVal(hex[start]);
        const d1: i16 = ccHexVal(hex[start + 1]);
        const d2: i16 = ccHexVal(hex[start + 2]);
        const d3: i16 = ccHexVal(hex[start + 3]);
        const d4: i16 = ccHexVal(hex[start + 4]);
        const d5: i16 = ccHexVal(hex[start + 5]);
        if (d0 < 0 || d1 < 0 || d2 < 0 || d3 < 0 || d4 < 0 || d5 < 0) {
            return RgbColor { r: 0, g: 0, b: 0, found: false };
        }
        return RgbColor {
            r: ((d0 * 16) + d1) as u8,
            g: ((d2 * 16) + d3) as u8,
            b: ((d4 * 16) + d5) as u8,
            found: true,
        };
    }
    return RgbColor { r: 0, g: 0, b: 0, found: false };
}

/// Look up a W3C CSS4 named color via `@cap/color-name` (case-insensitive).
pub fn keywordToRgb(name: string): RgbColor {
    return lookupColor(name);
}

/// Reverse-lookup a W3C CSS4 color name from exact `(r, g, b)` via `@cap/color-name`.
pub fn rgbToKeyword(r: u8, g: u8, b: u8): string {
    return reverseLookupColor(r, g, b);
}

/// Convert 8-bit RGB to grayscale percentage `[0.0, 100.0]`.
pub fn rgbToGray(r: u8, g: u8, b: u8): f64 {
    const avg: f64 = ((r as f64) + (g as f64) + (b as f64)) / (3.0 as f64);
    return (avg / (255.0 as f64)) * (100.0 as f64);
}

/// Convert grayscale percentage `[0.0, 100.0]` to 8-bit `Rgb`.
pub fn grayToRgb(gray: f64): Rgb {
    const c: u8 = ccRoundU8(ccClamp01(gray / (100.0 as f64)) * (255.0 as f64));
    return Rgb { r: c, g: c, b: c };
}

export {
    Rgb,
    RgbColor,
    Hsv,
    Hwb,
    Hsl,
    Cmyk,
    rgbToHsv,
    hsvToRgb,
    rgbToHsl,
    hslToRgb,
    rgbToHwb,
    hwbToRgb,
    rgbToCmyk,
    cmykToRgb,
    rgbToAnsi256,
    ansi256ToRgb,
    rgbToAnsi16,
    ansi256ToAnsi16,
    ansi16ToRgb,
    rgbToHex,
    hexToRgb,
    keywordToRgb,
    rgbToKeyword,
    rgbToGray,
    grayToRgb,
};
