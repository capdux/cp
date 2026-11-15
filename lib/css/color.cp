// lib/css/color.cp
// CSS Color Level 4 Parser & Color Space Converter (Hex, RGB/RGBA, HSL/HSLA, OKLCH, Named Colors)
// Absorbed and unified from Bun src/css/values/color.cp & color_generated.cp

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

    pub static fn fromOklch(l_val: f64, c_val: f64, h_deg: f64, a: u8): CssColor {
        const l = std.math.clamp(l_val, 0.0, 1.0);
        const c = if (c_val < 0.0) 0.0 else c_val;
        const h_rad = h_deg * (std.math.pi / 180.0);
        const oklab_a = c * @cos(h_rad);
        const oklab_b = c * @sin(h_rad);

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
            oklch_l: l,
            oklch_c: c,
            oklch_h: h_deg,
            is_oklch: true,
        };
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

        // 2. Functional notations: rgb(), rgba(), hsl(), hsla(), oklch()
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
                let h_tok = tokens[0];
                if (std.mem.endsWith(u8, h_tok, "deg")) {
                    h_tok = h_tok[0 .. h_tok.len - 3];
                }
                const h = parseFloatToken(h_tok)?;
                const s_raw = tokens[1];
                const l_raw = tokens[2];
                const s_str = if (std.mem.endsWith(u8, s_raw, "%")) s_raw[0 .. s_raw.len - 1] else s_raw;
                const l_str = if (std.mem.endsWith(u8, l_raw, "%")) l_raw[0 .. l_raw.len - 1] else l_raw;
                const sat = parseFloatToken(s_str)?;
                const lig = parseFloatToken(l_str)?;
                return CssColor.fromHsl(h, sat, lig, alpha);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "oklch")) {
                const l_raw = tokens[0];
                let l_val: f64 = 0.0;
                if (std.mem.endsWith(u8, l_raw, "%")) {
                    l_val = (parseFloatToken(l_raw[0 .. l_raw.len - 1])?) / 100.0;
                } else {
                    l_val = parseFloatToken(l_raw)?;
                }
                const c_val = parseFloatToken(tokens[1])?;
                let h_tok = tokens[2];
                if (std.mem.endsWith(u8, h_tok, "deg")) {
                    h_tok = h_tok[0 .. h_tok.len - 3];
                }
                const h_val = parseFloatToken(h_tok)?;
                return CssColor.fromOklch(l_val, c_val, h_val, alpha);
            }
            return null;
        }

        // 3. Standard CSS Named Colors
        if (std.ascii.eqlIgnoreCase(s, "transparent")) return CssColor.rgba(0, 0, 0, 0);
        if (std.ascii.eqlIgnoreCase(s, "black")) return CssColor.rgb(0, 0, 0);
        if (std.ascii.eqlIgnoreCase(s, "white")) return CssColor.rgb(255, 255, 255);
        if (std.ascii.eqlIgnoreCase(s, "red")) return CssColor.rgb(255, 0, 0);
        if (std.ascii.eqlIgnoreCase(s, "green")) return CssColor.rgb(0, 128, 0);
        if (std.ascii.eqlIgnoreCase(s, "lime")) return CssColor.rgb(0, 255, 0);
        if (std.ascii.eqlIgnoreCase(s, "blue")) return CssColor.rgb(0, 0, 255);
        if (std.ascii.eqlIgnoreCase(s, "yellow")) return CssColor.rgb(255, 255, 0);
        if (std.ascii.eqlIgnoreCase(s, "cyan") || std.ascii.eqlIgnoreCase(s, "aqua")) return CssColor.rgb(0, 255, 255);
        if (std.ascii.eqlIgnoreCase(s, "magenta") || std.ascii.eqlIgnoreCase(s, "fuchsia")) return CssColor.rgb(255, 0, 255);
        if (std.ascii.eqlIgnoreCase(s, "orange")) return CssColor.rgb(255, 165, 0);
        if (std.ascii.eqlIgnoreCase(s, "purple")) return CssColor.rgb(128, 0, 128);
        if (std.ascii.eqlIgnoreCase(s, "rebeccapurple")) return CssColor.rgb(102, 51, 153);
        if (std.ascii.eqlIgnoreCase(s, "cornflowerblue")) return CssColor.rgb(100, 149, 237);
        if (std.ascii.eqlIgnoreCase(s, "gold")) return CssColor.rgb(255, 215, 0);
        if (std.ascii.eqlIgnoreCase(s, "coral")) return CssColor.rgb(255, 127, 80);
        if (std.ascii.eqlIgnoreCase(s, "gray") || std.ascii.eqlIgnoreCase(s, "grey")) return CssColor.rgb(128, 128, 128);
        if (std.ascii.eqlIgnoreCase(s, "silver")) return CssColor.rgb(192, 192, 192);
        if (std.ascii.eqlIgnoreCase(s, "teal")) return CssColor.rgb(0, 128, 128);
        if (std.ascii.eqlIgnoreCase(s, "navy")) return CssColor.rgb(0, 0, 128);
        return null;
    }

    pub fn eql(other: CssColor): bool {
        return _.r == other.r && _.g == other.g && _.b == other.b && _.a == other.a;
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
