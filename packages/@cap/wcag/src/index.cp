// @cap/wcag — Zero-allocation W3C WCAG 2.1/2.2 color accessibility & contrast engine for CP
// Clean-room MIT implementation inspired by npm `wcag-contrast`, `color-contrast-checker`, and Rust `contrast`.
//
// Features:
// - Exact W3C WCAG 2.1/2.2 sRGB linearization and relative luminance calculation.
// - Contrast ratio (1.0 .. 21.0) for RGB structs and HEX strings (#RGB / RGB / #RRGGBB / RRGGBB).
// - AA / AAA compliance checks (normal & large text) and WcagLevel grading.
// - Automatic high-contrast text color recommendation (`bestTextColor`, `suggestAccessibleHex`).
// - Zero heap allocation.

const std = @import("std");

pub struct Rgb {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

pub enum WcagLevel: u8 {
    Fail,
    AALarge,
    AA,
    AAA,
}

fn hexNibble(c: u8) -> i8 {
    if (c >= '0' && c <= '9') {
        return (c - '0') as i8;
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10) as i8;
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10) as i8;
    }
    return -1;
}

fn nibbleToHexLower(n: u8) -> u8 {
    if (n < 10) {
        return '0' + n;
    }
    return 'a' + (n - 10);
}

/// Parses a CSS HEX color string (`#RGB`, `RGB`, `#RRGGBB`, or `RRGGBB`, case-insensitive).
/// Returns `null` if the input is malformed.
pub fn parseHex(hex: string) -> Rgb? {
    let start: usize = 0;
    if (hex.len > 0 && hex[0] == '#') {
        start = 1;
    }
    const rem = hex.len - start;

    if (rem == 3) {
        const nr = hexNibble(hex[start]);
        const ng = hexNibble(hex[start + 1]);
        const nb = hexNibble(hex[start + 2]);
        if (nr < 0 || ng < 0 || nb < 0) {
            return null;
        }
        const r = (nr as u8) * 17;
        const g = (ng as u8) * 17;
        const b = (nb as u8) * 17;
        return Rgb{ r: r, g: g, b: b };
    }

    if (rem == 6) {
        const r1 = hexNibble(hex[start]);
        const r2 = hexNibble(hex[start + 1]);
        const g1 = hexNibble(hex[start + 2]);
        const g2 = hexNibble(hex[start + 3]);
        const b1 = hexNibble(hex[start + 4]);
        const b2 = hexNibble(hex[start + 5]);
        if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0) {
            return null;
        }
        const r = ((r1 as u8) * 16) + (r2 as u8);
        const g = ((g1 as u8) * 16) + (g2 as u8);
        const b = ((b1 as u8) * 16) + (b2 as u8);
        return Rgb{ r: r, g: g, b: b };
    }

    return null;
}

/// Formats an `Rgb` color as lowercase `#rrggbb` into caller-provided `out` (needs >= 7 bytes).
pub fn formatHex(c: Rgb, out: []u8) -> string {
    if (out.len < 7) {
        return out[0..0];
    }
    out[0] = '#';
    out[1] = nibbleToHexLower((c.r >> 4) & 0x0f);
    out[2] = nibbleToHexLower(c.r & 0x0f);
    out[3] = nibbleToHexLower((c.g >> 4) & 0x0f);
    out[4] = nibbleToHexLower(c.g & 0x0f);
    out[5] = nibbleToHexLower((c.b >> 4) & 0x0f);
    out[6] = nibbleToHexLower(c.b & 0x0f);
    return out[0..7];
}

fn linearizeChannel(channel: u8) -> f64 {
    const s: f64 = (channel as f64) / 255.0;
    if (s <= 0.04045) {
        return s / 12.92;
    }
    return std.math.pow(f64, (s + 0.055) / 1.055, 2.4);
}

/// Computes W3C WCAG 2.1/2.2 relative luminance in range `[0.0, 1.0]`.
pub fn relativeLuminance(c: Rgb) -> f64 {
    const r = linearizeChannel(c.r);
    const g = linearizeChannel(c.g);
    const b = linearizeChannel(c.b);
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

/// Computes W3C WCAG contrast ratio `(L1 + 0.05) / (L2 + 0.05)` in range `[1.0, 21.0]`.
pub fn contrastRatio(fg: Rgb, bg: Rgb) -> f64 {
    const l1 = relativeLuminance(fg);
    const l2 = relativeLuminance(bg);
    const lighter = if (l1 > l2) { l1 } else { l2 };
    const darker = if (l1 > l2) { l2 } else { l1 };
    return (lighter + 0.05) / (darker + 0.05);
}

/// Parses two HEX color strings and computes their WCAG contrast ratio.
/// Returns `null` if either HEX string is invalid.
pub fn contrastRatioHex(fg_hex: string, bg_hex: string) -> f64? {
    const fg_opt = parseHex(fg_hex);
    if (fg_opt == null) {
        return null;
    }
    const bg_opt = parseHex(bg_hex);
    if (bg_opt == null) {
        return null;
    }
    return contrastRatio(fg_opt.?, bg_opt.?);
}

/// Checks WCAG AA compliance (normal text >= 4.5, large text >= 3.0).
pub fn meetsAA(ratio: f64, large_text: bool) -> bool {
    if (large_text) {
        return ratio >= 3.0;
    }
    return ratio >= 4.5;
}

/// Checks WCAG AAA compliance (normal text >= 7.0, large text >= 4.5).
pub fn meetsAAA(ratio: f64, large_text: bool) -> bool {
    if (large_text) {
        return ratio >= 4.5;
    }
    return ratio >= 7.0;
}

/// Grades a WCAG contrast ratio into `Fail`, `AALarge`, `AA`, or `AAA`.
pub fn gradeContrast(ratio: f64) -> WcagLevel {
    if (ratio >= 7.0) {
        return WcagLevel.AAA;
    }
    if (ratio >= 4.5) {
        return WcagLevel.AA;
    }
    if (ratio >= 3.0) {
        return WcagLevel.AALarge;
    }
    return WcagLevel.Fail;
}

/// Returns pure black `#000000` or pure white `#ffffff`, whichever achieves higher WCAG contrast against `bg`.
pub fn bestTextColor(bg: Rgb) -> Rgb {
    const black = Rgb{ r: 0, g: 0, b: 0 };
    const white = Rgb{ r: 255, g: 255, b: 255 };
    const r_black = contrastRatio(black, bg);
    const r_white = contrastRatio(white, bg);
    if (r_black >= r_white) {
        return black;
    }
    return white;
}

/// Recommends `"#000000"` or `"#ffffff"` for maximum readability over `bg_hex`.
/// Falls back to `"#000000"` if `bg_hex` is invalid.
pub fn suggestAccessibleHex(bg_hex: string) -> string {
    const bg_opt = parseHex(bg_hex);
    if (bg_opt == null) {
        return "#000000";
    }
    const best = bestTextColor(bg_opt.?);
    if (best.r == 0) {
        return "#000000";
    }
    return "#ffffff";
}
