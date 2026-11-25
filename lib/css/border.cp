// Source: Absorbed from /data/dev/bun/src/css/properties/border.cp | Synced: 2026-11-23T21:37:18Z
// Pure-CP CSS Border properties: LineStyle, BorderSideWidth, BorderColorValue,
// Border shorthand, BorderWidth/BorderStyle/BorderColor rect shorthands, and
// logical start/end border shorthands.

import { isValidCssLenPct } from "./length.cp";

pub enum LineStyle: u8 {
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

    pub static fn default(): LineStyle {
        return .none;
    }

    pub static fn parse(input: []const u8): LineStyle? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "none")) return .none;
        if (eqIgnoreCase(s, "hidden")) return .hidden;
        if (eqIgnoreCase(s, "inset")) return .inset;
        if (eqIgnoreCase(s, "groove")) return .groove;
        if (eqIgnoreCase(s, "outset")) return .outset;
        if (eqIgnoreCase(s, "ridge")) return .ridge;
        if (eqIgnoreCase(s, "dotted")) return .dotted;
        if (eqIgnoreCase(s, "dashed")) return .dashed;
        if (eqIgnoreCase(s, "solid")) return .solid;
        if (eqIgnoreCase(s, "double")) return .double;
        return null;
    }

    pub fn asStr(): []const u8 {
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

    pub fn toCss(): []const u8 {
        return _.asStr();
    }

    pub fn isVisible(): bool {
        return _ != .none && _ != .hidden;
    }

    pub fn eql(other: LineStyle): bool {
        return _ == other;
    }
}

pub enum BorderLengthUnit: u8 {
    px = 0,
    em = 1,
    rem = 2,
    pt = 3,
    vw = 4,
    vh = 5,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .px => "px",
            .em => "em",
            .rem => "rem",
            .pt => "pt",
            .vw => "vw",
            .vh => "vh",
        };
    }
}

pub enum BorderSideWidthKind: u8 {
    thin = 0,
    medium = 1,
    thick = 2,
    length = 3,
}

/// CSS `<line-width>` value used by `border-width` and `border` shorthand.
pub struct BorderSideWidth {
    kind: BorderSideWidthKind,
    value: f32,
    unit: BorderLengthUnit,

    pub static fn thin(): BorderSideWidth {
        return BorderSideWidth{ kind: .thin, value: 0.0, unit: .px };
    }

    pub static fn medium(): BorderSideWidth {
        return BorderSideWidth{ kind: .medium, value: 0.0, unit: .px };
    }

    pub static fn thick(): BorderSideWidth {
        return BorderSideWidth{ kind: .thick, value: 0.0, unit: .px };
    }

    pub static fn default(): BorderSideWidth {
        return BorderSideWidth.medium();
    }

    pub static fn length(v: f32, u: BorderLengthUnit): BorderSideWidth {
        return BorderSideWidth{ kind: .length, value: v, unit: u };
    }

    pub fn isDefault(): bool {
        return _.kind == .medium;
    }

    pub fn eql(other: BorderSideWidth): bool {
        if (_.kind != other.kind) return false;
        if (_.kind != .length) return true;
        if (_.value == 0.0 && other.value == 0.0) return true;
        return _.value == other.value && _.unit == other.unit;
    }

    pub static fn parse(input: []const u8): BorderSideWidth? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "thin")) return BorderSideWidth.thin();
        if (eqIgnoreCase(s, "medium")) return BorderSideWidth.medium();
        if (eqIgnoreCase(s, "thick")) return BorderSideWidth.thick();

        let idx: usize = 0;
        let has_digit = false;
        let has_dot = false;
        if (s[0] == '+') {
            idx = 1;
        } else if (s[0] == '-') {
            // Negative border width is invalid per CSS Backgrounds spec.
            return null;
        }
        while (idx < s.len) : (idx += 1) {
            const c = s[idx];
            if (c >= '0' && c <= '9') {
                has_digit = true;
            } else if (c == '.' && !has_dot) {
                has_dot = true;
            } else {
                break;
            }
        }
        if (!has_digit) return null;
        const num = parseNonNegFloat(s[0..idx]) ?? return null;
        const unit_str = s[idx..];
        if (unit_str.len == 0) {
            if (num == 0.0) return BorderSideWidth.length(0.0, .px);
            return null;
        }
        const u = parseLengthUnit(unit_str) ?? return null;
        return BorderSideWidth.length(num, u);
    }

    pub fn toCss(out: []u8): []const u8 {
        const len = writeSideWidth(out, 0, _);
        return out[0..len];
    }
}

pub enum BorderColorKind: u8 {
    current_color = 0,
    transparent = 1,
    rgba = 2,
}

/// Lightweight CSS color representation for border properties.
pub struct BorderColorValue {
    kind: BorderColorKind,
    r: u8,
    g: u8,
    b: u8,
    a: u8,

    pub static fn currentColor(): BorderColorValue {
        return BorderColorValue{ kind: .current_color, r: 0, g: 0, b: 0, a: 255 };
    }

    pub static fn transparent(): BorderColorValue {
        return BorderColorValue{ kind: .transparent, r: 0, g: 0, b: 0, a: 0 };
    }

    pub static fn rgb(r: u8, g: u8, b: u8): BorderColorValue {
        return BorderColorValue{ kind: .rgba, r: r, g: g, b: b, a: 255 };
    }

    pub static fn rgba(r: u8, g: u8, b: u8, a: u8): BorderColorValue {
        return BorderColorValue{ kind: .rgba, r: r, g: g, b: b, a: a };
    }

    pub fn isDefault(): bool {
        return _.kind == .current_color;
    }

    pub fn eql(other: BorderColorValue): bool {
        if (_.kind != other.kind) return false;
        if (_.kind != .rgba) return true;
        return _.r == other.r && _.g == other.g && _.b == other.b && _.a == other.a;
    }

    pub static fn parse(input: []const u8): BorderColorValue? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "currentcolor")) return BorderColorValue.currentColor();
        if (eqIgnoreCase(s, "transparent")) return BorderColorValue.transparent();
        if (eqIgnoreCase(s, "black")) return BorderColorValue.rgb(0, 0, 0);
        if (eqIgnoreCase(s, "white")) return BorderColorValue.rgb(255, 255, 255);
        if (eqIgnoreCase(s, "red")) return BorderColorValue.rgb(255, 0, 0);
        if (eqIgnoreCase(s, "green")) return BorderColorValue.rgb(0, 128, 0);
        if (eqIgnoreCase(s, "blue")) return BorderColorValue.rgb(0, 0, 255);
        if (eqIgnoreCase(s, "gray") || eqIgnoreCase(s, "grey")) return BorderColorValue.rgb(128, 128, 128);
        if (eqIgnoreCase(s, "silver")) return BorderColorValue.rgb(192, 192, 192);
        if (eqIgnoreCase(s, "yellow")) return BorderColorValue.rgb(255, 255, 0);
        if (eqIgnoreCase(s, "orange")) return BorderColorValue.rgb(255, 165, 0);
        if (eqIgnoreCase(s, "purple")) return BorderColorValue.rgb(128, 0, 128);
        if (eqIgnoreCase(s, "cyan") || eqIgnoreCase(s, "aqua")) return BorderColorValue.rgb(0, 255, 255);
        if (eqIgnoreCase(s, "magenta") || eqIgnoreCase(s, "fuchsia")) return BorderColorValue.rgb(255, 0, 255);

        if (s[0] == '#') {
            return parseHexColor(s[1..]);
        }
        return null;
    }

    pub fn toCss(out: []u8): []const u8 {
        const len = writeColor(out, 0, _);
        return out[0..len];
    }
}

/// CSS `border` / `border-top` / `border-inline` shorthand value.
pub struct Border {
    width: BorderSideWidth,
    style: LineStyle,
    color: BorderColorValue,

    pub static fn default(): Border {
        return Border{
            width: BorderSideWidth.default(),
            style: LineStyle.default(),
            color: BorderColorValue.currentColor(),
        };
    }

    pub fn eql(other: Border): bool {
        return _.width.eql(other.width) && _.style.eql(other.style) && _.color.eql(other.color);
    }

    /// Parse `border` shorthand in any token order: `<line-width> || <line-style> || <color>`.
    pub static fn parse(input: []const u8): Border? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let width: BorderSideWidth? = null;
        let style: LineStyle? = null;
        let color: BorderColorValue? = null;
        let has_any = false;

        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            const tok = s[start..pos];

            if (width == null) {
                if (w := BorderSideWidth.parse(tok)) {
                    width = w;
                    has_any = true;
                    continue;
                }
            }
            if (style == null) {
                if (st := LineStyle.parse(tok)) {
                    style = st;
                    has_any = true;
                    continue;
                }
            }
            if (color == null) {
                if (c := BorderColorValue.parse(tok)) {
                    color = c;
                    has_any = true;
                    continue;
                }
            }
            // Unknown or duplicate component
            return null;
        }

        if (!has_any) return null;
        return Border{
            width: width ?? BorderSideWidth.default(),
            style: style ?? LineStyle.default(),
            color: color ?? BorderColorValue.currentColor(),
        };
    }

    /// Serialize `border` shorthand, omitting default components (emitting `none` when all default).
    pub fn toCss(out: []u8): []const u8 {
        if (_.eql(Border.default())) {
            const len = appendStr(out, 0, _.style.asStr());
            return out[0..len];
        }

        let len: usize = 0;
        let needs_space = false;
        if (!_.width.isDefault()) {
            len = writeSideWidth(out, len, _.width);
            needs_space = true;
        }
        if (!_.style.eql(LineStyle.default())) {
            if (needs_space) len = appendStr(out, len, " ");
            len = appendStr(out, len, _.style.asStr());
            needs_space = true;
        }
        if (!_.color.isDefault()) {
            if (needs_space) len = appendStr(out, len, " ");
            len = writeColor(out, len, _.color);
        }
        return out[0..len];
    }
}

/// CSS `border-width` 4-side shorthand.
pub struct BorderWidth {
    top: BorderSideWidth,
    right: BorderSideWidth,
    bottom: BorderSideWidth,
    left: BorderSideWidth,

    pub fn eql(other: BorderWidth): bool {
        return _.top.eql(other.top) && _.right.eql(other.right) && _.bottom.eql(other.bottom) && _.left.eql(other.left);
    }

    pub static fn parse(input: []const u8): BorderWidth? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        let vals: [4]BorderSideWidth = [BorderSideWidth.default(); 4];
        let count: usize = 0;
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            if (count >= 4) return null;
            vals[count] = BorderSideWidth.parse(s[start..pos]) ?? return null;
            count += 1;
        }
        if (count == 0) return null;
        if (count == 1) return BorderWidth{ top: vals[0], right: vals[0], bottom: vals[0], left: vals[0] };
        if (count == 2) return BorderWidth{ top: vals[0], right: vals[1], bottom: vals[0], left: vals[1] };
        if (count == 3) return BorderWidth{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[1] };
        return BorderWidth{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[3] };
    }

    pub fn toCss(out: []u8): []const u8 {
        let len: usize = 0;
        len = writeSideWidth(out, len, _.top);
        const all_same = _.top.eql(_.right) && _.top.eql(_.bottom) && _.top.eql(_.left);
        if (all_same) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeSideWidth(out, len, _.right);
        if (_.top.eql(_.bottom) && _.right.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeSideWidth(out, len, _.bottom);
        if (_.right.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeSideWidth(out, len, _.left);
        return out[0..len];
    }
}

/// CSS `border-style` 4-side shorthand.
pub struct BorderStyle {
    top: LineStyle,
    right: LineStyle,
    bottom: LineStyle,
    left: LineStyle,

    pub fn eql(other: BorderStyle): bool {
        return _.top == other.top && _.right == other.right && _.bottom == other.bottom && _.left == other.left;
    }

    pub static fn parse(input: []const u8): BorderStyle? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        let vals: [4]LineStyle = [LineStyle.none; 4];
        let count: usize = 0;
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            if (count >= 4) return null;
            vals[count] = LineStyle.parse(s[start..pos]) ?? return null;
            count += 1;
        }
        if (count == 0) return null;
        if (count == 1) return BorderStyle{ top: vals[0], right: vals[0], bottom: vals[0], left: vals[0] };
        if (count == 2) return BorderStyle{ top: vals[0], right: vals[1], bottom: vals[0], left: vals[1] };
        if (count == 3) return BorderStyle{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[1] };
        return BorderStyle{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[3] };
    }

    pub fn toCss(out: []u8): []const u8 {
        let len: usize = 0;
        len = appendStr(out, len, _.top.asStr());
        if (_.top == _.right && _.top == _.bottom && _.top == _.left) return out[0..len];

        len = appendStr(out, len, " ");
        len = appendStr(out, len, _.right.asStr());
        if (_.top == _.bottom && _.right == _.left) return out[0..len];

        len = appendStr(out, len, " ");
        len = appendStr(out, len, _.bottom.asStr());
        if (_.right == _.left) return out[0..len];

        len = appendStr(out, len, " ");
        len = appendStr(out, len, _.left.asStr());
        return out[0..len];
    }
}

/// CSS `border-color` 4-side shorthand.
pub struct BorderColor {
    top: BorderColorValue,
    right: BorderColorValue,
    bottom: BorderColorValue,
    left: BorderColorValue,

    pub fn eql(other: BorderColor): bool {
        return _.top.eql(other.top) && _.right.eql(other.right) && _.bottom.eql(other.bottom) && _.left.eql(other.left);
    }

    pub static fn parse(input: []const u8): BorderColor? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        let vals: [4]BorderColorValue = [BorderColorValue.currentColor(); 4];
        let count: usize = 0;
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            if (count >= 4) return null;
            vals[count] = BorderColorValue.parse(s[start..pos]) ?? return null;
            count += 1;
        }
        if (count == 0) return null;
        if (count == 1) return BorderColor{ top: vals[0], right: vals[0], bottom: vals[0], left: vals[0] };
        if (count == 2) return BorderColor{ top: vals[0], right: vals[1], bottom: vals[0], left: vals[1] };
        if (count == 3) return BorderColor{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[1] };
        return BorderColor{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[3] };
    }

    pub fn toCss(out: []u8): []const u8 {
        let len: usize = 0;
        len = writeColor(out, len, _.top);
        if (_.top.eql(_.right) && _.top.eql(_.bottom) && _.top.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeColor(out, len, _.right);
        if (_.top.eql(_.bottom) && _.right.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeColor(out, len, _.bottom);
        if (_.right.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeColor(out, len, _.left);
        return out[0..len];
    }
}

/// Logical 2-side border width (`border-block-width` / `border-inline-width`).
pub struct BorderLogicalWidth {
    start: BorderSideWidth,
    end: BorderSideWidth,

    pub static fn parse(input: []const u8): BorderLogicalWidth? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        let vals: [2]BorderSideWidth = [BorderSideWidth.default(); 2];
        let count: usize = 0;
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            if (count >= 2) return null;
            vals[count] = BorderSideWidth.parse(s[start..pos]) ?? return null;
            count += 1;
        }
        if (count == 0) return null;
        if (count == 1) return BorderLogicalWidth{ start: vals[0], end: vals[0] };
        return BorderLogicalWidth{ start: vals[0], end: vals[1] };
    }

    pub fn toCss(out: []u8): []const u8 {
        let len = writeSideWidth(out, 0, _.start);
        if (!_.start.eql(_.end)) {
            len = appendStr(out, len, " ");
            len = writeSideWidth(out, len, _.end);
        }
        return out[0..len];
    }
}

// ---------- Internal helpers ----------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && isWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn parseLengthUnit(u: []const u8): BorderLengthUnit? {
    if (eqIgnoreCase(u, "px")) return .px;
    if (eqIgnoreCase(u, "em")) return .em;
    if (eqIgnoreCase(u, "rem")) return .rem;
    if (eqIgnoreCase(u, "pt")) return .pt;
    if (eqIgnoreCase(u, "vw")) return .vw;
    if (eqIgnoreCase(u, "vh")) return .vh;
    return null;
}

fn parseNonNegFloat(s: []const u8): f32? {
    if (s.len == 0) return null;
    let idx: usize = 0;
    if (s[0] == '+') idx = 1;
    let int_part: f64 = 0.0;
    let frac_part: f64 = 0.0;
    let frac_div: f64 = 1.0;
    let in_frac = false;
    let digits: usize = 0;
    while (idx < s.len) : (idx += 1) {
        const c = s[idx];
        if (c >= '0' && c <= '9') {
            const d: f64 = @floatFromInt(c - '0');
            if (in_frac) {
                frac_div *= 10.0;
                frac_part += d / frac_div;
            } else {
                int_part = int_part * 10.0 + d;
            }
            digits += 1;
        } else if (c == '.' && !in_frac) {
            in_frac = true;
        } else {
            return null;
        }
    }
    if (digits == 0) return null;
    return @floatCast(int_part + frac_part);
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn parseHexColor(hex: []const u8): BorderColorValue? {
    if (hex.len == 3) {
        const r = hexVal(hex[0]) ?? return null;
        const g = hexVal(hex[1]) ?? return null;
        const b = hexVal(hex[2]) ?? return null;
        return BorderColorValue.rgb(r * 17, g * 17, b * 17);
    }
    if (hex.len == 4) {
        const r = hexVal(hex[0]) ?? return null;
        const g = hexVal(hex[1]) ?? return null;
        const b = hexVal(hex[2]) ?? return null;
        const a = hexVal(hex[3]) ?? return null;
        return BorderColorValue.rgba(r * 17, g * 17, b * 17, a * 17);
    }
    if (hex.len == 6) {
        const r0 = hexVal(hex[0]) ?? return null;
        const r1 = hexVal(hex[1]) ?? return null;
        const g0 = hexVal(hex[2]) ?? return null;
        const g1 = hexVal(hex[3]) ?? return null;
        const b0 = hexVal(hex[4]) ?? return null;
        const b1 = hexVal(hex[5]) ?? return null;
        return BorderColorValue.rgb((r0 << 4) | r1, (g0 << 4) | g1, (b0 << 4) | b1);
    }
    if (hex.len == 8) {
        const r0 = hexVal(hex[0]) ?? return null;
        const r1 = hexVal(hex[1]) ?? return null;
        const g0 = hexVal(hex[2]) ?? return null;
        const g1 = hexVal(hex[3]) ?? return null;
        const b0 = hexVal(hex[4]) ?? return null;
        const b1 = hexVal(hex[5]) ?? return null;
        const a0 = hexVal(hex[6]) ?? return null;
        const a1 = hexVal(hex[7]) ?? return null;
        return BorderColorValue.rgba((r0 << 4) | r1, (g0 << 4) | g1, (b0 << 4) | b1, (a0 << 4) | a1);
    }
    return null;
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn appendU32(out: []u8, pos: usize, v_in: u32): usize {
    if (v_in == 0) return appendStr(out, pos, "0");
    let buf: [16]u8 = [0; 16];
    let n: usize = 0;
    let v = v_in;
    while (v > 0) : (v /= 10) {
        buf[n] = @intCast('0' + (v % 10));
        n += 1;
    }
    let p = pos;
    while (n > 0 && p < out.len) {
        n -= 1;
        out[p] = buf[n];
        p += 1;
    }
    return p;
}

fn writeSideWidth(out: []u8, pos: usize, w: BorderSideWidth): usize {
    switch (w.kind) {
        .thin => return appendStr(out, pos, "thin"),
        .medium => return appendStr(out, pos, "medium"),
        .thick => return appendStr(out, pos, "thick"),
        .length => {
            if (w.value == 0.0) return appendStr(out, pos, "0");
            const scaled: u32 = @intFromFloat(w.value * 100.0 + 0.5);
            const whole = scaled / 100;
            const frac = scaled % 100;
            let p = appendU32(out, pos, whole);
            if (frac != 0) {
                p = appendStr(out, p, ".");
                if (frac % 10 == 0) {
                    p = appendU32(out, p, frac / 10);
                } else {
                    if (frac < 10) p = appendStr(out, p, "0");
                    p = appendU32(out, p, frac);
                }
            }
            return appendStr(out, p, w.unit.asStr());
        },
    }
}

fn hexChar(n: u8): u8 {
    return if (n < 10) '0' + n else 'a' + (n - 10);
}

fn appendHexByte(out: []u8, pos: usize, b: u8): usize {
    let p = pos;
    if (p + 2 > out.len) return p;
    out[p] = hexChar((b >> 4) & 0xF);
    out[p + 1] = hexChar(b & 0xF);
    return p + 2;
}

fn writeColor(out: []u8, pos: usize, c: BorderColorValue): usize {
    switch (c.kind) {
        .current_color => return appendStr(out, pos, "currentcolor"),
        .transparent => return appendStr(out, pos, "transparent"),
        .rgba => {
            let p = appendStr(out, pos, "#");
            p = appendHexByte(out, p, c.r);
            p = appendHexByte(out, p, c.g);
            p = appendHexByte(out, p, c.b);
            if (c.a != 255) {
                p = appendHexByte(out, p, c.a);
            }
            return p;
        },
    }
}

fn isBorderIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

fn isBorderIdentChar(c: u8): bool {
    return isBorderIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
}

fn isBorderWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0c;
}

fn consumeBalancedVarEnvRest(s: []const u8, start: usize): bool {
    let i = start;
    let p_depth: usize = 1;
    let b_depth: usize = 0;
    let c_depth: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch == '(') p_depth += 1;
        else if (ch == ')') {
            p_depth -= 1;
            if (p_depth == 0) return (b_depth == 0 && c_depth == 0 && i + 1 == s.len);
        } else if (ch == '[') b_depth += 1;
        else if (ch == ']') {
            if (b_depth == 0) return false;
            b_depth -= 1;
        } else if (ch == '{') c_depth += 1;
        else if (ch == '}') {
            if (c_depth == 0) return false;
            c_depth -= 1;
        }
        i += 1;
    }
    return false;
}

fn isVarOrEnvToken(s: []const u8): bool {
    if (s.len >= 5 && eqIgnoreCase(s[0..4], "var(")) {
        let i: usize = 4;
        while (i < s.len && isBorderWs(s[i])) i += 1;
        if (i + 2 >= s.len || s[i] != '-' || s[i + 1] != '-') return false;
        i += 2;
        const name_start = i;
        while (i < s.len && isBorderIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len && isBorderWs(s[i])) i += 1;
        if (i >= s.len) return false;
        if (s[i] == ')') return i + 1 == s.len;
        if (s[i] != ',') return false;
        return consumeBalancedVarEnvRest(s, i + 1);
    }
    if (s.len >= 5 && eqIgnoreCase(s[0..4], "env(")) {
        let i: usize = 4;
        while (i < s.len && isBorderWs(s[i])) i += 1;
        if (i >= s.len || (!isBorderIdentStart(s[i]) && s[i] != '-')) return false;
        const name_start = i;
        while (i < s.len && isBorderIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len) {
            while (i < s.len && isBorderWs(s[i])) i += 1;
            if (i >= s.len) return false;
            if (s[i] == ')') return i + 1 == s.len;
            if (s[i] == ',') return consumeBalancedVarEnvRest(s, i + 1);
            if (s[i] >= '0' && s[i] <= '9') {
                while (i < s.len && s[i] >= '0' && s[i] <= '9') i += 1;
                continue;
            }
            return false;
        }
        return false;
    }
    return false;
}

fn isValidBorderWidthToken(tok: []const u8): bool {
    if (eqIgnoreCase(tok, "thin") || eqIgnoreCase(tok, "medium") || eqIgnoreCase(tok, "thick")) {
        return true;
    }
    if (isVarOrEnvToken(tok)) return true;
    if (std.mem.indexOfScalar(u8, tok, '%') != null) return false;
    return isValidCssLenPct(tok, false);
}

fn isValidBorderStyleToken(tok: []const u8): bool {
    if (isVarOrEnvToken(tok)) return true;
    return LineStyle.parse(tok) != null;
}

fn isValidBorderTokenList(val: []const u8, max_tokens: usize, is_width: bool): bool {
    if (max_tokens == 0) return false;
    const s = std.mem.trim(u8, val, " \t\r\n\x0c");
    if (s.len == 0) return false;

    let i: usize = 0;
    let tok_count: usize = 0;
    while (i < s.len) {
        while (i < s.len && isBorderWs(s[i])) : (i += 1) {}
        if (i >= s.len) break;

        const tok_start = i;
        let p_depth: usize = 0;
        let b_depth: usize = 0;
        let c_depth: usize = 0;
        while (i < s.len) {
            const ch = s[i];
            if (p_depth == 0 && b_depth == 0 && c_depth == 0 && isBorderWs(ch)) {
                break;
            }
            if (ch == '(') p_depth += 1;
            else if (ch == ')') {
                if (p_depth == 0) return false;
                p_depth -= 1;
            } else if (ch == '[') b_depth += 1;
            else if (ch == ']') {
                if (b_depth == 0) return false;
                b_depth -= 1;
            } else if (ch == '{') c_depth += 1;
            else if (ch == '}') {
                if (c_depth == 0) return false;
                c_depth -= 1;
            }
            i += 1;
        }
        if (p_depth != 0 || b_depth != 0 || c_depth != 0) return false;

        const tok = s[tok_start..i];
        tok_count += 1;
        if (tok_count > max_tokens) return false;
        if (is_width) {
            if (!isValidBorderWidthToken(tok)) return false;
        } else {
            if (!isValidBorderStyleToken(tok)) return false;
        }
    }

    return tok_count >= 1 && tok_count <= max_tokens;
}

/// Zero-allocation validator for CSS `border-width` / `outline-width` values.
pub fn isValidCssBorderWidthValue(val: []const u8, max_tokens: usize): bool {
    return isValidBorderTokenList(val, max_tokens, true);
}

/// Zero-allocation validator for CSS `border-style` values.
pub fn isValidCssBorderStyleValue(val: []const u8, max_tokens: usize): bool {
    return isValidBorderTokenList(val, max_tokens, false);
}
