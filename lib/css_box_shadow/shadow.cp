// Source: Absorbed from /data/dev/bun/src/css/properties/box_shadow.cp, /data/dev/bun/src/css/properties/outline.cp
//! CSS `box-shadow` shorthand parser/serializer and `border-style`/`outline-style` `LineStyle` model.

import std from "std";

export enum LineStyle: u8 {
    none = 0,
    hidden = 1,
    inset = 2,
    groove = 3,
    outset = 4,
    ridge = 5,
    dotted = 6,
    dashed = 7,
    solid = 8,
    double = 9,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .none => "none",
            .hidden => "hidden",
            .inset => "inset",
            .groove => "groove",
            .outset => "outset",
            .ridge => "ridge",
            .dotted => "dotted",
            .dashed => "dashed",
            .solid => "solid",
            .double => "double",
        };
    }

    pub static fn parse(input: []const u8): LineStyle? {
        const s = trimAscii(input);
        if (asciiCaseEql(s, "none")) return .none;
        if (asciiCaseEql(s, "hidden")) return .hidden;
        if (asciiCaseEql(s, "inset")) return .inset;
        if (asciiCaseEql(s, "groove")) return .groove;
        if (asciiCaseEql(s, "outset")) return .outset;
        if (asciiCaseEql(s, "ridge")) return .ridge;
        if (asciiCaseEql(s, "dotted")) return .dotted;
        if (asciiCaseEql(s, "dashed")) return .dashed;
        if (asciiCaseEql(s, "solid")) return .solid;
        if (asciiCaseEql(s, "double")) return .double;
        return null;
    }

    pub fn isVisible(): bool {
        return _ != .none && _ != .hidden;
    }
}

export enum OutlineStyle: u8 {
    auto = 0,
    none = 1,
    inset = 2,
    groove = 3,
    outset = 4,
    ridge = 5,
    dotted = 6,
    dashed = 7,
    solid = 8,
    double = 9,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .auto => "auto",
            .none => "none",
            .inset => "inset",
            .groove => "groove",
            .outset => "outset",
            .ridge => "ridge",
            .dotted => "dotted",
            .dashed => "dashed",
            .solid => "solid",
            .double => "double",
        };
    }

    pub static fn parse(input: []const u8): OutlineStyle? {
        const s = trimAscii(input);
        if (asciiCaseEql(s, "auto")) return .auto;
        if (asciiCaseEql(s, "none")) return .none;
        if (asciiCaseEql(s, "inset")) return .inset;
        if (asciiCaseEql(s, "groove")) return .groove;
        if (asciiCaseEql(s, "outset")) return .outset;
        if (asciiCaseEql(s, "ridge")) return .ridge;
        if (asciiCaseEql(s, "dotted")) return .dotted;
        if (asciiCaseEql(s, "dashed")) return .dashed;
        if (asciiCaseEql(s, "solid")) return .solid;
        if (asciiCaseEql(s, "double")) return .double;
        return null;
    }
}

export struct ShadowLength {
    value: f32 = 0.0,
    unit: []const u8 = "px",

    pub static fn zero(): ShadowLength {
        return ShadowLength{ value: 0.0, unit: "px" };
    }

    pub fn isZero(): bool {
        return _.value == 0.0;
    }

    pub static fn parse(tok: []const u8): ShadowLength? {
        if (tok.len == 0) return null;
        if (tok.len == 1 && tok[0] == '0') return ShadowLength.zero();

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
            // Unitless non-zero length is invalid in CSS
            const v0 = std.fmt.parseFloat(f32, tok) catch return null;
            if (v0 == 0.0) return ShadowLength.zero();
            return null;
        }
        const num = std.fmt.parseFloat(f32, tok[0..split]) catch return null;
        if (std.math.isNan(num) || std.math.isInf(num)) return null;
        const unit = tok[split..];
        if (!isValidLengthUnit(unit)) return null;
        return ShadowLength{ value: num, unit: unit };
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.value == 0.0) return std.fmt.bufPrint(out, "0", .{}) catch out[0..0];
        return std.fmt.bufPrint(out, "{d}{s}", .{ _.value, _.unit }) catch out[0..0];
    }
}

/// Parsed CSS `box-shadow` value.
export struct BoxShadow {
    x_offset: ShadowLength = ShadowLength.zero(),
    y_offset: ShadowLength = ShadowLength.zero(),
    blur: ShadowLength = ShadowLength.zero(),
    spread: ShadowLength = ShadowLength.zero(),
    color: []const u8 = "currentcolor",
    inset: bool = false,

    pub static fn parse(input: []const u8): BoxShadow? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let res = BoxShadow{};
        let has_lengths = false;
        let has_color = false;
        let pos: usize = 0;

        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const tok = nextToken(s, *pos);
            if (tok.len == 0) break;

            if (!res.inset && asciiCaseEql(tok, "inset")) {
                res.inset = true;
            } else if (!has_lengths && ShadowLength.parse(tok) != null) {
                res.x_offset = ShadowLength.parse(tok).?;

                // Mandatory y_offset
                while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
                const y_tok = nextToken(s, *pos);
                const y_len = ShadowLength.parse(y_tok) ?? return null;
                res.y_offset = y_len;

                // Optional blur (must be non-negative)
                const saved_blur = pos;
                while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
                if (pos < s.len) {
                    const b_tok = nextToken(s, *pos);
                    if (b_len := ShadowLength.parse(b_tok)) {
                        if (b_len.value < 0.0) return null;
                        res.blur = b_len;
                        // Optional spread
                        const saved_spread = pos;
                        while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
                        if (pos < s.len) {
                            const sp_tok = nextToken(s, *pos);
                            if (sp_len := ShadowLength.parse(sp_tok)) {
                                res.spread = sp_len;
                            } else {
                                pos = saved_spread;
                            }
                        }
                    } else {
                        pos = saved_blur;
                    }
                }
                has_lengths = true;
            } else if (!has_color) {
                res.color = tok;
                has_color = true;
            } else {
                return null;
            }
        }

        if (!has_lengths) return null;
        return res;
    }

    pub fn toCss(out: []u8): []const u8 {
        let len: usize = 0;
        let tmp: [48]u8 = undefined;
        if (_.inset) len = appendPart(out, len, "inset");
        len = appendPart(out, len, _.x_offset.toCss(tmp[0..]));
        len = appendPart(out, len, _.y_offset.toCss(tmp[0..]));
        if (!_.blur.isZero() || !_.spread.isZero()) {
            len = appendPart(out, len, _.blur.toCss(tmp[0..]));
            if (!_.spread.isZero()) {
                len = appendPart(out, len, _.spread.toCss(tmp[0..]));
            }
        }
        if (!asciiCaseEql(_.color, "currentcolor")) {
            len = appendPart(out, len, _.color);
        }
        return out[0..len];
    }
}

fn isValidLengthUnit(u: []const u8): bool {
    return asciiCaseEql(u, "px")
        || asciiCaseEql(u, "em")
        || asciiCaseEql(u, "rem")
        || asciiCaseEql(u, "pt")
        || asciiCaseEql(u, "cm")
        || asciiCaseEql(u, "mm")
        || asciiCaseEql(u, "in")
        || asciiCaseEql(u, "vh")
        || asciiCaseEql(u, "vw")
        || asciiCaseEql(u, "ch");
}

fn nextToken(s: []const u8, pos: *usize): []const u8 {
    const start = pos.*;
    let depth: i32 = 0;
    while (pos.* < s.len) : (pos.* += 1) {
        const c = s[pos.*];
        if (c == '(') {
            depth += 1;
        } else if (c == ')') {
            if (depth > 0) depth -= 1;
        } else if (depth == 0 && isWs(c)) {
            break;
        }
    }
    return s[start..pos.*];
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

fn appendPart(out: []u8, offset: usize, part: []const u8): usize {
    if (part.len == 0) return offset;
    let pos = offset;
    if (pos > 0 && pos < out.len) {
        out[pos] = ' ';
        pos += 1;
    }
    let i: usize = 0;
    while (i < part.len && pos < out.len) : (i += 1) {
        out[pos] = part[i];
        pos += 1;
    }
    return pos;
}
