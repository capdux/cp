// @cap/color-names — Zero-allocation 148 W3C CSS Color 4 named colors & color space conversion
// Clean-room MIT implementation inspired by npm `color-name` and `color-convert`.

pub struct Rgb {
    pub r: u8,
    pub g: u8,
    pub b: u8,

    pub static fn from(r: u8, g: u8, b: u8) -> Rgb {
        return Rgb{ r: r, g: g, b: b };
    }
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

struct NamedColor {
    name: string,
    r: u8,
    g: u8,
    b: u8,
}

// All 148 W3C CSS Color Module Level 4 named colors, sorted lexicographically for O(log N) lookup.
const CSS_COLORS: [148]NamedColor = [
    NamedColor{ name: "aliceblue", r: 240, g: 248, b: 255 },
    NamedColor{ name: "antiquewhite", r: 250, g: 235, b: 215 },
    NamedColor{ name: "aqua", r: 0, g: 255, b: 255 },
    NamedColor{ name: "aquamarine", r: 127, g: 255, b: 212 },
    NamedColor{ name: "azure", r: 240, g: 255, b: 255 },
    NamedColor{ name: "beige", r: 245, g: 245, b: 220 },
    NamedColor{ name: "bisque", r: 255, g: 228, b: 196 },
    NamedColor{ name: "black", r: 0, g: 0, b: 0 },
    NamedColor{ name: "blanchedalmond", r: 255, g: 235, b: 205 },
    NamedColor{ name: "blue", r: 0, g: 0, b: 255 },
    NamedColor{ name: "blueviolet", r: 138, g: 43, b: 226 },
    NamedColor{ name: "brown", r: 165, g: 42, b: 42 },
    NamedColor{ name: "burlywood", r: 222, g: 184, b: 135 },
    NamedColor{ name: "cadetblue", r: 95, g: 158, b: 160 },
    NamedColor{ name: "chartreuse", r: 127, g: 255, b: 0 },
    NamedColor{ name: "chocolate", r: 210, g: 105, b: 30 },
    NamedColor{ name: "coral", r: 255, g: 127, b: 80 },
    NamedColor{ name: "cornflowerblue", r: 100, g: 149, b: 237 },
    NamedColor{ name: "cornsilk", r: 255, g: 248, b: 220 },
    NamedColor{ name: "crimson", r: 220, g: 20, b: 60 },
    NamedColor{ name: "cyan", r: 0, g: 255, b: 255 },
    NamedColor{ name: "darkblue", r: 0, g: 0, b: 139 },
    NamedColor{ name: "darkcyan", r: 0, g: 139, b: 139 },
    NamedColor{ name: "darkgoldenrod", r: 184, g: 134, b: 11 },
    NamedColor{ name: "darkgray", r: 169, g: 169, b: 169 },
    NamedColor{ name: "darkgreen", r: 0, g: 100, b: 0 },
    NamedColor{ name: "darkgrey", r: 169, g: 169, b: 169 },
    NamedColor{ name: "darkkhaki", r: 189, g: 183, b: 107 },
    NamedColor{ name: "darkmagenta", r: 139, g: 0, b: 139 },
    NamedColor{ name: "darkolivegreen", r: 85, g: 107, b: 47 },
    NamedColor{ name: "darkorange", r: 255, g: 140, b: 0 },
    NamedColor{ name: "darkorchid", r: 153, g: 50, b: 204 },
    NamedColor{ name: "darkred", r: 139, g: 0, b: 0 },
    NamedColor{ name: "darksalmon", r: 233, g: 150, b: 122 },
    NamedColor{ name: "darkseagreen", r: 143, g: 188, b: 143 },
    NamedColor{ name: "darkslateblue", r: 72, g: 61, b: 139 },
    NamedColor{ name: "darkslategray", r: 47, g: 79, b: 79 },
    NamedColor{ name: "darkslategrey", r: 47, g: 79, b: 79 },
    NamedColor{ name: "darkturquoise", r: 0, g: 206, b: 209 },
    NamedColor{ name: "darkviolet", r: 148, g: 0, b: 211 },
    NamedColor{ name: "deeppink", r: 255, g: 20, b: 147 },
    NamedColor{ name: "deepskyblue", r: 0, g: 191, b: 255 },
    NamedColor{ name: "dimgray", r: 105, g: 105, b: 105 },
    NamedColor{ name: "dimgrey", r: 105, g: 105, b: 105 },
    NamedColor{ name: "dodgerblue", r: 30, g: 144, b: 255 },
    NamedColor{ name: "firebrick", r: 178, g: 34, b: 34 },
    NamedColor{ name: "floralwhite", r: 255, g: 250, b: 240 },
    NamedColor{ name: "forestgreen", r: 34, g: 139, b: 34 },
    NamedColor{ name: "fuchsia", r: 255, g: 0, b: 255 },
    NamedColor{ name: "gainsboro", r: 220, g: 220, b: 220 },
    NamedColor{ name: "ghostwhite", r: 248, g: 248, b: 255 },
    NamedColor{ name: "gold", r: 255, g: 215, b: 0 },
    NamedColor{ name: "goldenrod", r: 218, g: 165, b: 32 },
    NamedColor{ name: "gray", r: 128, g: 128, b: 128 },
    NamedColor{ name: "green", r: 0, g: 128, b: 0 },
    NamedColor{ name: "greenyellow", r: 173, g: 255, b: 47 },
    NamedColor{ name: "grey", r: 128, g: 128, b: 128 },
    NamedColor{ name: "honeydew", r: 240, g: 255, b: 240 },
    NamedColor{ name: "hotpink", r: 255, g: 105, b: 180 },
    NamedColor{ name: "indianred", r: 205, g: 92, b: 92 },
    NamedColor{ name: "indigo", r: 75, g: 0, b: 130 },
    NamedColor{ name: "ivory", r: 255, g: 255, b: 240 },
    NamedColor{ name: "khaki", r: 240, g: 230, b: 140 },
    NamedColor{ name: "lavender", r: 230, g: 230, b: 250 },
    NamedColor{ name: "lavenderblush", r: 255, g: 240, b: 245 },
    NamedColor{ name: "lawngreen", r: 124, g: 252, b: 0 },
    NamedColor{ name: "lemonchiffon", r: 255, g: 250, b: 205 },
    NamedColor{ name: "lightblue", r: 173, g: 216, b: 230 },
    NamedColor{ name: "lightcoral", r: 240, g: 128, b: 128 },
    NamedColor{ name: "lightcyan", r: 224, g: 255, b: 255 },
    NamedColor{ name: "lightgoldenrodyellow", r: 250, g: 250, b: 210 },
    NamedColor{ name: "lightgray", r: 211, g: 211, b: 211 },
    NamedColor{ name: "lightgreen", r: 144, g: 238, b: 144 },
    NamedColor{ name: "lightgrey", r: 211, g: 211, b: 211 },
    NamedColor{ name: "lightpink", r: 255, g: 182, b: 193 },
    NamedColor{ name: "lightsalmon", r: 255, g: 160, b: 122 },
    NamedColor{ name: "lightseagreen", r: 32, g: 178, b: 170 },
    NamedColor{ name: "lightskyblue", r: 135, g: 206, b: 250 },
    NamedColor{ name: "lightslategray", r: 119, g: 136, b: 153 },
    NamedColor{ name: "lightslategrey", r: 119, g: 136, b: 153 },
    NamedColor{ name: "lightsteelblue", r: 176, g: 196, b: 222 },
    NamedColor{ name: "lightyellow", r: 255, g: 255, b: 224 },
    NamedColor{ name: "lime", r: 0, g: 255, b: 0 },
    NamedColor{ name: "limegreen", r: 50, g: 205, b: 50 },
    NamedColor{ name: "linen", r: 250, g: 240, b: 230 },
    NamedColor{ name: "magenta", r: 255, g: 0, b: 255 },
    NamedColor{ name: "maroon", r: 128, g: 0, b: 0 },
    NamedColor{ name: "mediumaquamarine", r: 102, g: 205, b: 170 },
    NamedColor{ name: "mediumblue", r: 0, g: 0, b: 205 },
    NamedColor{ name: "mediumorchid", r: 186, g: 85, b: 211 },
    NamedColor{ name: "mediumpurple", r: 147, g: 112, b: 219 },
    NamedColor{ name: "mediumseagreen", r: 60, g: 179, b: 113 },
    NamedColor{ name: "mediumslateblue", r: 123, g: 104, b: 238 },
    NamedColor{ name: "mediumspringgreen", r: 0, g: 250, b: 154 },
    NamedColor{ name: "mediumturquoise", r: 72, g: 209, b: 204 },
    NamedColor{ name: "mediumvioletred", r: 199, g: 21, b: 133 },
    NamedColor{ name: "midnightblue", r: 25, g: 25, b: 112 },
    NamedColor{ name: "mintcream", r: 245, g: 255, b: 250 },
    NamedColor{ name: "mistyrose", r: 255, g: 228, b: 225 },
    NamedColor{ name: "moccasin", r: 255, g: 228, b: 181 },
    NamedColor{ name: "navajowhite", r: 255, g: 222, b: 173 },
    NamedColor{ name: "navy", r: 0, g: 0, b: 128 },
    NamedColor{ name: "oldlace", r: 253, g: 245, b: 230 },
    NamedColor{ name: "olive", r: 128, g: 128, b: 0 },
    NamedColor{ name: "olivedrab", r: 107, g: 142, b: 35 },
    NamedColor{ name: "orange", r: 255, g: 165, b: 0 },
    NamedColor{ name: "orangered", r: 255, g: 69, b: 0 },
    NamedColor{ name: "orchid", r: 218, g: 112, b: 214 },
    NamedColor{ name: "palegoldenrod", r: 238, g: 232, b: 170 },
    NamedColor{ name: "palegreen", r: 152, g: 251, b: 152 },
    NamedColor{ name: "paleturquoise", r: 175, g: 238, b: 238 },
    NamedColor{ name: "palevioletred", r: 219, g: 112, b: 147 },
    NamedColor{ name: "papayawhip", r: 255, g: 239, b: 213 },
    NamedColor{ name: "peachpuff", r: 255, g: 218, b: 185 },
    NamedColor{ name: "peru", r: 205, g: 133, b: 63 },
    NamedColor{ name: "pink", r: 255, g: 192, b: 203 },
    NamedColor{ name: "plum", r: 221, g: 160, b: 221 },
    NamedColor{ name: "powderblue", r: 176, g: 224, b: 230 },
    NamedColor{ name: "purple", r: 128, g: 0, b: 128 },
    NamedColor{ name: "rebeccapurple", r: 102, g: 51, b: 153 },
    NamedColor{ name: "red", r: 255, g: 0, b: 0 },
    NamedColor{ name: "rosybrown", r: 188, g: 143, b: 143 },
    NamedColor{ name: "royalblue", r: 65, g: 105, b: 225 },
    NamedColor{ name: "saddlebrown", r: 139, g: 69, b: 19 },
    NamedColor{ name: "salmon", r: 250, g: 128, b: 114 },
    NamedColor{ name: "sandybrown", r: 244, g: 164, b: 96 },
    NamedColor{ name: "seagreen", r: 46, g: 139, b: 87 },
    NamedColor{ name: "seashell", r: 255, g: 245, b: 238 },
    NamedColor{ name: "sienna", r: 160, g: 82, b: 45 },
    NamedColor{ name: "silver", r: 192, g: 192, b: 192 },
    NamedColor{ name: "skyblue", r: 135, g: 206, b: 235 },
    NamedColor{ name: "slateblue", r: 106, g: 90, b: 205 },
    NamedColor{ name: "slategray", r: 112, g: 128, b: 144 },
    NamedColor{ name: "slategrey", r: 112, g: 128, b: 144 },
    NamedColor{ name: "snow", r: 255, g: 250, b: 250 },
    NamedColor{ name: "springgreen", r: 0, g: 255, b: 127 },
    NamedColor{ name: "steelblue", r: 70, g: 130, b: 180 },
    NamedColor{ name: "tan", r: 210, g: 180, b: 140 },
    NamedColor{ name: "teal", r: 0, g: 128, b: 128 },
    NamedColor{ name: "thistle", r: 216, g: 191, b: 216 },
    NamedColor{ name: "tomato", r: 255, g: 99, b: 71 },
    NamedColor{ name: "turquoise", r: 64, g: 224, b: 208 },
    NamedColor{ name: "violet", r: 238, g: 130, b: 238 },
    NamedColor{ name: "wheat", r: 245, g: 222, b: 179 },
    NamedColor{ name: "white", r: 255, g: 255, b: 255 },
    NamedColor{ name: "whitesmoke", r: 245, g: 245, b: 245 },
    NamedColor{ name: "yellow", r: 255, g: 255, b: 0 },
    NamedColor{ name: "yellowgreen", r: 154, g: 205, b: 50 },
];

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isWs(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: string) -> string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

/// Case-insensitive lexicographical comparison against an already-lowercase `target`.
/// Returns -1 if `a < target`, 0 if equal, 1 if `a > target`.
fn cmpIgnoreCase(a: string, target: string) -> i32 {
    const min_len = if (a.len < target.len) a.len else target.len;
    let i: usize = 0;
    while (i < min_len) {
        const ca = toLowerAscii(a[i]);
        const ct = target[i];
        if (ca < ct) return -1;
        if (ca > ct) return 1;
        i += 1;
    }
    if (a.len < target.len) return -1;
    if (a.len > target.len) return 1;
    return 0;
}

fn clamp01(v: f64) -> f64 {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

fn normalizeHue(h: f64) -> f64 {
    let res = h;
    while (res < 0.0) {
        res += 360.0;
    }
    while (res >= 360.0) {
        res -= 360.0;
    }
    return res;
}

fn roundToU8(v: f64) -> u8 {
    if (v <= 0.0) return 0;
    if (v >= 255.0) return 255;
    return (v + 0.5) as u8;
}

fn absF64(x: f64) -> f64 {
    return if (x < 0.0) -x else x;
}

// ---------------------------------------------------------------------------
// 1. W3C CSS Color 4 Named Color Lookup
// ---------------------------------------------------------------------------

/// Returns the total number of built-in W3C CSS Color 4 named colors (148).
pub fn cssColorCount() -> usize {
    return CSS_COLORS.len;
}

/// Looks up a W3C CSS Color 4 named color (case-insensitive, trims surrounding whitespace).
pub fn rgbFromCssName(name: string) -> Rgb? {
    const trimmed = trimWs(name);
    if (trimmed.len == 0) {
        return null;
    }
    let lo: usize = 0;
    let hi: usize = CSS_COLORS.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        const cmp = cmpIgnoreCase(trimmed, CSS_COLORS[mid].name);
        if (cmp == 0) {
            const c = CSS_COLORS[mid];
            return Rgb{ r: c.r, g: c.g, b: c.b };
        }
        if (cmp < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return null;
}

/// Reverse lookup: returns the canonical W3C CSS color name matching `c`, or `null` if none.
pub fn cssNameFromRgb(c: Rgb) -> string? {
    let i: usize = 0;
    while (i < CSS_COLORS.len) {
        const entry = CSS_COLORS[i];
        if (entry.r == c.r && entry.g == c.g && entry.b == c.b) {
            return entry.name;
        }
        i += 1;
    }
    return null;
}

// ---------------------------------------------------------------------------
// 2. Color Space Conversions (RGB <-> HSL, RGB <-> HSV, RGB <-> HEX)
// ---------------------------------------------------------------------------

/// Converts `Rgb` (0..255) to `Hsl` (`h` in `[0, 360)`, `s` and `l` in `[0, 1]`).
pub fn rgbToHsl(c: Rgb) -> Hsl {
    const r = (c.r as f64) / 255.0;
    const g = (c.g as f64) / 255.0;
    const b = (c.b as f64) / 255.0;

    let max_c = r;
    if (g > max_c) max_c = g;
    if (b > max_c) max_c = b;

    let min_c = r;
    if (g < min_c) min_c = g;
    if (b < min_c) min_c = b;

    const delta = max_c - min_c;
    const l = (max_c + min_c) / 2.0;

    if (delta == 0.0) {
        return Hsl{ h: 0.0, s: 0.0, l: l };
    }

    const denom = 1.0 - absF64(2.0 * l - 1.0);
    const s = if (denom == 0.0) 0.0 else delta / denom;

    let h: f64 = 0.0;
    if (max_c == r) {
        h = 60.0 * ((g - b) / delta);
        if (h < 0.0) {
            h += 360.0;
        }
    } else if (max_c == g) {
        h = 60.0 * (((b - r) / delta) + 2.0);
    } else {
        h = 60.0 * (((r - g) / delta) + 4.0);
    }

    return Hsl{ h: normalizeHue(h), s: clamp01(s), l: clamp01(l) };
}

/// Converts `Hsl` (`h` in degrees, `s`/`l` in `[0, 1]`) to `Rgb` (0..255).
pub fn hslToRgb(c: Hsl) -> Rgb {
    const h = normalizeHue(c.h);
    const s = clamp01(c.s);
    const l = clamp01(c.l);

    const chroma = (1.0 - absF64(2.0 * l - 1.0)) * s;
    const h_prime = h / 60.0;
    const h_mod2 = h_prime - 2.0 * ((h_prime / 2.0) as i32 as f64);
    const x = chroma * (1.0 - absF64(h_mod2 - 1.0));
    const m = l - chroma / 2.0;

    let r1: f64 = 0.0;
    let g1: f64 = 0.0;
    let b1: f64 = 0.0;

    if (h < 60.0) {
        r1 = chroma; g1 = x; b1 = 0.0;
    } else if (h < 120.0) {
        r1 = x; g1 = chroma; b1 = 0.0;
    } else if (h < 180.0) {
        r1 = 0.0; g1 = chroma; b1 = x;
    } else if (h < 240.0) {
        r1 = 0.0; g1 = x; b1 = chroma;
    } else if (h < 300.0) {
        r1 = x; g1 = 0.0; b1 = chroma;
    } else {
        r1 = chroma; g1 = 0.0; b1 = x;
    }

    return Rgb{
        r: roundToU8((r1 + m) * 255.0),
        g: roundToU8((g1 + m) * 255.0),
        b: roundToU8((b1 + m) * 255.0),
    };
}

/// Converts `Rgb` (0..255) to `Hsv` (`h` in `[0, 360)`, `s` and `v` in `[0, 1]`).
pub fn rgbToHsv(c: Rgb) -> Hsv {
    const r = (c.r as f64) / 255.0;
    const g = (c.g as f64) / 255.0;
    const b = (c.b as f64) / 255.0;

    let max_c = r;
    if (g > max_c) max_c = g;
    if (b > max_c) max_c = b;

    let min_c = r;
    if (g < min_c) min_c = g;
    if (b < min_c) min_c = b;

    const delta = max_c - min_c;
    const v = max_c;
    const s = if (max_c == 0.0) 0.0 else delta / max_c;

    if (delta == 0.0) {
        return Hsv{ h: 0.0, s: 0.0, v: v };
    }

    let h: f64 = 0.0;
    if (max_c == r) {
        h = 60.0 * ((g - b) / delta);
        if (h < 0.0) {
            h += 360.0;
        }
    } else if (max_c == g) {
        h = 60.0 * (((b - r) / delta) + 2.0);
    } else {
        h = 60.0 * (((r - g) / delta) + 4.0);
    }

    return Hsv{ h: normalizeHue(h), s: clamp01(s), v: clamp01(v) };
}

/// Converts `Hsv` (`h` in degrees, `s`/`v` in `[0, 1]`) to `Rgb` (0..255).
pub fn hsvToRgb(c: Hsv) -> Rgb {
    const h = normalizeHue(c.h);
    const s = clamp01(c.s);
    const v = clamp01(c.v);

    const chroma = v * s;
    const h_prime = h / 60.0;
    const h_mod2 = h_prime - 2.0 * ((h_prime / 2.0) as i32 as f64);
    const x = chroma * (1.0 - absF64(h_mod2 - 1.0));
    const m = v - chroma;

    let r1: f64 = 0.0;
    let g1: f64 = 0.0;
    let b1: f64 = 0.0;

    if (h < 60.0) {
        r1 = chroma; g1 = x; b1 = 0.0;
    } else if (h < 120.0) {
        r1 = x; g1 = chroma; b1 = 0.0;
    } else if (h < 180.0) {
        r1 = 0.0; g1 = chroma; b1 = x;
    } else if (h < 240.0) {
        r1 = 0.0; g1 = x; b1 = chroma;
    } else if (h < 300.0) {
        r1 = x; g1 = 0.0; b1 = chroma;
    } else {
        r1 = chroma; g1 = 0.0; b1 = x;
    }

    return Rgb{
        r: roundToU8((r1 + m) * 255.0),
        g: roundToU8((g1 + m) * 255.0),
        b: roundToU8((b1 + m) * 255.0),
    };
}

fn hexNibbleChar(v: u8) -> u8 {
    if (v < 10) {
        return '0' + v;
    }
    return 'a' + (v - 10);
}

fn parseHexNibble(c: u8) -> u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
    return null;
}

/// Formats `c` as lowercase `"#rrggbb"` into `out`.
pub fn rgbToHex(c: Rgb, out: []u8) -> string {
    if (out.len < 7) {
        return out[0..0];
    }
    out[0] = '#';
    out[1] = hexNibbleChar((c.r >> 4) & 0x0F);
    out[2] = hexNibbleChar(c.r & 0x0F);
    out[3] = hexNibbleChar((c.g >> 4) & 0x0F);
    out[4] = hexNibbleChar(c.g & 0x0F);
    out[5] = hexNibbleChar((c.b >> 4) & 0x0F);
    out[6] = hexNibbleChar(c.b & 0x0F);
    return out[0..7];
}

/// Parses a 6-digit (`#rrggbb` / `rrggbb`) or 3-digit (`#rgb` / `rgb`) hex color string.
pub fn hexToRgb(hex: string) -> Rgb? {
    let s = trimWs(hex);
    if (s.len > 0 && s[0] == '#') {
        s = s[1..s.len];
    }
    if (s.len == 3) {
        const nr = parseHexNibble(s[0]);
        const ng = parseHexNibble(s[1]);
        const nb = parseHexNibble(s[2]);
        if (nr == null || ng == null || nb == null) {
            return null;
        }
        return Rgb{
            r: (nr.? << 4) | nr.?,
            g: (ng.? << 4) | ng.?,
            b: (nb.? << 4) | nb.?,
        };
    }
    if (s.len == 6) {
        const r1 = parseHexNibble(s[0]);
        const r2 = parseHexNibble(s[1]);
        const g1 = parseHexNibble(s[2]);
        const g2 = parseHexNibble(s[3]);
        const b1 = parseHexNibble(s[4]);
        const b2 = parseHexNibble(s[5]);
        if (r1 == null || r2 == null || g1 == null || g2 == null || b1 == null || b2 == null) {
            return null;
        }
        return Rgb{
            r: (r1.? << 4) | r2.?,
            g: (g1.? << 4) | g2.?,
            b: (b1.? << 4) | b2.?,
        };
    }
    return null;
}

// ---------------------------------------------------------------------------
// 3. Color Manipulation Utilities
// ---------------------------------------------------------------------------

/// Linearly interpolates between `a` (`weight = 0.0`) and `b` (`weight = 1.0`).
pub fn mixRgb(a: Rgb, b: Rgb, weight: f64) -> Rgb {
    const w = clamp01(weight);
    const inv = 1.0 - w;
    return Rgb{
        r: roundToU8((a.r as f64) * inv + (b.r as f64) * w),
        g: roundToU8((a.g as f64) * inv + (b.g as f64) * w),
        b: roundToU8((a.b as f64) * inv + (b.b as f64) * w),
    };
}

/// Increases HSL lightness of `c` by `amount` (`0.0..=1.0`).
pub fn lighten(c: Rgb, amount: f64) -> Rgb {
    const hsl = rgbToHsl(c);
    return hslToRgb(Hsl{ h: hsl.h, s: hsl.s, l: clamp01(hsl.l + amount) });
}

/// Decreases HSL lightness of `c` by `amount` (`0.0..=1.0`).
pub fn darken(c: Rgb, amount: f64) -> Rgb {
    const hsl = rgbToHsl(c);
    return hslToRgb(Hsl{ h: hsl.h, s: hsl.s, l: clamp01(hsl.l - amount) });
}
