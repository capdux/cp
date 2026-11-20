// Source: /data/dev/bun/src/css/properties/properties.cp
// lib/css/engine/properties.cp
// LightningCSS Shorthand Property Expander & Minifier:
// RectShorthand, BorderShorthand, BorderRadius, BoxShadow, TransformList (+ Matrix2D), FlexShorthand, Transition, Animation, Display, FilterList

const std = @import("std");
import { CssExpandedDecl } from "../prefixes.cp";
import { Angle, Time, EasingFunction, CssRect } from "./values.cp";
import { engAppendBytes, engFormatFloat, engSplitCommaTop, engSplitSpaceTop } from "./_utils.cp";

fn isCssLengthOrZero(tok: []const u8): bool {
    if (tok.len == 0) return false;
    if (std.mem.eql(u8, tok, "0")) return true;
    if (std.ascii.startsWithIgnoreCase(tok, "calc(") || std.ascii.startsWithIgnoreCase(tok, "var(")) return true;
    if (std.ascii.eqlIgnoreCase(tok, "thin") || std.ascii.eqlIgnoreCase(tok, "medium") || std.ascii.eqlIgnoreCase(tok, "thick")) return true;
    const c = tok[0];
    if (!((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+')) return false;
    let has_alpha_or_pct = false;
    for (ch of tok) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '%') {
            has_alpha_or_pct = true;
            break;
        }
    }
    return has_alpha_or_pct;
}


// ============================================================================
// 1. RectShorthand (margin, padding, inset, border-width/style/color)
// ============================================================================

pub struct RectShorthand {
    pub static fn isRectShorthand(prop_name: []const u8): bool {
        return std.ascii.eqlIgnoreCase(prop_name, "margin") ||
            std.ascii.eqlIgnoreCase(prop_name, "padding") ||
            std.ascii.eqlIgnoreCase(prop_name, "inset") ||
            std.ascii.eqlIgnoreCase(prop_name, "border-width") ||
            std.ascii.eqlIgnoreCase(prop_name, "border-style") ||
            std.ascii.eqlIgnoreCase(prop_name, "border-color") ||
            std.ascii.eqlIgnoreCase(prop_name, "scroll-margin") ||
            std.ascii.eqlIgnoreCase(prop_name, "scroll-padding");
    }

    /// Expands a 1/2/3/4-value rect shorthand into 4 longhand declarations.
    pub static fn expand(
        prop_name: []const u8,
        raw_val: []const u8,
        important: bool,
        out_decls: []CssExpandedDecl,
    ): usize {
        if (out_decls.len < 4) return 0;
        const rect = CssRect.parse(raw_val) ?? return 0;

        if (std.ascii.eqlIgnoreCase(prop_name, "margin")) {
            out_decls[0] = CssExpandedDecl { name: "margin-top", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "margin-right", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "margin-bottom", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "margin-left", value: rect.left, important: important };
            return 4;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "padding")) {
            out_decls[0] = CssExpandedDecl { name: "padding-top", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "padding-right", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "padding-bottom", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "padding-left", value: rect.left, important: important };
            return 4;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "inset")) {
            out_decls[0] = CssExpandedDecl { name: "top", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "right", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "bottom", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "left", value: rect.left, important: important };
            return 4;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "border-width")) {
            out_decls[0] = CssExpandedDecl { name: "border-top-width", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-right-width", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-bottom-width", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "border-left-width", value: rect.left, important: important };
            return 4;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "border-style")) {
            out_decls[0] = CssExpandedDecl { name: "border-top-style", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-right-style", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-bottom-style", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "border-left-style", value: rect.left, important: important };
            return 4;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "border-color")) {
            out_decls[0] = CssExpandedDecl { name: "border-top-color", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-right-color", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-bottom-color", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "border-left-color", value: rect.left, important: important };
            return 4;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "scroll-margin")) {
            out_decls[0] = CssExpandedDecl { name: "scroll-margin-top", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "scroll-margin-right", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "scroll-margin-bottom", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "scroll-margin-left", value: rect.left, important: important };
            return 4;
        }
        if (std.ascii.eqlIgnoreCase(prop_name, "scroll-padding")) {
            out_decls[0] = CssExpandedDecl { name: "scroll-padding-top", value: rect.top, important: important };
            out_decls[1] = CssExpandedDecl { name: "scroll-padding-right", value: rect.right, important: important };
            out_decls[2] = CssExpandedDecl { name: "scroll-padding-bottom", value: rect.bottom, important: important };
            out_decls[3] = CssExpandedDecl { name: "scroll-padding-left", value: rect.left, important: important };
            return 4;
        }
        return 0;
    }

    pub static fn minifyValue(raw_val: []const u8, out_buf: []u8): []const u8 {
        const rect = CssRect.parse(raw_val) ?? return raw_val;
        return rect.format(out_buf);
    }

    pub static fn minifyFour(
        top: []const u8,
        right: []const u8,
        bottom: []const u8,
        left: []const u8,
        out_buf: []u8,
    ): []const u8 {
        const rect = CssRect { top: top, right: right, bottom: bottom, left: left };
        return rect.format(out_buf);
    }
}

// ============================================================================
// 2. BorderShorthand (border: <width> || <style> || <color>)
// ============================================================================

pub enum BorderStyle {
    None,
    Hidden,
    Dotted,
    Dashed,
    Solid,
    Double,
    Groove,
    Ridge,
    Inset,
    Outset,

    static fn parse(s: []const u8): BorderStyle? {
        if (std.ascii.eqlIgnoreCase(s, "none")) return BorderStyle.None;
        if (std.ascii.eqlIgnoreCase(s, "hidden")) return BorderStyle.Hidden;
        if (std.ascii.eqlIgnoreCase(s, "dotted")) return BorderStyle.Dotted;
        if (std.ascii.eqlIgnoreCase(s, "dashed")) return BorderStyle.Dashed;
        if (std.ascii.eqlIgnoreCase(s, "solid")) return BorderStyle.Solid;
        if (std.ascii.eqlIgnoreCase(s, "double")) return BorderStyle.Double;
        if (std.ascii.eqlIgnoreCase(s, "groove")) return BorderStyle.Groove;
        if (std.ascii.eqlIgnoreCase(s, "ridge")) return BorderStyle.Ridge;
        if (std.ascii.eqlIgnoreCase(s, "inset")) return BorderStyle.Inset;
        if (std.ascii.eqlIgnoreCase(s, "outset")) return BorderStyle.Outset;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .None => "none",
            .Hidden => "hidden",
            .Dotted => "dotted",
            .Dashed => "dashed",
            .Solid => "solid",
            .Double => "double",
            .Groove => "groove",
            .Ridge => "ridge",
            .Inset => "inset",
            .Outset => "outset",
        };
    }
}

pub struct BorderShorthand {
    width: []const u8 = "medium";
    style: BorderStyle = BorderStyle.None;
    color: []const u8 = "currentColor";
    has_width: bool = false;
    has_style: bool = false;
    has_color: bool = false;

    pub static fn parse(raw: []const u8): BorderShorthand? {
        let parts: [4][]const u8 = ["", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..4]);
        if (n == 0 || n > 3) return null;

        let res = BorderShorthand {};
        let i: usize = 0;
        while (i < n) {
            const tok = parts[i];
            if (!res.has_style) {
                if (st := BorderStyle.parse(tok)) {
                    res.style = st;
                    res.has_style = true;
                    i += 1;
                    continue;
                }
            }
            if (!res.has_width && isCssLengthOrZero(tok)) {
                res.width = tok;
                res.has_width = true;
                i += 1;
                continue;
            }
            if (!res.has_color) {
                res.color = tok;
                res.has_color = true;
                i += 1;
                continue;
            }
            return null;
        }
        return res;
    }

    pub fn expandToLonghands(side: []const u8, important: bool, out_decls: []CssExpandedDecl): usize {
        if (out_decls.len < 3) return 0;
        const w = if (_.has_width) _.width else "medium";
        const st = _.style.asStr();
        const col = if (_.has_color) _.color else "currentColor";

        if (std.ascii.eqlIgnoreCase(side, "top")) {
            out_decls[0] = CssExpandedDecl { name: "border-top-width", value: w, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-top-style", value: st, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-top-color", value: col, important: important };
        } else if (std.ascii.eqlIgnoreCase(side, "right")) {
            out_decls[0] = CssExpandedDecl { name: "border-right-width", value: w, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-right-style", value: st, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-right-color", value: col, important: important };
        } else if (std.ascii.eqlIgnoreCase(side, "bottom")) {
            out_decls[0] = CssExpandedDecl { name: "border-bottom-width", value: w, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-bottom-style", value: st, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-bottom-color", value: col, important: important };
        } else if (std.ascii.eqlIgnoreCase(side, "left")) {
            out_decls[0] = CssExpandedDecl { name: "border-left-width", value: w, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-left-style", value: st, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-left-color", value: col, important: important };
        } else {
            out_decls[0] = CssExpandedDecl { name: "border-width", value: w, important: important };
            out_decls[1] = CssExpandedDecl { name: "border-style", value: st, important: important };
            out_decls[2] = CssExpandedDecl { name: "border-color", value: col, important: important };
        }
        return 3;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (minify && !_.has_width && (!_.has_style || _.style == BorderStyle.None) && !_.has_color) {
            return std.fmt.bufPrint(out_buf, "none", .{}) catch "";
        }
        let pos: usize = 0;
        let wrote = false;
        if (_.has_width && !(minify && std.ascii.eqlIgnoreCase(_.width, "medium") && _.has_style)) {
            pos = engAppendBytes(out_buf, pos, _.width);
            wrote = true;
        }
        if (_.has_style) {
            if (wrote) pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.style.asStr());
            wrote = true;
        }
        if (_.has_color && !(minify && std.ascii.eqlIgnoreCase(_.color, "currentcolor"))) {
            if (wrote) pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.color);
            wrote = true;
        }
        if (!wrote) return std.fmt.bufPrint(out_buf, "none", .{}) catch "";
        return out_buf[0..pos];
    }
}

// ============================================================================
// 3. BorderRadius (with optional `/` elliptical vertical radii)
// ============================================================================

pub struct BorderRadius {
    horiz: CssRect = CssRect {};
    vert: CssRect = CssRect {};

    pub static fn parse(raw: []const u8): BorderRadius? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (slash := std.mem.indexOfScalar(u8, s, '/')) {
            const h_rect = CssRect.parse(s[0..slash]) ?? return null;
            const v_rect = CssRect.parse(s[slash + 1 ..]) ?? return null;
            return BorderRadius { horiz: h_rect, vert: v_rect };
        }
        const h_rect = CssRect.parse(s) ?? return null;
        return BorderRadius { horiz: h_rect, vert: h_rect };
    }

    pub fn isElliptical(): bool {
        return !std.mem.eql(u8, _.horiz.top, _.vert.top) ||
            !std.mem.eql(u8, _.horiz.right, _.vert.right) ||
            !std.mem.eql(u8, _.horiz.bottom, _.vert.bottom) ||
            !std.mem.eql(u8, _.horiz.left, _.vert.left);
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let hb: [128]u8 = [0; 128];
        const hs = _.horiz.format(hb[0..128]);
        if (!_.isElliptical()) {
            return std.fmt.bufPrint(out_buf, "{s}", .{hs}) catch "";
        }
        let vb: [128]u8 = [0; 128];
        const vs = _.vert.format(vb[0..128]);
        if (minify) {
            return std.fmt.bufPrint(out_buf, "{s}/{s}", .{ hs, vs }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{s} / {s}", .{ hs, vs }) catch "";
    }
}

// ============================================================================
// 4. BoxShadow & BoxShadowList
// ============================================================================

pub struct BoxShadow {
    x_offset: []const u8 = "0";
    y_offset: []const u8 = "0";
    blur: []const u8 = "0";
    spread: []const u8 = "0";
    color: []const u8 = "";
    inset: bool = false;

    pub static fn parse(raw: []const u8): BoxShadow? {
        let parts: [8][]const u8 = ["", "", "", "", "", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..8]);
        if (n == 0) return null;

        let bs = BoxShadow {};
        let lengths: [4][]const u8 = ["0", "0", "0", "0"];
        let len_count: usize = 0;

        let i: usize = 0;
        while (i < n) {
            const tok = parts[i];
            if (!bs.inset && std.ascii.eqlIgnoreCase(tok, "inset")) {
                bs.inset = true;
            } else if (len_count < 4 && isCssLengthOrZero(tok)) {
                lengths[len_count] = tok;
                len_count += 1;
            } else if (bs.color.len == 0) {
                bs.color = tok;
            } else {
                return null;
            }
            i += 1;
        }
        if (len_count < 2) return null;
        bs.x_offset = lengths[0];
        bs.y_offset = lengths[1];
        if (len_count >= 3) bs.blur = lengths[2];
        if (len_count >= 4) bs.spread = lengths[3];
        return bs;
    }

    pub fn format(out_buf: []u8): []const u8 {
        let pos: usize = 0;
        if (_.inset) pos = engAppendBytes(out_buf, pos, "inset ");
        pos = engAppendBytes(out_buf, pos, _.x_offset);
        pos = engAppendBytes(out_buf, pos, " ");
        pos = engAppendBytes(out_buf, pos, _.y_offset);
        const zero_blur = std.mem.eql(u8, _.blur, "0") || std.mem.eql(u8, _.blur, "0px");
        const zero_spread = std.mem.eql(u8, _.spread, "0") || std.mem.eql(u8, _.spread, "0px");
        if (!zero_blur || !zero_spread) {
            pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.blur);
            if (!zero_spread) {
                pos = engAppendBytes(out_buf, pos, " ");
                pos = engAppendBytes(out_buf, pos, _.spread);
            }
        }
        if (_.color.len > 0) {
            pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.color);
        }
        return out_buf[0..pos];
    }
}

pub struct BoxShadowList {
    items: [8]BoxShadow = std.mem.zeroes([8]BoxShadow);
    count: usize = 0;
    is_none: bool = false;

    pub static fn parse(raw: []const u8): BoxShadowList? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) {
            return BoxShadowList { is_none: true };
        }
        let parts: [8][]const u8 = [""; 8];
        const n = engSplitCommaTop(s, parts[0..8]);
        if (n == 0) return null;
        let list = BoxShadowList {};
        let i: usize = 0;
        while (i < n && list.count < 8) {
            const bs = BoxShadow.parse(parts[i]) ?? return null;
            list.items[list.count] = bs;
            list.count += 1;
            i += 1;
        }
        return list;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.is_none) return std.fmt.bufPrint(out_buf, "none", .{}) catch "";
        let pos: usize = 0;
        let sb: [128]u8 = [0; 128];
        const sep: []const u8 = if (minify) "," else ", ";
        let i: usize = 0;
        while (i < _.count) {
            if (i > 0) pos = engAppendBytes(out_buf, pos, sep);
            pos = engAppendBytes(out_buf, pos, _.items[i].format(sb[0..128]));
            i += 1;
        }
        return out_buf[0..pos];
    }
}

// ============================================================================
// 5. Matrix2D & TransformList
// ============================================================================

pub struct Point2D {
    x: f64 = 0.0;
    y: f64 = 0.0;
}

pub struct Matrix2D {
    a: f64 = 1.0;
    b: f64 = 0.0;
    c: f64 = 0.0;
    d: f64 = 1.0;
    e: f64 = 0.0;
    f: f64 = 0.0;

    pub static fn identity(): Matrix2D {
        return Matrix2D {};
    }

    pub fn multiply(rhs: Matrix2D): Matrix2D {
        return Matrix2D {
            a: _.a * rhs.a + _.c * rhs.b,
            b: _.b * rhs.a + _.d * rhs.b,
            c: _.a * rhs.c + _.c * rhs.d,
            d: _.b * rhs.c + _.d * rhs.d,
            e: _.a * rhs.e + _.c * rhs.f + _.e,
            f: _.b * rhs.e + _.d * rhs.f + _.f,
        };
    }

    pub static fn translate(tx: f64, ty: f64): Matrix2D {
        return Matrix2D { e: tx, f: ty };
    }

    pub static fn scale(sx: f64, sy: f64): Matrix2D {
        return Matrix2D { a: sx, d: sy };
    }

    pub static fn rotateDeg(deg: f64): Matrix2D {
        const r = deg * (std.math.pi / 180.0);
        const cs = @cos(r);
        const sn = @sin(r);
        return Matrix2D { a: cs, b: sn, c: -sn, d: cs };
    }

    pub static fn skewDeg(ax: f64, ay: f64): Matrix2D {
        const tx = @tan(ax * (std.math.pi / 180.0));
        const ty = @tan(ay * (std.math.pi / 180.0));
        return Matrix2D { c: tx, b: ty };
    }

    pub fn applyPoint(x: f64, y: f64): Point2D {
        return Point2D {
            x: _.a * x + _.c * y + _.e,
            y: _.b * x + _.d * y + _.f,
        };
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let b1: [32]u8 = [0; 32];
        let b2: [32]u8 = [0; 32];
        let b3: [32]u8 = [0; 32];
        let b4: [32]u8 = [0; 32];
        let b5: [32]u8 = [0; 32];
        let b6: [32]u8 = [0; 32];
        const s1 = engFormatFloat(_.a, b1[0..32]);
        const s2 = engFormatFloat(_.b, b2[0..32]);
        const s3 = engFormatFloat(_.c, b3[0..32]);
        const s4 = engFormatFloat(_.d, b4[0..32]);
        const s5 = engFormatFloat(_.e, b5[0..32]);
        const s6 = engFormatFloat(_.f, b6[0..32]);
        if (minify) {
            return std.fmt.bufPrint(out_buf, "matrix({s},{s},{s},{s},{s},{s})", .{ s1, s2, s3, s4, s5, s6 }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "matrix({s}, {s}, {s}, {s}, {s}, {s})", .{ s1, s2, s3, s4, s5, s6 }) catch "";
    }
}

pub enum TransformKind {
    Translate,
    TranslateX,
    TranslateY,
    TranslateZ,
    Translate3d,
    Scale,
    ScaleX,
    ScaleY,
    ScaleZ,
    Scale3d,
    Rotate,
    RotateX,
    RotateY,
    RotateZ,
    Skew,
    SkewX,
    SkewY,
    Matrix,
    Matrix3d,
    Perspective,
}

pub struct TransformOp {
    kind: TransformKind = TransformKind.Translate;
    args: [16][]const u8 = [""; 16];
    arg_count: usize = 0;

    fn fnName(): []const u8 {
        return switch (_.kind) {
            .Translate => "translate",
            .TranslateX => "translateX",
            .TranslateY => "translateY",
            .TranslateZ => "translateZ",
            .Translate3d => "translate3d",
            .Scale => "scale",
            .ScaleX => "scaleX",
            .ScaleY => "scaleY",
            .ScaleZ => "scaleZ",
            .Scale3d => "scale3d",
            .Rotate => "rotate",
            .RotateX => "rotateX",
            .RotateY => "rotateY",
            .RotateZ => "rotateZ",
            .Skew => "skew",
            .SkewX => "skewX",
            .SkewY => "skewY",
            .Matrix => "matrix",
            .Matrix3d => "matrix3d",
            .Perspective => "perspective",
        };
    }

    pub static fn parse(raw: []const u8): TransformOp? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        const open = std.mem.indexOfScalar(u8, s, '(') ?? return null;
        if (s[s.len - 1] != ')') return null;
        const name = std.mem.trim(u8, s[0..open], " \t");
        const inner = s[open + 1 .. s.len - 1];

        let kind = TransformKind.Translate;
        if (std.ascii.eqlIgnoreCase(name, "translate")) kind = TransformKind.Translate
        else if (std.ascii.eqlIgnoreCase(name, "translateX")) kind = TransformKind.TranslateX
        else if (std.ascii.eqlIgnoreCase(name, "translateY")) kind = TransformKind.TranslateY
        else if (std.ascii.eqlIgnoreCase(name, "translateZ")) kind = TransformKind.TranslateZ
        else if (std.ascii.eqlIgnoreCase(name, "translate3d")) kind = TransformKind.Translate3d
        else if (std.ascii.eqlIgnoreCase(name, "scale")) kind = TransformKind.Scale
        else if (std.ascii.eqlIgnoreCase(name, "scaleX")) kind = TransformKind.ScaleX
        else if (std.ascii.eqlIgnoreCase(name, "scaleY")) kind = TransformKind.ScaleY
        else if (std.ascii.eqlIgnoreCase(name, "scaleZ")) kind = TransformKind.ScaleZ
        else if (std.ascii.eqlIgnoreCase(name, "scale3d")) kind = TransformKind.Scale3d
        else if (std.ascii.eqlIgnoreCase(name, "rotate")) kind = TransformKind.Rotate
        else if (std.ascii.eqlIgnoreCase(name, "rotateX")) kind = TransformKind.RotateX
        else if (std.ascii.eqlIgnoreCase(name, "rotateY")) kind = TransformKind.RotateY
        else if (std.ascii.eqlIgnoreCase(name, "rotateZ")) kind = TransformKind.RotateZ
        else if (std.ascii.eqlIgnoreCase(name, "skew")) kind = TransformKind.Skew
        else if (std.ascii.eqlIgnoreCase(name, "skewX")) kind = TransformKind.SkewX
        else if (std.ascii.eqlIgnoreCase(name, "skewY")) kind = TransformKind.SkewY
        else if (std.ascii.eqlIgnoreCase(name, "matrix")) kind = TransformKind.Matrix
        else if (std.ascii.eqlIgnoreCase(name, "matrix3d")) kind = TransformKind.Matrix3d
        else if (std.ascii.eqlIgnoreCase(name, "perspective")) kind = TransformKind.Perspective
        else return null;

        let op = TransformOp { kind: kind };
        op.arg_count = engSplitCommaTop(inner, op.args[0..16]);
        if (op.arg_count == 0) return null;
        return op;
    }

    fn parsePxOrZero(s: []const u8): f64? {
        const t = std.mem.trim(u8, s, " \t");
        if (std.mem.eql(u8, t, "0")) return 0.0;
        if (t.len > 2 && std.ascii.eqlIgnoreCase(t[t.len - 2 ..], "px")) {
            return std.fmt.parseFloat(f64, t[0 .. t.len - 2]) catch null;
        }
        return null;
    }

    pub fn toMatrix2D(): Matrix2D? {
        switch (_.kind) {
            .Translate => {
                const tx = TransformOp.parsePxOrZero(_.args[0]) ?? return null;
                const ty = if (_.arg_count >= 2) (TransformOp.parsePxOrZero(_.args[1]) ?? return null) else 0.0;
                return Matrix2D.translate(tx, ty);
            },
            .TranslateX => {
                const tx = TransformOp.parsePxOrZero(_.args[0]) ?? return null;
                return Matrix2D.translate(tx, 0.0);
            },
            .TranslateY => {
                const ty = TransformOp.parsePxOrZero(_.args[0]) ?? return null;
                return Matrix2D.translate(0.0, ty);
            },
            .Scale => {
                const sx = std.fmt.parseFloat(f64, _.args[0]) catch return null;
                const sy = if (_.arg_count >= 2) (std.fmt.parseFloat(f64, _.args[1]) catch return null) else sx;
                return Matrix2D.scale(sx, sy);
            },
            .ScaleX => {
                const sx = std.fmt.parseFloat(f64, _.args[0]) catch return null;
                return Matrix2D.scale(sx, 1.0);
            },
            .ScaleY => {
                const sy = std.fmt.parseFloat(f64, _.args[0]) catch return null;
                return Matrix2D.scale(1.0, sy);
            },
            .Rotate, .RotateZ => {
                const ang = Angle.parse(_.args[0]) ?? return null;
                return Matrix2D.rotateDeg(ang.toDegrees());
            },
            .Skew => {
                const ax = Angle.parse(_.args[0]) ?? return null;
                const ay = if (_.arg_count >= 2) (Angle.parse(_.args[1]) ?? return null) else Angle.zero();
                return Matrix2D.skewDeg(ax.toDegrees(), ay.toDegrees());
            },
            .SkewX => {
                const ax = Angle.parse(_.args[0]) ?? return null;
                return Matrix2D.skewDeg(ax.toDegrees(), 0.0);
            },
            .SkewY => {
                const ay = Angle.parse(_.args[0]) ?? return null;
                return Matrix2D.skewDeg(0.0, ay.toDegrees());
            },
            .Matrix => {
                if (_.arg_count != 6) return null;
                return Matrix2D {
                    a: std.fmt.parseFloat(f64, _.args[0]) catch return null,
                    b: std.fmt.parseFloat(f64, _.args[1]) catch return null,
                    c: std.fmt.parseFloat(f64, _.args[2]) catch return null,
                    d: std.fmt.parseFloat(f64, _.args[3]) catch return null,
                    e: std.fmt.parseFloat(f64, _.args[4]) catch return null,
                    f: std.fmt.parseFloat(f64, _.args[5]) catch return null,
                };
            },
            else => return null,
        }
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let pos: usize = 0;
        pos = engAppendBytes(out_buf, pos, _.fnName());
        pos = engAppendBytes(out_buf, pos, "(");
        const sep: []const u8 = if (minify) "," else ", ";
        let i: usize = 0;
        while (i < _.arg_count) {
            if (i > 0) pos = engAppendBytes(out_buf, pos, sep);
            pos = engAppendBytes(out_buf, pos, _.args[i]);
            i += 1;
        }
        pos = engAppendBytes(out_buf, pos, ")");
        return out_buf[0..pos];
    }
}

pub struct TransformList {
    ops: [16]TransformOp = std.mem.zeroes([16]TransformOp);
    count: usize = 0;
    is_none: bool = false;

    pub static fn parse(raw: []const u8): TransformList? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) return TransformList { is_none: true };

        let tokens: [16][]const u8 = [""; 16];
        const n = engSplitSpaceTop(s, tokens[0..16]);
        if (n == 0) return null;

        let list = TransformList {};
        let i: usize = 0;
        while (i < n && list.count < 16) {
            const op = TransformOp.parse(tokens[i]) ?? return null;
            list.ops[list.count] = op;
            list.count += 1;
            i += 1;
        }
        return list;
    }

    /// Composes all 2D transforms in the list into a single Matrix2D if all ops are 2D resolvable.
    pub fn toMatrix2D(): Matrix2D? {
        if (_.is_none) return Matrix2D.identity();
        let acc = Matrix2D.identity();
        let i: usize = 0;
        while (i < _.count) {
            const m = _.ops[i].toMatrix2D() ?? return null;
            acc = acc.multiply(m);
            i += 1;
        }
        return acc;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.is_none) return std.fmt.bufPrint(out_buf, "none", .{}) catch "";
        let pos: usize = 0;
        let ob: [128]u8 = [0; 128];
        let i: usize = 0;
        while (i < _.count) {
            if (i > 0 && !minify) pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.ops[i].format(ob[0..128], minify));
            i += 1;
        }
        return out_buf[0..pos];
    }
}

// ============================================================================
// 6. FlexShorthand, Transition, Animation, Display, FilterList
// ============================================================================

pub struct FlexShorthand {
    grow: f64 = 0.0;
    shrink: f64 = 1.0;
    basis: []const u8 = "auto";

    pub static fn parse(raw: []const u8): FlexShorthand? {
        let parts: [4][]const u8 = ["", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..4]);
        if (n == 0 || n > 3) return null;

        if (n == 1) {
            if (std.ascii.eqlIgnoreCase(parts[0], "none")) return FlexShorthand { grow: 0.0, shrink: 0.0, basis: "auto" };
            if (std.ascii.eqlIgnoreCase(parts[0], "auto")) return FlexShorthand { grow: 1.0, shrink: 1.0, basis: "auto" };
            if (std.ascii.eqlIgnoreCase(parts[0], "initial")) return FlexShorthand { grow: 0.0, shrink: 1.0, basis: "auto" };
            const g = std.fmt.parseFloat(f64, parts[0]) catch {
                // Single flex-basis value (e.g. `flex: 200px` -> `1 1 200px`)
                return FlexShorthand { grow: 1.0, shrink: 1.0, basis: parts[0] };
            };
            if (g < 0.0) return null;
            return FlexShorthand { grow: g, shrink: 1.0, basis: "0%" };
        }

        if (n == 2) {
            const g = std.fmt.parseFloat(f64, parts[0]) catch return null;
            if (g < 0.0) return null;
            if (sh := std.fmt.parseFloat(f64, parts[1]) catch null) {
                if (sh < 0.0) return null;
                return FlexShorthand { grow: g, shrink: sh, basis: "0%" };
            }
            return FlexShorthand { grow: g, shrink: 1.0, basis: parts[1] };
        }

        const g = std.fmt.parseFloat(f64, parts[0]) catch return null;
        const sh = std.fmt.parseFloat(f64, parts[1]) catch return null;
        if (g < 0.0 || sh < 0.0) return null;
        return FlexShorthand { grow: g, shrink: sh, basis: parts[2] };
    }

    pub fn expandToLonghands(important: bool, out_decls: []CssExpandedDecl, scratch_buf: []u8): usize {
        if (out_decls.len < 3) return 0;
        let g_buf: [32]u8 = [0; 32];
        let s_buf: [32]u8 = [0; 32];
        const gs = engFormatFloat(_.grow, g_buf[0..32]);
        const ss = engFormatFloat(_.shrink, s_buf[0..32]);
        const g_out = std.fmt.bufPrint(scratch_buf, "{s}", .{gs}) catch "0";
        const s_out = std.fmt.bufPrint(scratch_buf[g_out.len..], "{s}", .{ss}) catch "1";
        out_decls[0] = CssExpandedDecl { name: "flex-grow", value: g_out, important: important };
        out_decls[1] = CssExpandedDecl { name: "flex-shrink", value: s_out, important: important };
        out_decls[2] = CssExpandedDecl { name: "flex-basis", value: _.basis, important: important };
        return 3;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        const is_zero_basis = std.mem.eql(u8, _.basis, "0%") || std.mem.eql(u8, _.basis, "0px") || std.mem.eql(u8, _.basis, "0");
        if (minify) {
            if (_.grow == 0.0 && _.shrink == 0.0 && std.ascii.eqlIgnoreCase(_.basis, "auto")) {
                return std.fmt.bufPrint(out_buf, "none", .{}) catch "";
            }
            if (_.grow == 1.0 && _.shrink == 1.0 && std.ascii.eqlIgnoreCase(_.basis, "auto")) {
                return std.fmt.bufPrint(out_buf, "auto", .{}) catch "";
            }
            if (_.grow == 0.0 && _.shrink == 1.0 && std.ascii.eqlIgnoreCase(_.basis, "auto")) {
                return std.fmt.bufPrint(out_buf, "initial", .{}) catch "";
            }
            if (_.shrink == 1.0 && is_zero_basis) {
                return engFormatFloat(_.grow, out_buf);
            }
        }
        let gb: [32]u8 = [0; 32];
        let sb: [32]u8 = [0; 32];
        const gs = engFormatFloat(_.grow, gb[0..32]);
        const ss = engFormatFloat(_.shrink, sb[0..32]);
        if (minify && is_zero_basis) {
            return std.fmt.bufPrint(out_buf, "{s} {s}", .{ gs, ss }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{s} {s} {s}", .{ gs, ss, _.basis }) catch "";
    }
}

pub struct TransitionItem {
    property: []const u8 = "all";
    duration: Time = Time.zero();
    timing_function: EasingFunction = EasingFunction.ease();
    delay: Time = Time.zero();

    pub static fn parse(raw: []const u8): TransitionItem? {
        let parts: [6][]const u8 = ["", "", "", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..6]);
        if (n == 0) return null;

        let item = TransitionItem {};
        let got_dur = false;
        let got_delay = false;
        let got_ease = false;
        let got_prop = false;

        let i: usize = 0;
        while (i < n) {
            const tok = parts[i];
            if (!got_dur) {
                if (t := Time.parse(tok)) {
                    item.duration = t;
                    got_dur = true;
                    i += 1;
                    continue;
                }
            }
            if (!got_ease) {
                if (e := EasingFunction.parse(tok)) {
                    item.timing_function = e;
                    got_ease = true;
                    i += 1;
                    continue;
                }
            }
            if (!got_delay) {
                if (t := Time.parse(tok)) {
                    item.delay = t;
                    got_delay = true;
                    i += 1;
                    continue;
                }
            }
            if (!got_prop) {
                item.property = tok;
                got_prop = true;
                i += 1;
                continue;
            }
            return null;
        }
        return item;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let pos: usize = 0;
        pos = engAppendBytes(out_buf, pos, _.property);
        let tb: [32]u8 = [0; 32];
        if (!_.duration.isZero() || !_.delay.isZero()) {
            pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.duration.format(tb[0..32], minify));
        }
        if (!_.timing_function.isEase()) {
            let eb: [64]u8 = [0; 64];
            pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.timing_function.format(eb[0..64], minify));
        }
        if (!_.delay.isZero()) {
            pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.delay.format(tb[0..32], minify));
        }
        return out_buf[0..pos];
    }
}

pub struct AnimationItem {
    name: []const u8 = "none";
    duration: Time = Time.zero();
    timing_function: EasingFunction = EasingFunction.ease();
    delay: Time = Time.zero();
    iteration_count: []const u8 = "1";
    direction: []const u8 = "normal";
    fill_mode: []const u8 = "none";
    play_state: []const u8 = "running";

    pub static fn parse(raw: []const u8): AnimationItem? {
        let parts: [10][]const u8 = [""; 10];
        const n = engSplitSpaceTop(raw, parts[0..10]);
        if (n == 0) return null;

        let item = AnimationItem {};
        let got_dur = false;
        let got_delay = false;
        let got_ease = false;
        let got_iter = false;
        let got_dir = false;
        let got_fill = false;
        let got_play = false;
        let got_name = false;

        let i: usize = 0;
        while (i < n) {
            const tok = parts[i];
            if (!got_dur) {
                if (t := Time.parse(tok)) { item.duration = t; got_dur = true; i += 1; continue; }
            }
            if (!got_ease) {
                if (e := EasingFunction.parse(tok)) { item.timing_function = e; got_ease = true; i += 1; continue; }
            }
            if (!got_delay) {
                if (t := Time.parse(tok)) { item.delay = t; got_delay = true; i += 1; continue; }
            }
            if (!got_iter && (std.ascii.eqlIgnoreCase(tok, "infinite") || (std.fmt.parseFloat(f64, tok) catch null) != null)) {
                item.iteration_count = tok;
                got_iter = true;
                i += 1;
                continue;
            }
            if (!got_dir && (std.ascii.eqlIgnoreCase(tok, "normal") || std.ascii.eqlIgnoreCase(tok, "reverse") || std.ascii.eqlIgnoreCase(tok, "alternate") || std.ascii.eqlIgnoreCase(tok, "alternate-reverse"))) {
                item.direction = tok;
                got_dir = true;
                i += 1;
                continue;
            }
            if (!got_fill && (std.ascii.eqlIgnoreCase(tok, "forwards") || std.ascii.eqlIgnoreCase(tok, "backwards") || std.ascii.eqlIgnoreCase(tok, "both"))) {
                item.fill_mode = tok;
                got_fill = true;
                i += 1;
                continue;
            }
            if (!got_play && (std.ascii.eqlIgnoreCase(tok, "running") || std.ascii.eqlIgnoreCase(tok, "paused"))) {
                item.play_state = tok;
                got_play = true;
                i += 1;
                continue;
            }
            if (!got_name) {
                item.name = tok;
                got_name = true;
                i += 1;
                continue;
            }
            return null;
        }
        return item;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (std.ascii.eqlIgnoreCase(_.name, "none")) return std.fmt.bufPrint(out_buf, "none", .{}) catch "";
        let pos: usize = 0;
        let tb: [32]u8 = [0; 32];
        if (!_.duration.isZero() || !_.delay.isZero()) {
            pos = engAppendBytes(out_buf, pos, _.duration.format(tb[0..32], minify));
            pos = engAppendBytes(out_buf, pos, " ");
        }
        if (!_.timing_function.isEase()) {
            let eb: [64]u8 = [0; 64];
            pos = engAppendBytes(out_buf, pos, _.timing_function.format(eb[0..64], minify));
            pos = engAppendBytes(out_buf, pos, " ");
        }
        if (!_.delay.isZero()) {
            pos = engAppendBytes(out_buf, pos, _.delay.format(tb[0..32], minify));
            pos = engAppendBytes(out_buf, pos, " ");
        }
        if (!std.mem.eql(u8, _.iteration_count, "1")) {
            pos = engAppendBytes(out_buf, pos, _.iteration_count);
            pos = engAppendBytes(out_buf, pos, " ");
        }
        if (!std.ascii.eqlIgnoreCase(_.direction, "normal")) {
            pos = engAppendBytes(out_buf, pos, _.direction);
            pos = engAppendBytes(out_buf, pos, " ");
        }
        if (!std.ascii.eqlIgnoreCase(_.fill_mode, "none")) {
            pos = engAppendBytes(out_buf, pos, _.fill_mode);
            pos = engAppendBytes(out_buf, pos, " ");
        }
        if (!std.ascii.eqlIgnoreCase(_.play_state, "running")) {
            pos = engAppendBytes(out_buf, pos, _.play_state);
            pos = engAppendBytes(out_buf, pos, " ");
        }
        pos = engAppendBytes(out_buf, pos, _.name);
        return out_buf[0..pos];
    }
}

pub struct DisplayValue {
    outside: []const u8 = "";
    inside: []const u8 = "";
    is_list_item: bool = false;
    keyword: []const u8 = "";

    pub static fn parse(raw: []const u8): DisplayValue? {
        let parts: [4][]const u8 = ["", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..4]);
        if (n == 0 || n > 3) return null;

        if (n == 1) {
            const k = parts[0];
            if (std.ascii.eqlIgnoreCase(k, "none") || std.ascii.eqlIgnoreCase(k, "contents") ||
                std.ascii.startsWithIgnoreCase(k, "table-"))
            {
                return DisplayValue { keyword: k };
            }
            if (std.ascii.eqlIgnoreCase(k, "inline-block")) return DisplayValue { outside: "inline", inside: "flow-root" };
            if (std.ascii.eqlIgnoreCase(k, "inline-flex")) return DisplayValue { outside: "inline", inside: "flex" };
            if (std.ascii.eqlIgnoreCase(k, "inline-grid")) return DisplayValue { outside: "inline", inside: "grid" };
            if (std.ascii.eqlIgnoreCase(k, "inline-table")) return DisplayValue { outside: "inline", inside: "table" };
            if (std.ascii.eqlIgnoreCase(k, "block") || std.ascii.eqlIgnoreCase(k, "inline") || std.ascii.eqlIgnoreCase(k, "run-in")) {
                return DisplayValue { outside: k, inside: "flow" };
            }
            if (std.ascii.eqlIgnoreCase(k, "flow") || std.ascii.eqlIgnoreCase(k, "flow-root") ||
                std.ascii.eqlIgnoreCase(k, "flex") || std.ascii.eqlIgnoreCase(k, "grid") ||
                std.ascii.eqlIgnoreCase(k, "table") || std.ascii.eqlIgnoreCase(k, "ruby"))
            {
                return DisplayValue { outside: if (std.ascii.eqlIgnoreCase(k, "ruby")) "inline" else "block", inside: k };
            }
            if (std.ascii.eqlIgnoreCase(k, "list-item")) {
                return DisplayValue { outside: "block", inside: "flow", is_list_item: true };
            }
            return null;
        }

        let dv = DisplayValue {};
        let i: usize = 0;
        while (i < n) {
            const tok = parts[i];
            if (std.ascii.eqlIgnoreCase(tok, "list-item")) {
                dv.is_list_item = true;
            } else if (std.ascii.eqlIgnoreCase(tok, "block") || std.ascii.eqlIgnoreCase(tok, "inline") || std.ascii.eqlIgnoreCase(tok, "run-in")) {
                dv.outside = tok;
            } else if (std.ascii.eqlIgnoreCase(tok, "flow") || std.ascii.eqlIgnoreCase(tok, "flow-root") ||
                std.ascii.eqlIgnoreCase(tok, "flex") || std.ascii.eqlIgnoreCase(tok, "grid") ||
                std.ascii.eqlIgnoreCase(tok, "table") || std.ascii.eqlIgnoreCase(tok, "ruby"))
            {
                dv.inside = tok;
            } else {
                return null;
            }
            i += 1;
        }
        if (dv.inside.len == 0) dv.inside = "flow";
        if (dv.outside.len == 0) dv.outside = if (std.ascii.eqlIgnoreCase(dv.inside, "ruby")) "inline" else "block";
        return dv;
    }

    pub fn format(out_buf: []u8): []const u8 {
        if (_.keyword.len > 0) return std.fmt.bufPrint(out_buf, "{s}", .{_.keyword}) catch "";
        if (!_.is_list_item) {
            if (std.ascii.eqlIgnoreCase(_.outside, "inline") && std.ascii.eqlIgnoreCase(_.inside, "flow-root")) return std.fmt.bufPrint(out_buf, "inline-block", .{}) catch "";
            if (std.ascii.eqlIgnoreCase(_.outside, "inline") && std.ascii.eqlIgnoreCase(_.inside, "flex")) return std.fmt.bufPrint(out_buf, "inline-flex", .{}) catch "";
            if (std.ascii.eqlIgnoreCase(_.outside, "inline") && std.ascii.eqlIgnoreCase(_.inside, "grid")) return std.fmt.bufPrint(out_buf, "inline-grid", .{}) catch "";
            if (std.ascii.eqlIgnoreCase(_.outside, "inline") && std.ascii.eqlIgnoreCase(_.inside, "table")) return std.fmt.bufPrint(out_buf, "inline-table", .{}) catch "";
            if (std.ascii.eqlIgnoreCase(_.inside, "flow")) return std.fmt.bufPrint(out_buf, "{s}", .{_.outside}) catch "";
            if (std.ascii.eqlIgnoreCase(_.outside, "block") && !std.ascii.eqlIgnoreCase(_.inside, "ruby")) return std.fmt.bufPrint(out_buf, "{s}", .{_.inside}) catch "";
            return std.fmt.bufPrint(out_buf, "{s} {s}", .{ _.outside, _.inside }) catch "";
        }
        if (std.ascii.eqlIgnoreCase(_.outside, "block") && std.ascii.eqlIgnoreCase(_.inside, "flow")) {
            return std.fmt.bufPrint(out_buf, "list-item", .{}) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{s} {s} list-item", .{ _.outside, _.inside }) catch "";
    }
}

pub enum FilterKind {
    Blur,
    Brightness,
    Contrast,
    DropShadow,
    Grayscale,
    HueRotate,
    Invert,
    Opacity,
    Saturate,
    Sepia,
    Url,
}

pub struct FilterOp {
    kind: FilterKind = FilterKind.Blur;
    arg: []const u8 = "";

    fn fnName(): []const u8 {
        return switch (_.kind) {
            .Blur => "blur",
            .Brightness => "brightness",
            .Contrast => "contrast",
            .DropShadow => "drop-shadow",
            .Grayscale => "grayscale",
            .HueRotate => "hue-rotate",
            .Invert => "invert",
            .Opacity => "opacity",
            .Saturate => "saturate",
            .Sepia => "sepia",
            .Url => "url",
        };
    }

    pub static fn parse(raw: []const u8): FilterOp? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        const open = std.mem.indexOfScalar(u8, s, '(') ?? return null;
        if (s[s.len - 1] != ')') return null;
        const name = std.mem.trim(u8, s[0..open], " \t");
        const arg = std.mem.trim(u8, s[open + 1 .. s.len - 1], " \t");

        if (std.ascii.eqlIgnoreCase(name, "blur")) return FilterOp { kind: FilterKind.Blur, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "brightness")) return FilterOp { kind: FilterKind.Brightness, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "contrast")) return FilterOp { kind: FilterKind.Contrast, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "drop-shadow")) return FilterOp { kind: FilterKind.DropShadow, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "grayscale")) return FilterOp { kind: FilterKind.Grayscale, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "hue-rotate")) return FilterOp { kind: FilterKind.HueRotate, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "invert")) return FilterOp { kind: FilterKind.Invert, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "opacity")) return FilterOp { kind: FilterKind.Opacity, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "saturate")) return FilterOp { kind: FilterKind.Saturate, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "sepia")) return FilterOp { kind: FilterKind.Sepia, arg: arg };
        if (std.ascii.eqlIgnoreCase(name, "url")) return FilterOp { kind: FilterKind.Url, arg: arg };
        return null;
    }

    pub fn format(out_buf: []u8): []const u8 {
        return std.fmt.bufPrint(out_buf, "{s}({s})", .{ _.fnName(), _.arg }) catch "";
    }
}

pub struct FilterList {
    ops: [12]FilterOp = std.mem.zeroes([12]FilterOp);
    count: usize = 0;
    is_none: bool = false;

    pub static fn parse(raw: []const u8): FilterList? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) return FilterList { is_none: true };
        let tokens: [12][]const u8 = [""; 12];
        const n = engSplitSpaceTop(s, tokens[0..12]);
        if (n == 0) return null;
        let list = FilterList {};
        let i: usize = 0;
        while (i < n && list.count < 12) {
            const op = FilterOp.parse(tokens[i]) ?? return null;
            list.ops[list.count] = op;
            list.count += 1;
            i += 1;
        }
        return list;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.is_none) return std.fmt.bufPrint(out_buf, "none", .{}) catch "";
        let pos: usize = 0;
        let ob: [128]u8 = [0; 128];
        let i: usize = 0;
        while (i < _.count) {
            if (i > 0 && !minify) pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.ops[i].format(ob[0..128]));
            i += 1;
        }
        return out_buf[0..pos];
    }
}
