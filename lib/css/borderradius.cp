// Source: Absorbed from /data/dev/bun/src/css/properties/border_radius.cp | Synced: 2026-11-22T12:49:17Z
//! CSS Backgrounds Level 3 `border-radius` shorthand parser & shortest-form serializer
//! (supports 1–4 horizontal radii + optional `/ 1–4 vertical radii` elliptical corners).

import std from "std";

export struct RadiusValue {
    value: f32 = 0.0,
    unit: []const u8 = "px",

    pub static fn zero(): RadiusValue {
        return RadiusValue{ value: 0.0, unit: "px" };
    }

    pub fn isZero(): bool {
        return _.value == 0.0;
    }

    pub fn eql(other: RadiusValue): bool {
        if (_.value == 0.0 && other.value == 0.0) return true;
        return _.value == other.value && asciiCaseEql(_.unit, other.unit);
    }

    pub static fn parse(tok: []const u8): RadiusValue? {
        if (tok.len == 0) return null;
        if (tok.len == 1 && tok[0] == '0') return RadiusValue.zero();

        let split: usize = tok.len;
        let i: usize = 0;
        while (i < tok.len) : (i += 1) {
            const c = tok[i];
            const is_num = (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+';
            if (!is_num) {
                split = i;
                break;
            }
        }
        if (split == 0 || split == tok.len) {
            const v0 = std.fmt.parseFloat(f32, tok) catch return null;
            if (v0 == 0.0) return RadiusValue.zero();
            return null;
        }
        const num = std.fmt.parseFloat(f32, tok[0..split]) catch return null;
        if (std.math.isNan(num) || std.math.isInf(num) || num < 0.0) return null;
        const unit = tok[split..];
        if (!isValidRadiusUnit(unit)) return null;
        return RadiusValue{ value: num, unit: unit };
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.value == 0.0) return std.fmt.bufPrint(out, "0", .{}) catch out[0..0];
        return std.fmt.bufPrint(out, "{d}{s}", .{ _.value, _.unit }) catch out[0..0];
    }
}

export struct CornerRadius {
    x: RadiusValue = RadiusValue.zero(),
    y: RadiusValue = RadiusValue.zero(),

    pub static fn circular(r: RadiusValue): CornerRadius {
        return CornerRadius{ x: r, y: r };
    }

    pub fn isCircular(): bool {
        return _.x.eql(_.y);
    }

    pub fn eql(other: CornerRadius): bool {
        return _.x.eql(other.x) && _.y.eql(other.y);
    }
}

export struct BorderRadius {
    top_left: CornerRadius = CornerRadius{},
    top_right: CornerRadius = CornerRadius{},
    bottom_right: CornerRadius = CornerRadius{},
    bottom_left: CornerRadius = CornerRadius{},

    pub fn isUniform(): bool {
        return _.top_left.eql(_.top_right)
            && _.top_left.eql(_.bottom_right)
            && _.top_left.eql(_.bottom_left);
    }

    pub fn isCircular(): bool {
        return _.top_left.isCircular()
            && _.top_right.isCircular()
            && _.bottom_right.isCircular()
            && _.bottom_left.isCircular();
    }

    pub fn eql(other: BorderRadius): bool {
        return _.top_left.eql(other.top_left)
            && _.top_right.eql(other.top_right)
            && _.bottom_right.eql(other.bottom_right)
            && _.bottom_left.eql(other.bottom_left);
    }

    /// Parse CSS `border-radius` shorthand: `1..4 values` optionally followed by `/ 1..4 values`.
    pub static fn parse(input: []const u8): BorderRadius? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let slash_idx: usize? = null;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (s[i] == '/') {
                if (slash_idx != null) return null; // multiple '/' invalid
                slash_idx = i;
            }
        }

        let w_str = s;
        if (idx := slash_idx) {
            w_str = s[0..idx];
        }
        const w4 = parseQuad(w_str) ?? return null;

        let h4 = w4;
        if (idx := slash_idx) {
            h4 = parseQuad(s[idx + 1 ..]) ?? return null;
        }

        return BorderRadius{
            top_left: CornerRadius{ x: w4[0], y: h4[0] },
            top_right: CornerRadius{ x: w4[1], y: h4[1] },
            bottom_right: CornerRadius{ x: w4[2], y: h4[2] },
            bottom_left: CornerRadius{ x: w4[3], y: h4[3] },
        };
    }

    /// Serialize to shortest canonical CSS `border-radius` form into `out`.
    pub fn toCss(out: []u8): []const u8 {
        const w: [4]RadiusValue = [_.top_left.x, _.top_right.x, _.bottom_right.x, _.bottom_left.x];
        const h: [4]RadiusValue = [_.top_left.y, _.top_right.y, _.bottom_right.y, _.bottom_left.y];

        let len = writeQuad(out, 0, w);
        const same = w[0].eql(h[0]) && w[1].eql(h[1]) && w[2].eql(h[2]) && w[3].eql(h[3]);
        if (!same) {
            len = appendStr(out, len, " / ");
            len = writeQuad(out, len, h);
        }
        return out[0..len];
    }
}

fn parseQuad(s_in: []const u8): ([4]RadiusValue)? {
    const s = trimAscii(s_in);
    if (s.len == 0) return null;
    let vals: [4]RadiusValue = [RadiusValue.zero(); 4];
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
        if (pos >= s.len) break;
        const start = pos;
        while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
        if (count >= 4) return null;
        vals[count] = RadiusValue.parse(s[start..pos]) ?? return null;
        count += 1;
    }
    if (count == 0) return null;
    if (count == 1) {
        return [vals[0], vals[0], vals[0], vals[0]];
    } else if (count == 2) {
        return [vals[0], vals[1], vals[0], vals[1]];
    } else if (count == 3) {
        return [vals[0], vals[1], vals[2], vals[1]];
    }
    return vals;
}

fn writeQuad(out: []u8, start_off: usize, q: [4]RadiusValue): usize {
    let tmp: [32]u8 = undefined;
    let len = start_off;

    const all_same = q[0].eql(q[1]) && q[0].eql(q[2]) && q[0].eql(q[3]);
    if (all_same) {
        return appendStr(out, len, q[0].toCss(tmp[0..]));
    }
    const pair_same = q[0].eql(q[2]) && q[1].eql(q[3]);
    if (pair_same) {
        len = appendStr(out, len, q[0].toCss(tmp[0..]));
        len = appendStr(out, len, " ");
        return appendStr(out, len, q[1].toCss(tmp[0..]));
    }
    if (q[1].eql(q[3])) {
        len = appendStr(out, len, q[0].toCss(tmp[0..]));
        len = appendStr(out, len, " ");
        len = appendStr(out, len, q[1].toCss(tmp[0..]));
        len = appendStr(out, len, " ");
        return appendStr(out, len, q[2].toCss(tmp[0..]));
    }
    len = appendStr(out, len, q[0].toCss(tmp[0..]));
    len = appendStr(out, len, " ");
    len = appendStr(out, len, q[1].toCss(tmp[0..]));
    len = appendStr(out, len, " ");
    len = appendStr(out, len, q[2].toCss(tmp[0..]));
    len = appendStr(out, len, " ");
    return appendStr(out, len, q[3].toCss(tmp[0..]));
}

fn isValidRadiusUnit(u: []const u8): bool {
    return asciiCaseEql(u, "px")
        || asciiCaseEql(u, "%")
        || asciiCaseEql(u, "em")
        || asciiCaseEql(u, "rem")
        || asciiCaseEql(u, "pt")
        || asciiCaseEql(u, "vh")
        || asciiCaseEql(u, "vw");
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimAscii(s: []const u8): []const u8 {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isWs(s[a])) : (a += 1) {}
    while (b > a && isWs(s[b - 1])) : (b -= 1) {}
    return s[a..b];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiCaseEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn appendStr(out: []u8, offset: usize, src: []const u8): usize {
    let pos = offset;
    let i: usize = 0;
    while (i < src.len && pos < out.len) : (i += 1) {
        out[pos] = src[i];
        pos += 1;
    }
    return pos;
}
