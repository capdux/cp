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

const CalcUnit = enum {
    none,
    percent,
    deg,
};

const CalcVal = struct {
    val: f64,
    unit: CalcUnit,
};

const ColorComp = struct {
    val: f64,
    is_none: bool,
};

fn stripCssComments(input: []const u8, buf: []u8): ([]const u8)? {
    if (std.mem.indexOf(u8, input, "/*") == null) {
        return std.mem.trim(u8, input, " \t\r\n");
    }
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (i + 1 < input.len && input[i] == '/' && input[i + 1] == '*') {
            let j = i + 2;
            let closed = false;
            while (j + 1 < input.len) {
                if (input[j] == '*' && input[j + 1] == '/') {
                    closed = true;
                    j += 2;
                    break;
                }
                j += 1;
            }
            if (!closed) {
                return null;
            }
            if (out_len < buf.len) {
                buf[out_len] = ' ';
                out_len += 1;
            }
            i = j;
        } else {
            if (out_len >= buf.len) {
                return null;
            }
            buf[out_len] = input[i];
            out_len += 1;
            i += 1;
        }
    }
    return std.mem.trim(u8, buf[0..out_len], " \t\r\n");
}

const CalcParser = struct {
    src: []const u8,
    pos: usize = 0,

    fn skipWs(*_) {
        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t' || _.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
            _.pos += 1;
        }
    }

    fn parseExpr(*_): CalcVal? {
        let lhs = _.parseTerm() ?? return null;
        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len) break;
            const op = _.src[_.pos];
            if (op != '+' && op != '-') break;
            _.pos += 1;
            const rhs = _.parseTerm() ?? return null;
            if (lhs.unit != rhs.unit) {
                if (lhs.unit == .none && lhs.val == 0.0) {
                    lhs.unit = rhs.unit;
                } else if (rhs.unit == .none && rhs.val == 0.0) {
                    // keep lhs.unit
                } else {
                    return null;
                }
            }
            if (op == '+') {
                lhs.val += rhs.val;
            } else {
                lhs.val -= rhs.val;
            }
        }
        return lhs;
    }

    fn parseTerm(*_): CalcVal? {
        let lhs = _.parseFactor() ?? return null;
        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len) break;
            const op = _.src[_.pos];
            if (op != '*' && op != '/') break;
            _.pos += 1;
            const rhs = _.parseFactor() ?? return null;
            if (op == '*') {
                if (lhs.unit == .none) {
                    lhs = CalcVal { val: lhs.val * rhs.val, unit: rhs.unit };
                } else if (rhs.unit == .none) {
                    lhs.val *= rhs.val;
                } else {
                    return null;
                }
            } else {
                if (rhs.unit != .none || rhs.val == 0.0) {
                    return null;
                }
                lhs.val /= rhs.val;
            }
        }
        return lhs;
    }

    fn parseFactor(*_): CalcVal? {
        _.skipWs();
        if (_.pos >= _.src.len) return null;
        if (_.src[_.pos] == '+') {
            _.pos += 1;
            return _.parseFactor();
        }
        if (_.src[_.pos] == '-') {
            _.pos += 1;
            let v = _.parseFactor() ?? return null;
            v.val = -v.val;
            return v;
        }
        if (_.src[_.pos] == '(') {
            _.pos += 1;
            const v = _.parseExpr() ?? return null;
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != ')') return null;
            _.pos += 1;
            return v;
        }
        // Check calc( nested
        if (_.pos + 5 <= _.src.len && std.ascii.eqlIgnoreCase(_.src[_.pos .. _.pos + 5], "calc(")) {
            _.pos += 5;
            const v = _.parseExpr() ?? return null;
            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] != ')') return null;
            _.pos += 1;
            return v;
        }
        const start = _.pos;
        let has_digit = false;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if ((c >= '0' && c <= '9') || c == '.') {
                if (c >= '0' && c <= '9') has_digit = true;
                _.pos += 1;
            } else if ((c == 'e' || c == 'E') && has_digit && _.pos + 1 < _.src.len && (_.src[_.pos + 1] == '+' || _.src[_.pos + 1] == '-' || (_.src[_.pos + 1] >= '0' && _.src[_.pos + 1] <= '9'))) {
                _.pos += 1;
                if (_.src[_.pos] == '+' || _.src[_.pos] == '-') _.pos += 1;
            } else {
                break;
            }
        }
        if (has_digit) {
            const num = std.fmt.parseFloat(f64, _.src[start.._.pos]) catch return null;
            if (_.pos < _.src.len && _.src[_.pos] == '%') {
                _.pos += 1;
                return CalcVal { val: num, unit: .percent };
            }
            const u_start = _.pos;
            while (_.pos < _.src.len && ((_.src[_.pos] >= 'a' && _.src[_.pos] <= 'z') || (_.src[_.pos] >= 'A' && _.src[_.pos] <= 'Z'))) {
                _.pos += 1;
            }
            if (_.pos == u_start) {
                return CalcVal { val: num, unit: .none };
            }
            const unit_s = _.src[u_start.._.pos];
            if (std.ascii.eqlIgnoreCase(unit_s, "deg")) return CalcVal { val: num, unit: .deg };
            if (std.ascii.eqlIgnoreCase(unit_s, "grad")) return CalcVal { val: num * 0.9, unit: .deg };
            if (std.ascii.eqlIgnoreCase(unit_s, "turn")) return CalcVal { val: num * 360.0, unit: .deg };
            if (std.ascii.eqlIgnoreCase(unit_s, "rad")) return CalcVal { val: num * (180.0 / std.math.pi), unit: .deg };
            return null;
        }
        const ident_start = _.pos;
        while (_.pos < _.src.len && ((_.src[_.pos] >= 'a' && _.src[_.pos] <= 'z') || (_.src[_.pos] >= 'A' && _.src[_.pos] <= 'Z'))) {
            _.pos += 1;
        }
        if (_.pos == ident_start) return null;
        const ident = _.src[ident_start.._.pos];
        if (std.ascii.eqlIgnoreCase(ident, "pi")) return CalcVal { val: std.math.pi, unit: .none };
        if (std.ascii.eqlIgnoreCase(ident, "e")) return CalcVal { val: std.math.e, unit: .none };
        if (std.ascii.eqlIgnoreCase(ident, "infinity")) return CalcVal { val: std.math.inf(f64), unit: .none };
        if (std.ascii.eqlIgnoreCase(ident, "nan")) return CalcVal { val: std.math.nan(f64), unit: .none };
        return null;
    }
};

fn evalCalcToken(raw: []const u8): CalcVal? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len < 6) return null;
    if (!std.ascii.eqlIgnoreCase(s[0..5], "calc(") || s[s.len - 1] != ')') return null;
    let parser = CalcParser { src: s[5 .. s.len - 1] };
    const res = parser.parseExpr() ?? return null;
    parser.skipWs();
    if (parser.pos != parser.src.len) return null;
    if (std.math.isNan(res.val)) return null;
    return res;
}

fn parseFloatToken(raw: []const u8): f64? {
    const trimmed = std.mem.trim(u8, raw, " \t\r\n");
    if (trimmed.len == 0) {
        return null;
    }
    if (cv := evalCalcToken(trimmed)) {
        if (cv.unit != .none) return null;
        return cv.val;
    }
    return std.fmt.parseFloat(f64, trimmed) catch null;
}

fn parseAngleOrNone(raw: []const u8): ColorComp? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return null;
    if (std.ascii.eqlIgnoreCase(s, "none")) return ColorComp { val: 0.0, is_none: true };
    if (cv := evalCalcToken(s)) {
        if (cv.unit == .percent) return null;
        return ColorComp { val: cv.val, is_none: false };
    }
    if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "grad")) {
        const grad = parseFloatToken(s[0 .. s.len - 4])?;
        return ColorComp { val: grad * 0.9, is_none: false };
    }
    if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "turn")) {
        const turn = parseFloatToken(s[0 .. s.len - 4])?;
        return ColorComp { val: turn * 360.0, is_none: false };
    }
    if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "deg")) {
        const d = parseFloatToken(s[0 .. s.len - 3])?;
        return ColorComp { val: d, is_none: false };
    }
    if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "rad")) {
        const rad = parseFloatToken(s[0 .. s.len - 3])?;
        return ColorComp { val: rad * (180.0 / std.math.pi), is_none: false };
    }
    const v = parseFloatToken(s)?;
    return ColorComp { val: v, is_none: false };
}

fn parseAngleDeg(raw: []const u8): f64? {
    const c = parseAngleOrNone(raw)?;
    return c.val;
}

fn parseCompOrNone(raw: []const u8, pct_scale: f64): ColorComp? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return null;
    if (std.ascii.eqlIgnoreCase(s, "none")) return ColorComp { val: 0.0, is_none: true };
    if (cv := evalCalcToken(s)) {
        if (cv.unit == .deg) return null;
        if (cv.unit == .percent) {
            return ColorComp { val: (cv.val / 100.0) * pct_scale, is_none: false };
        }
        return ColorComp { val: cv.val, is_none: false };
    }
    if (s[s.len - 1] == '%') {
        const v = parseFloatToken(s[0 .. s.len - 1])?;
        return ColorComp { val: (v / 100.0) * pct_scale, is_none: false };
    }
    const v = parseFloatToken(s)?;
    return ColorComp { val: v, is_none: false };
}

fn parsePctOrUnit(raw: []const u8, pct_scale: f64): f64? {
    const c = parseCompOrNone(raw, pct_scale)?;
    return c.val;
}

fn parseAlphaOrNone(raw: []const u8): ColorComp? {
    const trimmed = std.mem.trim(u8, raw, " \t\r\n");
    if (trimmed.len == 0) {
        return null;
    }
    if (std.ascii.eqlIgnoreCase(trimmed, "none")) {
        return ColorComp { val: 0.0, is_none: true };
    }
    if (cv := evalCalcToken(trimmed)) {
        if (cv.unit == .deg) return null;
        const v = if (cv.unit == .percent) (cv.val / 100.0) else cv.val;
        return ColorComp { val: std.math.clamp(v, 0.0, 1.0), is_none: false };
    }
    if (trimmed[trimmed.len - 1] == '%') {
        if (pct := parseFloatToken(trimmed[0 .. trimmed.len - 1])) {
            return ColorComp { val: std.math.clamp(pct / 100.0, 0.0, 1.0), is_none: false };
        }
        return null;
    }
    if (val := parseFloatToken(trimmed)) {
        return ColorComp { val: std.math.clamp(val, 0.0, 1.0), is_none: false };
    }
    return null;
}

fn parseAlphaToken(raw: []const u8): u8? {
    const c = parseAlphaOrNone(raw)?;
    return clampU8(c.val * 255.0);
}

fn parseRgbChannelOrNone(raw: []const u8): ColorComp? {
    const trimmed = std.mem.trim(u8, raw, " \t\r\n");
    if (trimmed.len == 0) {
        return null;
    }
    if (std.ascii.eqlIgnoreCase(trimmed, "none")) {
        return ColorComp { val: 0.0, is_none: true };
    }
    if (cv := evalCalcToken(trimmed)) {
        if (cv.unit == .deg) return null;
        if (cv.unit == .percent) {
            return ColorComp { val: (cv.val / 100.0) * 255.0, is_none: false };
        }
        return ColorComp { val: cv.val, is_none: false };
    }
    if (trimmed[trimmed.len - 1] == '%') {
        if (pct := parseFloatToken(trimmed[0 .. trimmed.len - 1])) {
            return ColorComp { val: (pct / 100.0) * 255.0, is_none: false };
        }
        return null;
    }
    if (val := parseFloatToken(trimmed)) {
        return ColorComp { val: val, is_none: false };
    }
    return null;
}

fn parseRgbChannelToken(raw: []const u8): u8? {
    const c = parseRgbChannelOrNone(raw)?;
    return clampU8(c.val);
}

fn splitColorArgsEx(inner: []const u8, out_tokens: *[4][]const u8, out_is_comma: *bool): usize {
    let count: usize = 0;
    const trimmed = std.mem.trim(u8, inner, " \t\r\n");
    out_is_comma.* = false;
    if (trimmed.len == 0) {
        return 0;
    }

    // Check for comma at paren depth 0
    let has_top_comma = false;
    let depth: usize = 0;
    for (ch of trimmed) {
        if (ch == '(') {
            depth += 1;
        } else if (ch == ')') {
            if (depth > 0) depth -= 1;
        } else if (ch == ',' && depth == 0) {
            has_top_comma = true;
            break;
        }
    }

    if (has_top_comma) {
        out_is_comma.* = true;
        let start: usize = 0;
        let i: usize = 0;
        depth = 0;
        while (i <= trimmed.len) {
            const at_end = i == trimmed.len;
            const ch = if (at_end) @as(u8, ',') else trimmed[i];
            if (!at_end && ch == '(') {
                depth += 1;
            } else if (!at_end && ch == ')') {
                if (depth > 0) depth -= 1;
            } else if (ch == ',' && depth == 0) {
                const seg = std.mem.trim(u8, trimmed[start..i], " \t\r\n");
                if (seg.len == 0 || count >= 4) {
                    return 0;
                }
                out_tokens[count] = seg;
                count += 1;
                start = i + 1;
            }
            i += 1;
        }
        return count;
    }

    // Modern CSS Color 4 space/slash syntax: e.g. `255 0 128 / 50%`
    let slash_idx: usize? = null;
    depth = 0;
    let si: usize = 0;
    while (si < trimmed.len) : (si += 1) {
        const ch = trimmed[si];
        if (ch == '(') {
            depth += 1;
        } else if (ch == ')') {
            if (depth > 0) depth -= 1;
        } else if (ch == '/' && depth == 0) {
            if (slash_idx != null) return 0;
            slash_idx = si;
        }
    }

    let main_part = trimmed;
    let alpha_part: ([]const u8)? = null;
    if (sidx := slash_idx) {
        main_part = std.mem.trim(u8, trimmed[0..sidx], " \t\r\n");
        const ap = std.mem.trim(u8, trimmed[sidx + 1 ..], " \t\r\n");
        if (ap.len == 0) return 0;
        alpha_part = ap;
    }

    // Tokenize main_part by whitespace at paren depth 0
    let ti: usize = 0;
    depth = 0;
    while (ti < main_part.len) {
        while (ti < main_part.len && (main_part[ti] == ' ' || main_part[ti] == '\t' || main_part[ti] == '\r' || main_part[ti] == '\n')) {
            ti += 1;
        }
        if (ti >= main_part.len) break;
        const tok_start = ti;
        while (ti < main_part.len) {
            const c = main_part[ti];
            if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                if (depth > 0) depth -= 1;
            } else if (depth == 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                break;
            }
            ti += 1;
        }
        if (count >= 3) return 0;
        out_tokens[count] = main_part[tok_start..ti];
        count += 1;
    }
    if (count != 3) return 0;

    if (a_tok := alpha_part) {
        // Ensure alpha_part is a single token (no top-level whitespace)
        let ad: usize = 0;
        for (c of a_tok) {
            if (c == '(') {
                ad += 1;
            } else if (c == ')') {
                if (ad > 0) ad -= 1;
            } else if (ad == 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                return 0;
            }
        }
        out_tokens[3] = a_tok;
        count = 4;
    }
    return count;
}

fn splitColorArgs(inner: []const u8, out_tokens: *[4][]const u8): usize {
    let is_comma = false;
    return splitColorArgsEx(inner, out_tokens, &is_comma);
}

fn formatCssNum(buf: []u8, val_in: f64): []const u8 {
    let val = val_in;
    if (@abs(val) < 0.0000005) {
        val = 0.0;
    }
    let s = std.fmt.bufPrint(buf, "{d:.6}", .{val}) catch "0";
    if (std.mem.indexOfScalar(u8, s, '.') != null) {
        while (s.len > 1 && s[s.len - 1] == '0') {
            s = s[0 .. s.len - 1];
        }
        if (s.len > 1 && s[s.len - 1] == '.') {
            s = s[0 .. s.len - 1];
        }
    }
    if (std.mem.eql(u8, s, "-0")) {
        return "0";
    }
    return s;
}

fn formatComp(buf: []u8, c: ColorComp): []const u8 {
    if (c.is_none) return "none";
    return formatCssNum(buf, c.val);
}

fn normalizeHueDeg(h_deg: f64): f64 {
    let h = @mod(h_deg, 360.0);
    if (h < 0.0) h += 360.0;
    if (@abs(h - 360.0) < 0.0000005) h = 0.0;
    return h;
}

fn formatRgbaFloatAlpha(out_buf: []u8, r: u8, g: u8, b: u8, alpha_01: f64): []const u8 {
    if (alpha_01 >= 1.0 - 0.0000005) {
        return std.fmt.bufPrint(out_buf, "rgb({d}, {d}, {d})", .{ r, g, b }) catch "";
    }
    if (alpha_01 <= 0.0000005) {
        return std.fmt.bufPrint(out_buf, "rgba({d}, {d}, {d}, 0)", .{ r, g, b }) catch "";
    }
    let abuf: [32]u8 = undefined;
    const astr = formatCssNum(&abuf, alpha_01);
    return std.fmt.bufPrint(out_buf, "rgba({d}, {d}, {d}, {s})", .{ r, g, b, astr }) catch "";
}

fn formatColorSpace3(out_buf: []u8, prefix: []const u8, c0: ColorComp, c1: ColorComp, c2: ColorComp, alpha: ColorComp): usize {
    let b0: [32]u8 = undefined;
    let b1: [32]u8 = undefined;
    let b2: [32]u8 = undefined;
    let ba: [32]u8 = undefined;
    const s0 = formatComp(&b0, c0);
    const s1 = formatComp(&b1, c1);
    const s2 = formatComp(&b2, c2);
    if (!alpha.is_none && alpha.val >= 1.0 - 0.0000005) {
        const res = std.fmt.bufPrint(out_buf, "{s}({s} {s} {s})", .{ prefix, s0, s1, s2 }) catch return 0;
        return res.len;
    }
    const sa = formatComp(&ba, alpha);
    const res = std.fmt.bufPrint(out_buf, "{s}({s} {s} {s} / {s})", .{ prefix, s0, s1, s2, sa }) catch return 0;
    return res.len;
}

fn formatColorSpace4(out_buf: []u8, space: []const u8, c0: ColorComp, c1: ColorComp, c2: ColorComp, alpha: ColorComp): usize {
    let b0: [32]u8 = undefined;
    let b1: [32]u8 = undefined;
    let b2: [32]u8 = undefined;
    let ba: [32]u8 = undefined;
    const s0 = formatComp(&b0, c0);
    const s1 = formatComp(&b1, c1);
    const s2 = formatComp(&b2, c2);
    if (!alpha.is_none && alpha.val >= 1.0 - 0.0000005) {
        const res = std.fmt.bufPrint(out_buf, "color({s} {s} {s} {s})", .{ space, s0, s1, s2 }) catch return 0;
        return res.len;
    }
    const sa = formatComp(&ba, alpha);
    const res = std.fmt.bufPrint(out_buf, "color({s} {s} {s} {s} / {s})", .{ space, s0, s1, s2, sa }) catch return 0;
    return res.len;
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
        let h = @mod(h_deg, 360.0);
        if (h < 0.0) {
            h += 360.0;
        }
        const h_norm = h / 360.0;
        const rf = hueToRgb(0.0, 1.0, h_norm + 1.0 / 3.0) * 255.0;
        const gf = hueToRgb(0.0, 1.0, h_norm) * 255.0;
        const bf = hueToRgb(0.0, 1.0, h_norm - 1.0 / 3.0) * 255.0;
        const factor = 1.0 - w - bl;
        return CssColor {
            r: clampU8(rf * factor + w * 255.0),
            g: clampU8(gf * factor + w * 255.0),
            b: clampU8(bf * factor + w * 255.0),
            a: a,
        };
    }

    pub static fn fromOklab(l_val: f64, oklab_a: f64, oklab_b: f64, a: u8): CssColor {
        if (l_val <= 0.00001) return CssColor.rgba(0, 0, 0, a);
        if (l_val >= 0.99999) return CssColor.rgba(255, 255, 255, a);
        const l = l_val;
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

    static fn xyzD65ToCssColor(x: f64, y: f64, z: f64, a: u8): CssColor {
        const lin_r = 3.2409699419045213 * x - 1.5373831775700935 * y - 0.4986107602930033 * z;
        const lin_g = -0.9692436362808798 * x + 1.8759675015077206 * y + 0.04155505740717561 * z;
        const lin_b = 0.05563007969699361 * x - 0.20397695888897657 * y + 1.0569715142428786 * z;
        return CssColor {
            r: linearToSrgbChannel(lin_r),
            g: linearToSrgbChannel(lin_g),
            b: linearToSrgbChannel(lin_b),
            a: a,
        };
    }

    static fn xyzD50ToCssColor(x50: f64, y50: f64, z50: f64, a: u8): CssColor {
        const x65 = 0.9554734527042182 * x50 - 0.023098536874261423 * y50 + 0.0632593086610217 * z50;
        const y65 = -0.028369706963208136 * x50 + 1.0099954580058226 * y50 + 0.021041398966943008 * z50;
        const z65 = 0.012314001688319899 * x50 - 0.020507696433477912 * y50 + 1.3303659366080753 * z50;
        return CssColor.xyzD65ToCssColor(x65, y65, z65, a);
    }

    pub static fn fromLab(l_val: f64, a_val: f64, b_val: f64, a: u8): CssColor {
        if (l_val <= 0.001) return CssColor.rgba(0, 0, 0, a);
        if (l_val >= 99.999) return CssColor.rgba(255, 255, 255, a);
        const l = l_val;
        const kappa: f64 = 24389.0 / 27.0;
        const epsilon: f64 = 216.0 / 24389.0;
        const fy = (l + 16.0) / 116.0;
        const fx = a_val / 500.0 + fy;
        const fz = fy - b_val / 200.0;
        const fx3 = fx * fx * fx;
        const fz3 = fz * fz * fz;
        const xr = if (fx3 > epsilon) fx3 else ((116.0 * fx - 16.0) / kappa);
        const yr = if (l > kappa * epsilon) std.math.pow(f64, fy, 3.0) else (l / kappa);
        const zr = if (fz3 > epsilon) fz3 else ((116.0 * fz - 16.0) / kappa);
        return CssColor.xyzD50ToCssColor(xr * 0.3457 / 0.3585, yr * 1.0, zr * (1.0 - 0.3457 - 0.3585) / 0.3585, a);
    }

    pub static fn fromLch(l_val: f64, c_val: f64, h_deg: f64, a: u8): CssColor {
        const c = if (c_val < 0.0) 0.0 else c_val;
        const h_rad = h_deg * (std.math.pi / 180.0);
        return CssColor.fromLab(l_val, c * @cos(h_rad), c * @sin(h_rad), a);
    }

    static fn srgbToLinearChannel(v: f64): f64 {
        const sign: f64 = if (v < 0.0) -1.0 else 1.0;
        const av = @abs(v);
        if (av <= 0.04045) return v / 12.92;
        return sign * std.math.pow(f64, (av + 0.055) / 1.055, 2.4);
    }

    static fn a98ToLinear(v: f64): f64 {
        const sign: f64 = if (v < 0.0) -1.0 else 1.0;
        return sign * std.math.pow(f64, @abs(v), 563.0 / 256.0);
    }

    static fn prophotoToLinear(v: f64): f64 {
        const sign: f64 = if (v < 0.0) -1.0 else 1.0;
        const av = @abs(v);
        if (av <= 16.0 / 512.0) return v / 16.0;
        return sign * std.math.pow(f64, av, 1.8);
    }

    static fn rec2020ToLinear(v: f64): f64 {
        const sign: f64 = if (v < 0.0) -1.0 else 1.0;
        const av = @abs(v);
        const alpha: f64 = 1.09929682680944;
        const beta: f64 = 0.018053968510807;
        if (av < beta * 4.5) return v / 4.5;
        return sign * std.math.pow(f64, (av + alpha - 1.0) / alpha, 1.0 / 0.45);
    }

    pub static fn fromColorSpace(space: []const u8, c0: f64, c1: f64, c2: f64, a: u8): CssColor? {
        if (std.ascii.eqlIgnoreCase(space, "srgb")) {
            return CssColor {
                r: clampU8(c0 * 255.0),
                g: clampU8(c1 * 255.0),
                b: clampU8(c2 * 255.0),
                a: a,
            };
        }
        if (std.ascii.eqlIgnoreCase(space, "srgb-linear")) {
            return CssColor {
                r: linearToSrgbChannel(c0),
                g: linearToSrgbChannel(c1),
                b: linearToSrgbChannel(c2),
                a: a,
            };
        }
        if (std.ascii.eqlIgnoreCase(space, "display-p3")) {
            const lr = CssColor.srgbToLinearChannel(c0);
            const lg = CssColor.srgbToLinearChannel(c1);
            const lb = CssColor.srgbToLinearChannel(c2);
            const x = 0.4865709486482162 * lr + 0.26566769316909306 * lg + 0.1982172852343625 * lb;
            const y = 0.2289745640697488 * lr + 0.6917385218365064 * lg + 0.079286914093745 * lb;
            const z = 0.0000000000000000 * lr + 0.04511338185890264 * lg + 1.043944368900976 * lb;
            return CssColor.xyzD65ToCssColor(x, y, z, a);
        }
        if (std.ascii.eqlIgnoreCase(space, "a98-rgb")) {
            const lr = CssColor.a98ToLinear(c0);
            const lg = CssColor.a98ToLinear(c1);
            const lb = CssColor.a98ToLinear(c2);
            const x = 0.5766690429101305 * lr + 0.1855582379065463 * lg + 0.1882286462349947 * lb;
            const y = 0.29734497525053605 * lr + 0.6273635662554661 * lg + 0.07529145849399788 * lb;
            const z = 0.02703136138641234 * lr + 0.07068885253582723 * lg + 0.9913375368376388 * lb;
            return CssColor.xyzD65ToCssColor(x, y, z, a);
        }
        if (std.ascii.eqlIgnoreCase(space, "prophoto-rgb")) {
            const lr = CssColor.prophotoToLinear(c0);
            const lg = CssColor.prophotoToLinear(c1);
            const lb = CssColor.prophotoToLinear(c2);
            const x50 = 0.7977604896723027 * lr + 0.13518583717574031 * lg + 0.0313493495815248 * lb;
            const y50 = 0.2880711282292934 * lr + 0.7118432178101014 * lg + 0.00008565396060525902 * lb;
            const z50 = 0.0 * lr + 0.0 * lg + 0.8251046025104601 * lb;
            return CssColor.xyzD50ToCssColor(x50, y50, z50, a);
        }
        if (std.ascii.eqlIgnoreCase(space, "rec2020")) {
            const lr = CssColor.rec2020ToLinear(c0);
            const lg = CssColor.rec2020ToLinear(c1);
            const lb = CssColor.rec2020ToLinear(c2);
            const x = 0.6369580483012914 * lr + 0.14461690358620832 * lg + 0.1688809751641721 * lb;
            const y = 0.2627002120112671 * lr + 0.6779980715188708 * lg + 0.05930171646986196 * lb;
            const z = 0.000000000000000 * lr + 0.028072693049087428 * lg + 1.060985057710791 * lb;
            return CssColor.xyzD65ToCssColor(x, y, z, a);
        }
        if (std.ascii.eqlIgnoreCase(space, "xyz") || std.ascii.eqlIgnoreCase(space, "xyz-d65")) {
            return CssColor.xyzD65ToCssColor(c0, c1, c2, a);
        }
        if (std.ascii.eqlIgnoreCase(space, "xyz-d50")) {
            return CssColor.xyzD50ToCssColor(c0, c1, c2, a);
        }
        return null;
    }

    pub static fn parse(input: []const u8): CssColor? {
        let cbuf: [1024]u8 = undefined;
        const s = stripCssComments(input, &cbuf)?;
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
            const inner = std.mem.trim(u8, s[open_paren + 1 .. s.len - 1], " \t\r\n");
            if (std.ascii.eqlIgnoreCase(fn_name, "color")) {
                let sp_end: usize = 0;
                while (sp_end < inner.len && inner[sp_end] != ' ' && inner[sp_end] != '\t' && inner[sp_end] != '\r' && inner[sp_end] != '\n') {
                    sp_end += 1;
                }
                const space_name = inner[0..sp_end];
                const rest_args = std.mem.trim(u8, inner[sp_end..], " \t\r\n");
                let ctoks: [4][]const u8 = ["", "", "", ""];
                const cn = splitColorArgs(rest_args, &ctoks);
                if (cn < 3) return null;
                const c_alpha: u8 = if (cn == 4) (parseAlphaToken(ctoks[3])?) else 255;
                const c0 = parsePctOrUnit(ctoks[0], 1.0)?;
                const c1 = parsePctOrUnit(ctoks[1], 1.0)?;
                const c2 = parsePctOrUnit(ctoks[2], 1.0)?;
                return CssColor.fromColorSpace(space_name, c0, c1, c2, c_alpha);
            }
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

            if (std.ascii.eqlIgnoreCase(fn_name, "lab")) {
                const l_val = parsePctOrUnit(tokens[0], 100.0)?;
                const a_val = parsePctOrUnit(tokens[1], 125.0)?;
                const b_val = parsePctOrUnit(tokens[2], 125.0)?;
                return CssColor.fromLab(l_val, a_val, b_val, alpha);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "lch")) {
                const l_val = parsePctOrUnit(tokens[0], 100.0)?;
                const c_val = parsePctOrUnit(tokens[1], 150.0)?;
                const h_val = parseAngleDeg(tokens[2])?;
                return CssColor.fromLch(l_val, c_val, h_val, alpha);
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
        if (_.a == 0) {
            return std.fmt.bufPrint(out_buf, "rgba({d}, {d}, {d}, 0)", .{ _.r, _.g, _.b }) catch "";
        }
        const af = @as(f64, @floatFromInt(_.a)) / 255.0;
        let abuf: [32]u8 = undefined;
        let astr = std.fmt.bufPrint(&abuf, "{d:.6}", .{af}) catch "0";
        while (astr.len > 1 && astr[astr.len - 1] == '0') {
            astr = astr[0 .. astr.len - 1];
        }
        if (astr.len > 1 && astr[astr.len - 1] == '.') {
            astr = astr[0 .. astr.len - 1];
        }
        return std.fmt.bufPrint(out_buf, "rgba({d}, {d}, {d}, {s})", .{ _.r, _.g, _.b, astr }) catch "";
    }

    /// Serializes this `CssColor` to a CSSOM `getComputedStyle().color` string.
    pub fn toCssomString(out_buf: []u8): []const u8 {
        if (_.is_oklch) {
            const c0 = ColorComp { val: _.oklch_l, is_none: false };
            const c1 = ColorComp { val: _.oklch_c, is_none: false };
            const c2 = ColorComp { val: normalizeHueDeg(_.oklch_h), is_none: false };
            const alpha = ColorComp { val: @as(f64, @floatFromInt(_.a)) / 255.0, is_none: false };
            const len = formatColorSpace3(out_buf, "oklch", c0, c1, c2, alpha);
            return out_buf[0..len];
        }
        return _.toRgbString(out_buf);
    }

    /// Parses a CSS Color 4 string and serializes directly to CSSOM `getComputedStyle().color` format.
    /// Supports wide-gamut `lab()`, `lch()`, `oklab()`, `oklch()`, `color(...)`, `none` components,
    /// `calc(...)` expressions, CSS comments, hex, named colors, `rgb()`, `hsl()`, and `hwb()`.
    pub static fn parseColorToCssomString(input: []const u8, out_buf: []u8): ([]const u8)? {
        let cbuf: [1024]u8 = undefined;
        const s = stripCssComments(input, &cbuf)?;
        if (s.len == 0) {
            return null;
        }

        if (s[0] == '#') {
            const col = CssColor.parse(s)?;
            return col.toRgbString(out_buf);
        }

        if (open_paren := std.mem.indexOfScalar(u8, s, '(')) {
            if (s[s.len - 1] != ')') {
                return null;
            }
            const fn_name = std.mem.trim(u8, s[0..open_paren], " \t\r\n");
            const inner = std.mem.trim(u8, s[open_paren + 1 .. s.len - 1], " \t\r\n");

            if (std.ascii.eqlIgnoreCase(fn_name, "color")) {
                let sp_end: usize = 0;
                while (sp_end < inner.len && inner[sp_end] != ' ' && inner[sp_end] != '\t' && inner[sp_end] != '\r' && inner[sp_end] != '\n') {
                    sp_end += 1;
                }
                const space_raw = inner[0..sp_end];
                let canonical_space: []const u8 = "";
                if (std.ascii.eqlIgnoreCase(space_raw, "srgb")) {
                    canonical_space = "srgb";
                } else if (std.ascii.eqlIgnoreCase(space_raw, "srgb-linear")) {
                    canonical_space = "srgb-linear";
                } else if (std.ascii.eqlIgnoreCase(space_raw, "display-p3")) {
                    canonical_space = "display-p3";
                } else if (std.ascii.eqlIgnoreCase(space_raw, "a98-rgb")) {
                    canonical_space = "a98-rgb";
                } else if (std.ascii.eqlIgnoreCase(space_raw, "prophoto-rgb")) {
                    canonical_space = "prophoto-rgb";
                } else if (std.ascii.eqlIgnoreCase(space_raw, "rec2020")) {
                    canonical_space = "rec2020";
                } else if (std.ascii.eqlIgnoreCase(space_raw, "xyz") || std.ascii.eqlIgnoreCase(space_raw, "xyz-d65")) {
                    canonical_space = "xyz-d65";
                } else if (std.ascii.eqlIgnoreCase(space_raw, "xyz-d50")) {
                    canonical_space = "xyz-d50";
                } else {
                    return null;
                }
                const rest_args = std.mem.trim(u8, inner[sp_end..], " \t\r\n");
                let ctoks: [4][]const u8 = ["", "", "", ""];
                let is_comma = false;
                const cn = splitColorArgsEx(rest_args, &ctoks, &is_comma);
                if (cn < 3 || is_comma) return null;
                const c0 = parseCompOrNone(ctoks[0], 1.0)?;
                const c1 = parseCompOrNone(ctoks[1], 1.0)?;
                const c2 = parseCompOrNone(ctoks[2], 1.0)?;
                const alpha = if (cn == 4) (parseAlphaOrNone(ctoks[3])?) else (ColorComp { val: 1.0, is_none: false });
                const len = formatColorSpace4(out_buf, canonical_space, c0, c1, c2, alpha);
                return out_buf[0..len];
            }

            let tokens: [4][]const u8 = ["", "", "", ""];
            let is_comma = false;
            const n = splitColorArgsEx(inner, &tokens, &is_comma);
            if (n < 3) {
                return null;
            }
            const alpha = if (n == 4) (parseAlphaOrNone(tokens[3])?) else (ColorComp { val: 1.0, is_none: false });
            if (is_comma && alpha.is_none) return null;

            if (std.ascii.eqlIgnoreCase(fn_name, "lab")) {
                if (is_comma) return null;
                let l_comp = parseCompOrNone(tokens[0], 100.0)?;
                if (!l_comp.is_none) l_comp.val = std.math.clamp(l_comp.val, 0.0, 100.0);
                const a_comp = parseCompOrNone(tokens[1], 125.0)?;
                const b_comp = parseCompOrNone(tokens[2], 125.0)?;
                const len = formatColorSpace3(out_buf, "lab", l_comp, a_comp, b_comp, alpha);
                return out_buf[0..len];
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "lch")) {
                if (is_comma) return null;
                let l_comp = parseCompOrNone(tokens[0], 100.0)?;
                if (!l_comp.is_none) l_comp.val = std.math.clamp(l_comp.val, 0.0, 100.0);
                let c_comp = parseCompOrNone(tokens[1], 150.0)?;
                if (!c_comp.is_none && c_comp.val < 0.0) c_comp.val = 0.0;
                let h_comp = parseAngleOrNone(tokens[2])?;
                if (!h_comp.is_none) h_comp.val = normalizeHueDeg(h_comp.val);
                const len = formatColorSpace3(out_buf, "lch", l_comp, c_comp, h_comp, alpha);
                return out_buf[0..len];
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "oklab")) {
                if (is_comma) return null;
                let l_comp = parseCompOrNone(tokens[0], 1.0)?;
                if (!l_comp.is_none) l_comp.val = std.math.clamp(l_comp.val, 0.0, 1.0);
                const a_comp = parseCompOrNone(tokens[1], 0.4)?;
                const b_comp = parseCompOrNone(tokens[2], 0.4)?;
                const len = formatColorSpace3(out_buf, "oklab", l_comp, a_comp, b_comp, alpha);
                return out_buf[0..len];
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "oklch")) {
                if (is_comma) return null;
                let l_comp = parseCompOrNone(tokens[0], 1.0)?;
                if (!l_comp.is_none) l_comp.val = std.math.clamp(l_comp.val, 0.0, 1.0);
                let c_comp = parseCompOrNone(tokens[1], 0.4)?;
                if (!c_comp.is_none && c_comp.val < 0.0) c_comp.val = 0.0;
                let h_comp = parseAngleOrNone(tokens[2])?;
                if (!h_comp.is_none) h_comp.val = normalizeHueDeg(h_comp.val);
                const len = formatColorSpace3(out_buf, "oklch", l_comp, c_comp, h_comp, alpha);
                return out_buf[0..len];
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "rgb") || std.ascii.eqlIgnoreCase(fn_name, "rgba")) {
                const rc = parseRgbChannelOrNone(tokens[0])?;
                const gc = parseRgbChannelOrNone(tokens[1])?;
                const bc = parseRgbChannelOrNone(tokens[2])?;
                if (is_comma && (rc.is_none || gc.is_none || bc.is_none)) return null;
                if (rc.is_none || gc.is_none || bc.is_none || alpha.is_none) {
                    const c0 = ColorComp { val: rc.val / 255.0, is_none: rc.is_none };
                    const c1 = ColorComp { val: gc.val / 255.0, is_none: gc.is_none };
                    const c2 = ColorComp { val: bc.val / 255.0, is_none: bc.is_none };
                    const len = formatColorSpace4(out_buf, "srgb", c0, c1, c2, alpha);
                    return out_buf[0..len];
                }
                return formatRgbaFloatAlpha(out_buf, clampU8(rc.val), clampU8(gc.val), clampU8(bc.val), alpha.val);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "hsl") || std.ascii.eqlIgnoreCase(fn_name, "hsla")) {
                const hc = parseAngleOrNone(tokens[0])?;
                const sc = parseCompOrNone(tokens[1], 100.0)?;
                const lc = parseCompOrNone(tokens[2], 100.0)?;
                if (is_comma && (hc.is_none || sc.is_none || lc.is_none)) return null;
                const col = CssColor.fromHsl(hc.val, sc.val, lc.val, clampU8(alpha.val * 255.0));
                if (alpha.is_none) {
                    const c0 = ColorComp { val: @as(f64, @floatFromInt(col.r)) / 255.0, is_none: false };
                    const c1 = ColorComp { val: @as(f64, @floatFromInt(col.g)) / 255.0, is_none: false };
                    const c2 = ColorComp { val: @as(f64, @floatFromInt(col.b)) / 255.0, is_none: false };
                    const len = formatColorSpace4(out_buf, "srgb", c0, c1, c2, alpha);
                    return out_buf[0..len];
                }
                return formatRgbaFloatAlpha(out_buf, col.r, col.g, col.b, alpha.val);
            }

            if (std.ascii.eqlIgnoreCase(fn_name, "hwb")) {
                if (is_comma) return null;
                const hc = parseAngleOrNone(tokens[0])?;
                const wc = parseCompOrNone(tokens[1], 100.0)?;
                const bc = parseCompOrNone(tokens[2], 100.0)?;
                const col = CssColor.fromHwb(hc.val, wc.val, bc.val, clampU8(alpha.val * 255.0));
                if (alpha.is_none) {
                    const c0 = ColorComp { val: @as(f64, @floatFromInt(col.r)) / 255.0, is_none: false };
                    const c1 = ColorComp { val: @as(f64, @floatFromInt(col.g)) / 255.0, is_none: false };
                    const c2 = ColorComp { val: @as(f64, @floatFromInt(col.b)) / 255.0, is_none: false };
                    const len = formatColorSpace4(out_buf, "srgb", c0, c1, c2, alpha);
                    return out_buf[0..len];
                }
                return formatRgbaFloatAlpha(out_buf, col.r, col.g, col.b, alpha.val);
            }

            return null;
        }

        const named_col = CssColor.parse(s)?;
        return named_col.toRgbString(out_buf);
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

fn hasBalancedParens(s: []const u8): bool {
    let depth: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '(') {
            depth += 1;
        } else if (s[i] == ')') {
            if (depth == 0) return false;
            depth -= 1;
        }
        i += 1;
    }
    return depth == 0;
}

fn isValidColorMixStop(raw: []const u8, depth: usize): bool {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return false;
    if (isValidCssColorSyntaxInner(s, depth)) return true;

    // Check trailing percentage: "<color> <pct>%"
    let paren_depth: usize = 0;
    let last_ws: usize? = null;
    let first_ws: usize? = null;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '(') {
            paren_depth += 1;
        } else if (s[i] == ')') {
            if (paren_depth > 0) paren_depth -= 1;
        } else if (paren_depth == 0 && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
            if (first_ws == null) first_ws = i;
            last_ws = i;
        }
        i += 1;
    }
    if (lws := last_ws) {
        const tail = std.mem.trim(u8, s[lws + 1 ..], " \t\r\n");
        if (tail.len > 1 && tail[tail.len - 1] == '%' && parseFloatToken(tail[0 .. tail.len - 1]) != null) {
            const head = std.mem.trim(u8, s[0..lws], " \t\r\n");
            if (isValidCssColorSyntaxInner(head, depth)) return true;
        }
    }
    if (fws := first_ws) {
        const head = std.mem.trim(u8, s[0..fws], " \t\r\n");
        if (head.len > 1 && head[head.len - 1] == '%' && parseFloatToken(head[0 .. head.len - 1]) != null) {
            const tail = std.mem.trim(u8, s[fws + 1 ..], " \t\r\n");
            if (isValidCssColorSyntaxInner(tail, depth)) return true;
        }
    }
    return false;
}

fn splitTopLevelCommaArgs(inner: []const u8, out_args: *[4][]const u8): usize {
    let count: usize = 0;
    let depth: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i < inner.len) {
        const ch = inner[i];
        if (ch == '(') {
            depth += 1;
        } else if (ch == ')') {
            if (depth == 0) return 0;
            depth -= 1;
        } else if (ch == ',' && depth == 0) {
            if (count >= 4) return 0;
            out_args[count] = std.mem.trim(u8, inner[start..i], " \t\r\n");
            count += 1;
            start = i + 1;
        }
        i += 1;
    }
    if (depth != 0) return 0;
    if (count >= 4) return 0;
    out_args[count] = std.mem.trim(u8, inner[start..], " \t\r\n");
    count += 1;
    return count;
}

fn isValidCssColorSyntaxInner(raw: []const u8, depth: usize): bool {
    if (depth > 16) return false;
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return false;

    if (std.ascii.eqlIgnoreCase(s, "currentcolor") || std.ascii.eqlIgnoreCase(s, "transparent")) {
        return true;
    }

    if (open_paren := std.mem.indexOfScalar(u8, s, '(')) {
        if (s[s.len - 1] != ')') return false;
        const fn_name = std.mem.trim(u8, s[0..open_paren], " \t\r\n");
        const inner = std.mem.trim(u8, s[open_paren + 1 .. s.len - 1], " \t\r\n");
        if (fn_name.len == 0 || inner.len == 0) return false;
        if (!hasBalancedParens(inner)) return false;

        if (std.ascii.eqlIgnoreCase(fn_name, "var")) {
            return std.mem.startsWith(u8, inner, "--") && inner.len > 2;
        }
        if (std.ascii.eqlIgnoreCase(fn_name, "env")) {
            return true;
        }
        if (std.ascii.eqlIgnoreCase(fn_name, "light-dark")) {
            let args: [4][]const u8 = ["", "", "", ""];
            const n = splitTopLevelCommaArgs(inner, &args);
            if (n != 2) return false;
            return isValidCssColorSyntaxInner(args[0], depth + 1) && isValidCssColorSyntaxInner(args[1], depth + 1);
        }
        if (std.ascii.eqlIgnoreCase(fn_name, "color-mix")) {
            let args: [4][]const u8 = ["", "", "", ""];
            const n = splitTopLevelCommaArgs(inner, &args);
            if (n != 3) return false;
            const method = args[0];
            if (method.len < 4) return false;
            if (!std.ascii.eqlIgnoreCase(method[0..2], "in")) return false;
            if (method[2] != ' ' && method[2] != '\t' && method[2] != '\r' && method[2] != '\n') return false;
            const space_rest = std.mem.trim(u8, method[2..], " \t\r\n");
            if (space_rest.len == 0) return false;
            return isValidColorMixStop(args[1], depth + 1) && isValidColorMixStop(args[2], depth + 1);
        }

        let out_buf: [256]u8 = undefined;
        return CssColor.parseColorToCssomString(s, &out_buf) != null;
    }

    return CssColor.parse(s) != null;
}

/// Validates CSS Color Level 4 syntax (comments, keywords, hex, named colors,
/// functional notations, color-mix(), light-dark(), var(), env()).
pub fn isValidCssColorSyntax(val: []const u8): bool {
    let cbuf: [1024]u8 = undefined;
    const s_opt = stripCssComments(val, &cbuf);
    if (s_opt == null) return false;
    const s = s_opt.?;
    if (s.len == 0) return false;
    return isValidCssColorSyntaxInner(s, 0);
}
