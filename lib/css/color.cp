// Source: Absorbed from /data/dev/bun/src/css/values/color.cp | Synced: 2026-11-23T22:40:10Z
// lib/css/color.cp
// CSS Color Level 4 Parser & Color Space Converter
// Supports Hex (3/4/6/8), RGB/RGBA, HSL/HSLA, HWB, OKLAB, OKLCH, 148 W3C Named Colors, WCAG & Mixing

const std = @import("std");

fn hexDigitVal(c: u8): u8? {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return null;
}

fn clampU8(v: f64): u8 {
    if (v <= 0.0) {
        return 0;
    }
    if (v >= 255.0) {
        return 255;
    }
    return @intFromFloat(@round(v));
}

fn hueToRgb(p: f64, q: f64, t_in: f64): f64 {
    let t = t_in;
    if (t < 0.0) {
        t += 1.0;
    }
    if (t > 1.0) {
        t -= 1.0;
    }
    if (t < 1.0 / 6.0) {
        return p + (q - p) * 6.0 * t;
    }
    if (t < 1.0 / 2.0) {
        return q;
    }
    if (t < 2.0 / 3.0) {
        return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    }
    return p;
}

fn linearToSrgbChannel(x: f64): u8 {
    let c = x;
    if (c < 0.0) {
        c = 0.0;
    }
    if (c > 1.0) {
        c = 1.0;
    }
    const encoded = if (c <= 0.0031308)
        (12.92 * c)
    else
        (1.055 * std.math.pow(f64, c, 1.0 / 2.4) - 0.055);
    return clampU8(encoded * 255.0);
}

fn srgbChannelToLinear(c: u8): f64 {
    const cf: f64 = @floatFromInt(c);
    const v = cf / 255.0;
    if (v <= 0.04045) {
        return v / 12.92;
    }
    return std.math.pow(f64, (v + 0.055) / 1.055, 2.4);
}

fn parseFloatToken(raw: []const u8): f64? {
    const trimmed = std.mem.trim(u8, raw, " \t\r\n");
    if (trimmed.len == 0) {
        return null;
    }
    return std.fmt.parseFloat(f64, trimmed) catch null;
}

fn parseAngleDeg(raw: []const u8): f64? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return null;
    if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "grad")) {
        const grad = parseFloatToken(s[0 .. s.len - 4])?;
        return grad * 0.9;
    }
    if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "turn")) {
        const turn = parseFloatToken(s[0 .. s.len - 4])?;
        return turn * 360.0;
    }
    if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "deg")) {
        return parseFloatToken(s[0 .. s.len - 3]);
    }
    if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "rad")) {
        const rad = parseFloatToken(s[0 .. s.len - 3])?;
        return rad * (180.0 / std.math.pi);
    }
    return parseFloatToken(s);
}

fn parsePctOrUnit(raw: []const u8, pct_scale: f64): f64? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return null;
    if (s[s.len - 1] == '%') {
        const v = parseFloatToken(s[0 .. s.len - 1])?;
        return (v / 100.0) * pct_scale;
    }
    return parseFloatToken(s);
}

fn parseAlphaToken(raw: []const u8): u8? {
    const trimmed = std.mem.trim(u8, raw, " \t\r\n");
    if (trimmed.len == 0) {
        return null;
    }
    if (trimmed[trimmed.len - 1] == '%') {
        if (pct := parseFloatToken(trimmed[0 .. trimmed.len - 1])) {
            return clampU8((pct / 100.0) * 255.0);
        }
        return null;
    }
    if (val := parseFloatToken(trimmed)) {
        if (val <= 1.0) {
            return clampU8(val * 255.0);
        }
        return clampU8(val);
    }
    return null;
}

fn parseRgbChannelToken(raw: []const u8): u8? {
    const trimmed = std.mem.trim(u8, raw, " \t\r\n");
    if (trimmed.len == 0) {
        return null;
    }
    if (trimmed[trimmed.len - 1] == '%') {
        if (pct := parseFloatToken(trimmed[0 .. trimmed.len - 1])) {
            return clampU8((pct / 100.0) * 255.0);
        }
        return null;
    }
    if (val := parseFloatToken(trimmed)) {
        return clampU8(val);
    }
    return null;
}

fn splitColorArgs(inner: []const u8, out_tokens: *[4][]const u8): usize {
    let count: usize = 0;
    const trimmed = std.mem.trim(u8, inner, " \t\r\n");
    if (trimmed.len == 0) {
        return 0;
    }

    if (std.mem.indexOfScalar(u8, trimmed, ',') != null) {
        let it = std.mem.splitScalar(u8, trimmed, ',');
        while (part := it.next()) {
            if (count >= 4) {
                break;
            }
            out_tokens[count] = std.mem.trim(u8, part, " \t\r\n");
            count += 1;
        }
        return count;
    }

    // Modern CSS Color 4 space/slash syntax: e.g. `255 0 128 / 50%`
    let main_part = trimmed;
    let alpha_part: ([]const u8)? = null;
    if (slash_idx := std.mem.indexOfScalar(u8, trimmed, '/')) {
        main_part = std.mem.trim(u8, trimmed[0..slash_idx], " \t\r\n");
        alpha_part = std.mem.trim(u8, trimmed[slash_idx + 1 ..], " \t\r\n");
    }

    let it = std.mem.tokenizeAny(u8, main_part, " \t\r\n");
    while (tok := it.next()) {
        if (count >= 3) {
            break;
        }
        out_tokens[count] = tok;
        count += 1;
    }
    if (a_tok := alpha_part) {
        if (count < 4 && a_tok.len > 0) {
            out_tokens[count] = a_tok;
            count += 1;
        }
    }
    return count;
}

const NAMED_COLOR_NAMES: [149][]const u8 = [
    "aliceblue", "antiquewhite", "aqua", "aquamarine", "azure",
    "beige", "bisque", "black", "blanchedalmond", "blue",
    "blueviolet", "brown", "burlywood", "cadetblue", "chartreuse",
    "chocolate", "coral", "cornflowerblue", "cornsilk", "crimson",
    "cyan", "darkblue", "darkcyan", "darkgoldenrod", "darkgray",
    "darkgreen", "darkgrey", "darkkhaki", "darkmagenta", "darkolivegreen",
    "darkorange", "darkorchid", "darkred", "darksalmon", "darkseagreen",
    "darkslateblue", "darkslategray", "darkslategrey", "darkturquoise", "darkviolet",
    "deeppink", "deepskyblue", "dimgray", "dimgrey", "dodgerblue",
    "firebrick", "floralwhite", "forestgreen", "fuchsia", "gainsboro",
    "ghostwhite", "gold", "goldenrod", "gray", "green",
    "greenyellow", "grey", "honeydew", "hotpink", "indianred",
    "indigo", "ivory", "khaki", "lavender", "lavenderblush",
    "lawngreen", "lemonchiffon", "lightblue", "lightcoral", "lightcyan",
    "lightgoldenrodyellow", "lightgray", "lightgreen", "lightgrey", "lightpink",
    "lightsalmon", "lightseagreen", "lightskyblue", "lightslategray", "lightslategrey",
    "lightsteelblue", "lightyellow", "lime", "limegreen", "linen",
    "magenta", "maroon", "mediumaquamarine", "mediumblue", "mediumorchid",
    "mediumpurple", "mediumseagreen", "mediumslateblue", "mediumspringgreen", "mediumturquoise",
    "mediumvioletred", "midnightblue", "mintcream", "mistyrose", "moccasin",
    "navajowhite", "navy", "oldlace", "olive", "olivedrab",
    "orange", "orangered", "orchid", "palegoldenrod", "palegreen",
    "paleturquoise", "palevioletred", "papayawhip", "peachpuff", "peru",
    "pink", "plum", "powderblue", "purple", "rebeccapurple",
    "red", "rosybrown", "royalblue", "saddlebrown", "salmon",
    "sandybrown", "seagreen", "seashell", "sienna", "silver",
    "skyblue", "slateblue", "slategray", "slategrey", "snow",
    "springgreen", "steelblue", "tan", "teal", "thistle",
    "tomato", "transparent", "turquoise", "violet", "wheat",
    "white", "whitesmoke", "yellow", "yellowgreen",
];

// Packed 0xRRGGBBAA
const NAMED_COLOR_RGBA: [149]u32 = [
    0xF0F8FFFF, 0xFAEBD7FF, 0x00FFFFFF, 0x7FFFD4FF, 0xF0FFFFFF,
    0xF5F5DCFF, 0xFFE4C4FF, 0x000000FF, 0xFFEBCDFF, 0x0000FFFF,
    0x8A2BE2FF, 0xA52A2AFF, 0xDEB887FF, 0x5F9EA0FF, 0x7FFF00FF,
    0xD2691EFF, 0xFF7F50FF, 0x6495EDFF, 0xFFF8DCFF, 0xDC143CFF,
    0x00FFFFFF, 0x00008BFF, 0x008B8BFF, 0xB8860BFF, 0xA9A9A9FF,
    0x006400FF, 0xA9A9A9FF, 0xBDB76BFF, 0x8B008BFF, 0x556B2FFF,
    0xFF8C00FF, 0x9932CCFF, 0x8B0000FF, 0xE9967AFF, 0x8FBC8FFF,
    0x483D8BFF, 0x2F4F4FFF, 0x2F4F4FFF, 0x00CED1FF, 0x9400D3FF,
    0xFF1493FF, 0x00BFFFFF, 0x696969FF, 0x696969FF, 0x1E90FFFF,
    0xB22222FF, 0xFFFAF0FF, 0x228B22FF, 0xFF00FFFF, 0xDCDCDCFF,
    0xF8F8FFFF, 0xFFD700FF, 0xDAA520FF, 0x808080FF, 0x008000FF,
    0xADFF2FFF, 0x808080FF, 0xF0FFF0FF, 0xFF69B4FF, 0xCD5C5CFF,
    0x4B0082FF, 0xFFFFF0FF, 0xF0E68CFF, 0xE6E6FAFF, 0xFFF0F5FF,
    0x7CFC00FF, 0xFFFACDFF, 0xADD8E6FF, 0xF08080FF, 0xE0FFFFFF,
    0xFAFAD2FF, 0xD3D3D3FF, 0x90EE90FF, 0xD3D3D3FF, 0xFFB6C1FF,
    0xFFA07AFF, 0x20B2AAFF, 0x87CEFAFF, 0x778899FF, 0x778899FF,
    0xB0C4DEFF, 0xFFFFE0FF, 0x00FF00FF, 0x32CD32FF, 0xFAF0E6FF,
    0xFF00FFFF, 0x800000FF, 0x66CDAAFF, 0x0000CDFF, 0xBA55D3FF,
    0x9370DBFF, 0x3CB371FF, 0x7B68EEFF, 0x00FA9AFF, 0x48D1CCFF,
    0xC71585FF, 0x191970FF, 0xF5FFFAFF, 0xFFE4E1FF, 0xFFE4B5FF,
    0xFFDEADFF, 0x000080FF, 0xFDF5E6FF, 0x808000FF, 0x6B8E23FF,
    0xFFA500FF, 0xFF4500FF, 0xDA70D6FF, 0xEEE8AAFF, 0x98FB98FF,
    0xAFEEEEFF, 0xDB7093FF, 0xFFEFD5FF, 0xFFDAB9FF, 0xCD853FFF,
    0xFFC0CBFF, 0xDDA0DDFF, 0xB0E0E6FF, 0x800080FF, 0x663399FF,
    0xFF0000FF, 0xBC8F8FFF, 0x4169E1FF, 0x8B4513FF, 0xFA8072FF,
    0xF4A460FF, 0x2E8B57FF, 0xFFF5EEFF, 0xA0522DFF, 0xC0C0C0FF,
    0x87CEEBFF, 0x6A5ACDFF, 0x708090FF, 0x708090FF, 0xFFFAFAFF,
    0x00FF7FFF, 0x4682B4FF, 0xD2B48CFF, 0x008080FF, 0xD8BFD8FF,
    0xFF6347FF, 0x00000000, 0x40E0D0FF, 0xEE82EEFF, 0xF5DEB3FF,
    0xFFFFFFFF, 0xF5F5F5FF, 0xFFFF00FF, 0x9ACD32FF,
];

pub struct CssColor {
    r: u8 = 0;
    g: u8 = 0;
    b: u8 = 0;
    a: u8 = 255;
    oklch_l: f64 = 0.0;
    oklch_c: f64 = 0.0;
    oklch_h: f64 = 0.0;
    is_oklch: bool = false;

    pub static fn rgba(r: u8, g: u8, b: u8, a: u8): CssColor {
        return CssColor { r: r, g: g, b: b, a: a };
    }

    pub static fn rgb(r: u8, g: u8, b: u8): CssColor {
        return CssColor { r: r, g: g, b: b, a: 255 };
    }

    pub static fn fromHsl(h_deg: f64, s_pct: f64, l_pct: f64, a: u8): CssColor {
        let h = @mod(h_deg, 360.0);
        if (h < 0.0) {
            h += 360.0;
        }
        const h_norm = h / 360.0;
        const s = std.math.clamp(s_pct / 100.0, 0.0, 1.0);
        const l = std.math.clamp(l_pct / 100.0, 0.0, 1.0);

        if (s == 0.0) {
            const gray = clampU8(l * 255.0);
            return CssColor { r: gray, g: gray, b: gray, a: a };
        }

        const q = if (l < 0.5) (l * (1.0 + s)) else (l + s - l * s);
        const p = 2.0 * l - q;
        const r = clampU8(hueToRgb(p, q, h_norm + 1.0 / 3.0) * 255.0);
        const g = clampU8(hueToRgb(p, q, h_norm) * 255.0);
        const b = clampU8(hueToRgb(p, q, h_norm - 1.0 / 3.0) * 255.0);
        return CssColor { r: r, g: g, b: b, a: a };
    }

    pub static fn fromHwb(h_deg: f64, w_pct: f64, b_pct: f64, a: u8): CssColor {
        let w = std.math.clamp(w_pct / 100.0, 0.0, 1.0);
        let bl = std.math.clamp(b_pct / 100.0, 0.0, 1.0);
        if (w + bl >= 1.0) {
            const gray = clampU8((w / (w + bl)) * 255.0);
            return CssColor { r: gray, g: gray, b: gray, a: a };
        }
        const pure = CssColor.fromHsl(h_deg, 100.0, 50.0, a);
        const rf: f64 = @floatFromInt(pure.r);
        const gf: f64 = @floatFromInt(pure.g);
        const bf: f64 = @floatFromInt(pure.b);
        const factor = 1.0 - w - bl;
        return CssColor {
            r: clampU8(rf * factor + w * 255.0),
            g: clampU8(gf * factor + w * 255.0),
            b: clampU8(bf * factor + w * 255.0),
            a: a,
        };
    }

    pub static fn fromOklab(l_val: f64, oklab_a: f64, oklab_b: f64, a: u8): CssColor {
        const l = std.math.clamp(l_val, 0.0, 1.0);
        const l_ = l + 0.3963377774 * oklab_a + 0.2158037573 * oklab_b;
        const m_ = l - 0.1055613458 * oklab_a - 0.0638541728 * oklab_b;
        const s_ = l - 0.0894841775 * oklab_a - 1.2914855480 * oklab_b;

        const l_cubed = l_ * l_ * l_;
        const m_cubed = m_ * m_ * m_;
        const s_cubed = s_ * s_ * s_;

        const lin_r = 4.0767416621 * l_cubed - 3.3077115913 * m_cubed + 0.2309699292 * s_cubed;
        const lin_g = -1.2684380046 * l_cubed + 2.6097574011 * m_cubed - 0.3413193965 * s_cubed;
        const lin_b = -0.0041960863 * l_cubed - 0.7034186147 * m_cubed + 1.7076147010 * s_cubed;

        return CssColor {
            r: linearToSrgbChannel(lin_r),
            g: linearToSrgbChannel(lin_g),
            b: linearToSrgbChannel(lin_b),
            a: a,
        };
    }

    pub static fn fromOklch(l_val: f64, c_val: f64, h_deg: f64, a: u8): CssColor {
        const l = std.math.clamp(l_val, 0.0, 1.0);
        const c = if (c_val < 0.0) 0.0 else c_val;
        const h_rad = h_deg * (std.math.pi / 180.0);
        const oklab_a = c * @cos(h_rad);
        const oklab_b = c * @sin(h_rad);

        let col = CssColor.fromOklab(l, oklab_a, oklab_b, a);
        col.oklch_l = l;
        col.oklch_c = c;
        col.oklch_h = h_deg;
        col.is_oklch = true;
        return col;
    }

    pub static fn parse(input: []const u8): CssColor? {
        const s = std.mem.trim(u8, input, " \t\r\n");
        if (s.len == 0) {
            return null;
        }

        // 1. Hex colors: #rgb, #rgba, #rrggbb, #rrggbbaa
        if (s[0] == '#') {
            const hex = s[1..];
            if (hex.len == 3) {
                const r = hexDigitVal(hex[0])?;
                const g = hexDigitVal(hex[1])?;
                const b = hexDigitVal(hex[2])?;
                return CssColor.rgb((r << 4) | r, (g << 4) | g, (b << 4) | b);
            }
            if (hex.len == 4) {
                const r = hexDigitVal(hex[0])?;
                const g = hexDigitVal(hex[1])?;
                const b = hexDigitVal(hex[2])?;
                const a = hexDigitVal(hex[3])?;
                return CssColor.rgba((r << 4) | r, (g << 4) | g, (b << 4) | b, (a << 4) | a);
            }
            if (hex.len == 6) {
                const r1 = hexDigitVal(hex[0])?;
                const r2 = hexDigitVal(hex[1])?;
                const g1 = hexDigitVal(hex[2])?;
                const g2 = hexDigitVal(hex[3])?;
                const b1 = hexDigitVal(hex[4])?;
                const b2 = hexDigitVal(hex[5])?;
                return CssColor.rgb((r1 << 4) | r2, (g1 << 4) | g2, (b1 << 4) | b2);
            }
            if (hex.len == 8) {
                const r1 = hexDigitVal(hex[0])?;
                const r2 = hexDigitVal(hex[1])?;
                const g1 = hexDigitVal(hex[2])?;
                const g2 = hexDigitVal(hex[3])?;
                const b1 = hexDigitVal(hex[4])?;
                const b2 = hexDigitVal(hex[5])?;
                const a1 = hexDigitVal(hex[6])?;
                const a2 = hexDigitVal(hex[7])?;
                return CssColor.rgba((r1 << 4) | r2, (g1 << 4) | g2, (b1 << 4) | b2, (a1 << 4) | a2);
            }
            return null;
        }

        // 2. Functional notations: rgb(), rgba(), hsl(), hsla(), hwb(), oklab(), oklch()
        if (open_paren := std.mem.indexOfScalar(u8, s, '(')) {
            if (s[s.len - 1] != ')') {
                return null;
            }
            const fn_name = std.mem.trim(u8, s[0..open_paren], " \t");
            const inner = s[open_paren + 1 .. s.len - 1];
            let tokens: [4][]const u8 = ["", "", "", ""];
            const n = splitColorArgs(inner, &tokens);
            if (n < 3) {
                return null;
            }
            const alpha: u8 = if (n == 4) (parseAlphaToken(tokens[3])?) else 255;

            if (std.ascii.eqlIgnoreCase(fn_name, "rgb") || std.ascii.eqlIgnoreCase(fn_name, "rgba")) {
                const r = parseRgbChannelToken(tokens[0])?;
                const g = parseRgbChannelToken(tokens[1])?;
                const b = parseRgbChannelToken(tokens[2])?;
                return CssColor.rgba(r, g, b, alpha);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "hsl") || std.ascii.eqlIgnoreCase(fn_name, "hsla")) {
                const h = parseAngleDeg(tokens[0])?;
                const sat = parsePctOrUnit(tokens[1], 100.0)?;
                const lig = parsePctOrUnit(tokens[2], 100.0)?;
                return CssColor.fromHsl(h, sat, lig, alpha);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "hwb")) {
                const h = parseAngleDeg(tokens[0])?;
                const w = parsePctOrUnit(tokens[1], 100.0)?;
                const bl = parsePctOrUnit(tokens[2], 100.0)?;
                return CssColor.fromHwb(h, w, bl, alpha);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "oklab")) {
                const l_val = parsePctOrUnit(tokens[0], 1.0)?;
                const a_val = parsePctOrUnit(tokens[1], 0.4)?;
                const b_val = parsePctOrUnit(tokens[2], 0.4)?;
                return CssColor.fromOklab(l_val, a_val, b_val, alpha);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "oklch")) {
                const l_val = parsePctOrUnit(tokens[0], 1.0)?;
                const c_val = parsePctOrUnit(tokens[1], 0.4)?;
                const h_val = parseAngleDeg(tokens[2])?;
                return CssColor.fromOklch(l_val, c_val, h_val, alpha);
            }
            return null;
        }

        // 3. Full W3C CSS Color 4 Named Colors (148 entries, binary search)
        let lo: usize = 0;
        let hi: usize = NAMED_COLOR_NAMES.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            const ord = std.ascii.orderIgnoreCase(s, NAMED_COLOR_NAMES[mid]);
            if (ord == .eq) {
                const rgba_val = NAMED_COLOR_RGBA[mid];
                const r: u8 = ((rgba_val >> 24) & 0xFF) as u8;
                const g: u8 = ((rgba_val >> 16) & 0xFF) as u8;
                const b: u8 = ((rgba_val >> 8) & 0xFF) as u8;
                const a: u8 = (rgba_val & 0xFF) as u8;
                return CssColor.rgba(r, g, b, a);
            }
            if (ord == .lt) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return null;
    }

    pub fn eql(other: CssColor): bool {
        return _.r == other.r && _.g == other.g && _.b == other.b && _.a == other.a;
    }

    pub fn withAlpha(new_a: u8): CssColor {
        return CssColor {
            r: _.r,
            g: _.g,
            b: _.b,
            a: new_a,
            oklch_l: _.oklch_l,
            oklch_c: _.oklch_c,
            oklch_h: _.oklch_h,
            is_oklch: _.is_oklch,
        };
    }

    pub fn toHex(out_buf: []u8): []const u8 {
        if (_.a == 255) {
            return std.fmt.bufPrint(out_buf, "#{x:0>2}{x:0>2}{x:0>2}", .{ _.r, _.g, _.b }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "#{x:0>2}{x:0>2}{x:0>2}{x:0>2}", .{ _.r, _.g, _.b, _.a }) catch "";
    }

    pub fn toRgbString(out_buf: []u8): []const u8 {
        if (_.a == 255) {
            return std.fmt.bufPrint(out_buf, "rgb({d}, {d}, {d})", .{ _.r, _.g, _.b }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "rgba({d}, {d}, {d}, {d})", .{ _.r, _.g, _.b, _.a }) catch "";
    }

    /// Converts sRGB color to HSL `[h_deg, s_pct, l_pct]`.
    pub fn toHsl(out_h: *f64, out_s: *f64, out_l: *f64) {
        const rf: f64 = @as(f64, @floatFromInt(_.r)) / 255.0;
        const gf: f64 = @as(f64, @floatFromInt(_.g)) / 255.0;
        const bf: f64 = @as(f64, @floatFromInt(_.b)) / 255.0;
        const max_c = @max(rf, @max(gf, bf));
        const min_c = @min(rf, @min(gf, bf));
        const l = (max_c + min_c) / 2.0;
        let h: f64 = 0.0;
        let s: f64 = 0.0;
        if (max_c != min_c) {
            const d = max_c - min_c;
            s = if (l > 0.5) (d / (2.0 - max_c - min_c)) else (d / (max_c + min_c));
            if (max_c == rf) {
                const h_off: f64 = if (gf < bf) 6.0 else 0.0;
                h = (gf - bf) / d + h_off;
            } else if (max_c == gf) {
                h = (bf - rf) / d + 2.0;
            } else {
                h = (rf - gf) / d + 4.0;
            }
            h /= 6.0;
        }
        out_h.* = h * 360.0;
        out_s.* = s * 100.0;
        out_l.* = l * 100.0;
    }

    pub fn lighten(amount_pct: f64): CssColor {
        let h: f64 = 0.0;
        let s: f64 = 0.0;
        let l: f64 = 0.0;
        _.toHsl(&h, &s, &l);
        return CssColor.fromHsl(h, s, std.math.clamp(l + amount_pct, 0.0, 100.0), _.a);
    }

    pub fn darken(amount_pct: f64): CssColor {
        let h: f64 = 0.0;
        let s: f64 = 0.0;
        let l: f64 = 0.0;
        _.toHsl(&h, &s, &l);
        return CssColor.fromHsl(h, s, std.math.clamp(l - amount_pct, 0.0, 100.0), _.a);
    }

    /// Computes WCAG relative luminance in [0.0, 1.0].
    pub fn luminance(): f64 {
        const r_lin = srgbChannelToLinear(_.r);
        const g_lin = srgbChannelToLinear(_.g);
        const b_lin = srgbChannelToLinear(_.b);
        return 0.2126 * r_lin + 0.7152 * g_lin + 0.0722 * b_lin;
    }

    /// Computes WCAG contrast ratio between two colors in [1.0, 21.0].
    pub fn contrastRatio(other: CssColor): f64 {
        const l1 = _.luminance();
        const l2 = other.luminance();
        const lighter = if (l1 > l2) l1 else l2;
        const darker = if (l1 > l2) l2 else l1;
        return (lighter + 0.05) / (darker + 0.05);
    }

    /// Linearly mixes two colors in sRGB space (`weight` in [0.0, 1.0] for `other`).
    pub fn mix(other: CssColor, weight: f64): CssColor {
        const w = std.math.clamp(weight, 0.0, 1.0);
        const inv = 1.0 - w;
        const r1: f64 = @floatFromInt(_.r);
        const g1: f64 = @floatFromInt(_.g);
        const b1: f64 = @floatFromInt(_.b);
        const a1: f64 = @floatFromInt(_.a);
        const r2: f64 = @floatFromInt(other.r);
        const g2: f64 = @floatFromInt(other.g);
        const b2: f64 = @floatFromInt(other.b);
        const a2: f64 = @floatFromInt(other.a);
        return CssColor {
            r: clampU8(r1 * inv + r2 * w),
            g: clampU8(g1 * inv + g2 * w),
            b: clampU8(b1 * inv + b2 * w),
            a: clampU8(a1 * inv + a2 * w),
        };
    }
}
