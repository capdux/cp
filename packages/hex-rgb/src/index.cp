// packages/@cap/hex-color/src/index.cp
// Zero-allocation CSS hex color parser, formatter, alpha blender & luminance helper.
// MIT clean-room pure CP implementation.

import { trim, hexVal } from "cap:ascii";
import { writeHexByte } from "cap:fmt";

/// 8-bit per-channel RGBA color with validity flag.
pub struct Rgba {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
    pub valid: bool,
}

/// Parse a CSS hex color string (`#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`, optional leading `#`).
/// Expands shorthand nibbles (e.g. `#f0a` -> `#ff00aa`). Returns `valid: false` on malformed input.
pub fn parseHexColor(input: string): Rgba {
    let invalid = Rgba{
        r: 0 as u8,
        g: 0 as u8,
        b: 0 as u8,
        a: 0 as u8,
        valid: false,
    };

    let s = trim(input);
    if (s.len == 0) {
        return invalid;
    }
    if (s[0] == ('#' as u8)) {
        s = s[1..s.len];
    }

    if (s.len != 3 && s.len != 4 && s.len != 6 && s.len != 8) {
        return invalid;
    }

    let d: [8]u8 = undefined;
    let i: usize = 0;
    while (i < s.len) {
        if (hv := hexVal(s[i])) {
            d[i] = hv;
        } else {
            return invalid;
        }
        i += 1;
    }

    if (s.len == 3) {
        return Rgba{
            r: (d[0] << (4 as u3)) | d[0],
            g: (d[1] << (4 as u3)) | d[1],
            b: (d[2] << (4 as u3)) | d[2],
            a: 255 as u8,
            valid: true,
        };
    }
    if (s.len == 4) {
        return Rgba{
            r: (d[0] << (4 as u3)) | d[0],
            g: (d[1] << (4 as u3)) | d[1],
            b: (d[2] << (4 as u3)) | d[2],
            a: (d[3] << (4 as u3)) | d[3],
            valid: true,
        };
    }
    if (s.len == 6) {
        return Rgba{
            r: (d[0] << (4 as u3)) | d[1],
            g: (d[2] << (4 as u3)) | d[3],
            b: (d[4] << (4 as u3)) | d[5],
            a: 255 as u8,
            valid: true,
        };
    }
    return Rgba{
        r: (d[0] << (4 as u3)) | d[1],
        g: (d[2] << (4 as u3)) | d[3],
        b: (d[4] << (4 as u3)) | d[5],
        a: (d[6] << (4 as u3)) | d[7],
        valid: true,
    };
}

/// Format `c` as 7-byte `#rrggbb` (or `#RRGGBB`) in `out`. Returns `7`, or `0` if `out.len < 7`.
pub fn formatHex6(c: Rgba, uppercase: bool, out: []u8): usize {
    if (out.len < 7) {
        return 0;
    }
    out[0] = '#' as u8;
    let pos = writeHexByte(c.r, uppercase, out, 1);
    pos = writeHexByte(c.g, uppercase, out, pos);
    return writeHexByte(c.b, uppercase, out, pos);
}

/// Format `c` as 9-byte `#rrggbbaa` (or `#RRGGBBAA`) in `out`. Returns `9`, or `0` if `out.len < 9`.
pub fn formatHex8(c: Rgba, uppercase: bool, out: []u8): usize {
    if (out.len < 9) {
        return 0;
    }
    out[0] = '#' as u8;
    let pos = writeHexByte(c.r, uppercase, out, 1);
    pos = writeHexByte(c.g, uppercase, out, pos);
    pos = writeHexByte(c.b, uppercase, out, pos);
    return writeHexByte(c.a, uppercase, out, pos);
}

/// Pack `c` into a big-endian `0xRRGGBBAA` 32-bit integer.
pub fn packRgba32(c: Rgba): u32 {
    return ((c.r as u32) << (24 as u5)) | ((c.g as u32) << (16 as u5)) | ((c.b as u32) << (8 as u5)) | (c.a as u32);
}

/// Unpack a big-endian `0xRRGGBBAA` 32-bit integer into an `Rgba`.
pub fn unpackRgba32(v: u32): Rgba {
    return Rgba{
        r: ((v >> (24 as u5)) & (0xFF as u32)) as u8,
        g: ((v >> (16 as u5)) & (0xFF as u32)) as u8,
        b: ((v >> (8 as u5)) & (0xFF as u32)) as u8,
        a: (v & (0xFF as u32)) as u8,
        valid: true,
    };
}

/// Invert RGB channels (`255 - ch`) while preserving alpha `a`.
pub fn invertRgb(c: Rgba): Rgba {
    return Rgba{
        r: (255 as u8) - c.r,
        g: (255 as u8) - c.g,
        b: (255 as u8) - c.b,
        a: c.a,
        valid: c.valid,
    };
}

/// Returns true if `c` is perceptually dark per BT.601 luma `(299*r + 587*g + 114*b) < 128000`.
pub fn isDark(c: Rgba): bool {
    const luma: u32 = ((299 as u32) * (c.r as u32)) + ((587 as u32) * (c.g as u32)) + ((114 as u32) * (c.b as u32));
    return luma < (128000 as u32);
}

fn hcClampU8(v: f64): u8 {
    if (v <= (0.0 as f64)) {
        return 0 as u8;
    }
    if (v >= (255.0 as f64)) {
        return 255 as u8;
    }
    return ((v + (0.5 as f64)) as u32) as u8;
}

/// Standard Porter-Duff `source-over` alpha compositing of `fg` over `bg`.
pub fn blendOver(fg: Rgba, bg: Rgba): Rgba {
    const fa: f64 = (fg.a as f64) / (255.0 as f64);
    const ba: f64 = (bg.a as f64) / (255.0 as f64);
    const invFa: f64 = (1.0 as f64) - fa;
    const outA: f64 = fa + ba * invFa;

    if (outA <= (0.0000001 as f64)) {
        return Rgba{
            r: 0 as u8,
            g: 0 as u8,
            b: 0 as u8,
            a: 0 as u8,
            valid: true,
        };
    }

    const r: f64 = (((fg.r as f64) * fa) + ((bg.r as f64) * ba * invFa)) / outA;
    const g: f64 = (((fg.g as f64) * fa) + ((bg.g as f64) * ba * invFa)) / outA;
    const b: f64 = (((fg.b as f64) * fa) + ((bg.b as f64) * ba * invFa)) / outA;

    return Rgba{
        r: hcClampU8(r),
        g: hcClampU8(g),
        b: hcClampU8(b),
        a: hcClampU8(outA * (255.0 as f64)),
        valid: true,
    };
}

export {
    Rgba,
    parseHexColor,
    formatHex6,
    formatHex8,
    packRgba32,
    unpackRgba32,
    invertRgb,
    isDark,
    blendOver,
};
