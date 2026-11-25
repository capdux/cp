// Source: Absorbed from /data/dev/bun/src/css/properties/{overflow,position,contain}.cp | Synced: 2026-11-21T17:05:36Z
// Pure-CP zero-allocation CSS Layout Control properties: Overflow, TextOverflow,
// Position (with -webkit-sticky), ContainerType, ContainFlags, and ZIndex.

import { isValidCssMathSyntax } from "./mathFunc.cp";
import { isValidCssLenPct } from "./length.cp";

pub enum OverflowKeyword: u8 {
    visible = 0,
    hidden = 1,
    clip = 2,
    scroll = 3,
    auto = 4,

    pub static fn parse(input: []const u8): OverflowKeyword? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "visible")) return .visible;
        if (eqIgnoreCase(s, "hidden")) return .hidden;
        if (eqIgnoreCase(s, "clip")) return .clip;
        if (eqIgnoreCase(s, "scroll")) return .scroll;
        if (eqIgnoreCase(s, "auto")) return .auto;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .visible => "visible",
            .hidden => "hidden",
            .clip => "clip",
            .scroll => "scroll",
            .auto => "auto",
        };
    }
}

/// CSS `overflow` shorthand (`<x> [<y>]`).
pub struct Overflow {
    x: OverflowKeyword,
    y: OverflowKeyword,

    pub static fn parse(input: []const u8): Overflow? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        let pos: usize = 0;
        const t1 = nextTok(s, &pos) ?? return null;
        const x = OverflowKeyword.parse(t1) ?? return null;
        if (t2 := nextTok(s, &pos)) {
            if (nextTok(s, &pos) != null) return null;
            const y = OverflowKeyword.parse(t2) ?? return null;
            return Overflow{ x: x, y: y };
        }
        return Overflow{ x: x, y: x };
    }

    pub fn toCss(out: []u8): []const u8 {
        let p = appendStr(out, 0, _.x.asStr());
        if (_.y != _.x) {
            p = appendStr(out, p, " ");
            p = appendStr(out, p, _.y.asStr());
        }
        return out[0..p];
    }
}

pub enum TextOverflow: u8 {
    clip = 0,
    ellipsis = 1,

    pub static fn parse(input: []const u8): TextOverflow? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "clip")) return .clip;
        if (eqIgnoreCase(s, "ellipsis")) return .ellipsis;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .clip => "clip",
            .ellipsis => "ellipsis",
        };
    }
}

pub enum Position: u8 {
    static_pos = 0,
    relative = 1,
    absolute = 2,
    fixed = 3,
    sticky = 4,
    webkit_sticky = 5,

    pub static fn parse(input: []const u8): Position? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "static")) return .static_pos;
        if (eqIgnoreCase(s, "relative")) return .relative;
        if (eqIgnoreCase(s, "absolute")) return .absolute;
        if (eqIgnoreCase(s, "fixed")) return .fixed;
        if (eqIgnoreCase(s, "sticky")) return .sticky;
        if (eqIgnoreCase(s, "-webkit-sticky")) return .webkit_sticky;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .static_pos => "static",
            .relative => "relative",
            .absolute => "absolute",
            .fixed => "fixed",
            .sticky => "sticky",
            .webkit_sticky => "-webkit-sticky",
        };
    }

    pub fn isSticky(): bool {
        return _ == .sticky || _ == .webkit_sticky;
    }
}

pub enum ContainerType: u8 {
    normal = 0,
    size = 1,
    inline_size = 2,

    pub static fn parse(input: []const u8): ContainerType? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "normal")) return .normal;
        if (eqIgnoreCase(s, "size")) return .size;
        if (eqIgnoreCase(s, "inline-size")) return .inline_size;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .normal => "normal",
            .size => "size",
            .inline_size => "inline-size",
        };
    }
}

/// CSS `contain` property flags: `none | strict | content | [size || inline-size || layout || style || paint]`.
pub struct ContainFlags {
    size: bool,
    inline_size: bool,
    layout: bool,
    style: bool,
    paint: bool,

    pub static fn noneVal(): ContainFlags {
        return ContainFlags{ size: false, inline_size: false, layout: false, style: false, paint: false };
    }

    pub static fn strictVal(): ContainFlags {
        return ContainFlags{ size: true, inline_size: false, layout: true, style: true, paint: true };
    }

    pub static fn contentVal(): ContainFlags {
        return ContainFlags{ size: false, inline_size: false, layout: true, style: true, paint: true };
    }

    pub static fn parse(input: []const u8): ContainFlags? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "none")) return ContainFlags.noneVal();
        if (eqIgnoreCase(s, "strict")) return ContainFlags.strictVal();
        if (eqIgnoreCase(s, "content")) return ContainFlags.contentVal();

        let res = ContainFlags.noneVal();
        let pos: usize = 0;
        let count: usize = 0;
        while (tok := nextTok(s, &pos)) {
            if (eqIgnoreCase(tok, "size")) {
                if (res.size || res.inline_size) return null;
                res.size = true;
            } else if (eqIgnoreCase(tok, "inline-size")) {
                if (res.size || res.inline_size) return null;
                res.inline_size = true;
            } else if (eqIgnoreCase(tok, "layout")) {
                if (res.layout) return null;
                res.layout = true;
            } else if (eqIgnoreCase(tok, "style")) {
                if (res.style) return null;
                res.style = true;
            } else if (eqIgnoreCase(tok, "paint")) {
                if (res.paint) return null;
                res.paint = true;
            } else {
                return null;
            }
            count += 1;
        }
        if (count == 0) return null;
        return res;
    }

    pub fn toCss(out: []u8): []const u8 {
        if (!_.size && !_.inline_size && !_.layout && !_.style && !_.paint) {
            const l = appendStr(out, 0, "none");
            return out[0..l];
        }
        if (_.size && !_.inline_size && _.layout && _.style && _.paint) {
            const l = appendStr(out, 0, "strict");
            return out[0..l];
        }
        if (!_.size && !_.inline_size && _.layout && _.style && _.paint) {
            const l = appendStr(out, 0, "content");
            return out[0..l];
        }
        let p: usize = 0;
        if (_.size) p = appendSpaced(out, p, "size");
        if (_.inline_size) p = appendSpaced(out, p, "inline-size");
        if (_.layout) p = appendSpaced(out, p, "layout");
        if (_.style) p = appendSpaced(out, p, "style");
        if (_.paint) p = appendSpaced(out, p, "paint");
        return out[0..p];
    }
}

/// CSS `z-index`: `auto | <integer>`.
pub struct ZIndex {
    is_auto: bool,
    value: i32,

    pub static fn autoVal(): ZIndex {
        return ZIndex{ is_auto: true, value: 0 };
    }

    pub static fn integer(v: i32): ZIndex {
        return ZIndex{ is_auto: false, value: v };
    }

    pub static fn parse(input: []const u8): ZIndex? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "auto")) return ZIndex.autoVal();
        let idx: usize = 0;
        let neg = false;
        if (s[0] == '-') {
            neg = true;
            idx = 1;
        } else if (s[0] == '+') {
            idx = 1;
        }
        if (idx >= s.len) return null;
        let acc: i64 = 0;
        while (idx < s.len) : (idx += 1) {
            const c = s[idx];
            if (c < '0' || c > '9') return null;
            acc = acc * 10 + @as(i64, c - '0');
            if (acc > 2147483647) return null;
        }
        const signed: i32 = if (neg) @intCast(0 - acc) else @intCast(acc);
        return ZIndex.integer(signed);
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.is_auto) {
            const l = appendStr(out, 0, "auto");
            return out[0..l];
        }
        let p: usize = 0;
        let v: i64 = _.value;
        if (v < 0) {
            p = appendStr(out, p, "-");
            v = 0 - v;
        }
        p = appendU32(out, p, @intCast(v));
        return out[0..p];
    }
}

// ---------- Helpers ----------

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

fn nextTok(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && isWs(s[pos.*])) : (pos.* += 1) {}
    if (pos.* >= s.len) return null;
    const start = pos.*;
    while (pos.* < s.len && !isWs(s[pos.*])) : (pos.* += 1) {}
    return s[start..pos.*];
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

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn appendSpaced(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    if (p > 0) p = appendStr(out, p, " ");
    return appendStr(out, p, s);
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

// ---------- Property Syntax Validators ----------

fn isCssDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isCssIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

fn isCssIdentChar(c: u8): bool {
    return isCssIdentStart(c) || isCssDigit(c) || c == '-';
}

fn isCssWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\x0c';
}

fn trimCssWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isCssWs(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && isCssWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn skipBalancedParens(s: []const u8, open_pos: usize): usize {
    let i = open_pos + 1;
    let depth: usize = 1;
    while (i < s.len && depth > 0) {
        if (s[i] == '(') depth += 1;
        else if (s[i] == ')') depth -= 1;
        i += 1;
    }
    return i;
}

fn consumeBalancedCallRest(s: []const u8, start: usize): bool {
    let i = start;
    let depth: usize = 1;
    while (i < s.len && depth > 0) {
        if (s[i] == '(') depth += 1;
        else if (s[i] == ')') depth -= 1;
        i += 1;
    }
    return depth == 0 && i == s.len;
}

fn isValidVarOrEnv(s: []const u8): bool {
    if (s.len >= 5 && eqIgnoreCase(s[0..4], "var(")) {
        let i: usize = 4;
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i + 2 >= s.len || s[i] != '-' || s[i + 1] != '-') return false;
        i += 2;
        const name_start = i;
        while (i < s.len && isCssIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i >= s.len) return false;
        if (s[i] == ')') return i + 1 == s.len;
        if (s[i] != ',') return false;
        return consumeBalancedCallRest(s, i + 1);
    }
    if (s.len >= 5 && eqIgnoreCase(s[0..4], "env(")) {
        let i: usize = 4;
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i >= s.len || (!isCssIdentStart(s[i]) && s[i] != '-')) return false;
        const name_start = i;
        while (i < s.len && isCssIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len) {
            while (i < s.len && isCssWs(s[i])) i += 1;
            if (i >= s.len) return false;
            if (s[i] == ')') return i + 1 == s.len;
            if (s[i] == ',') return consumeBalancedCallRest(s, i + 1);
            if (isCssDigit(s[i])) {
                while (i < s.len && isCssDigit(s[i])) i += 1;
                continue;
            }
            return false;
        }
        return false;
    }
    return false;
}

fn checkMathPureNumberOnly(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (isCssIdentStart(c) || (c == '-' && i + 1 < s.len && (isCssIdentStart(s[i + 1]) || s[i + 1] == '-'))) {
            const start = i;
            i += 1;
            while (i < s.len && isCssIdentChar(s[i])) i += 1;
            const ident = s[start..i];
            if (i < s.len && s[i] == '(' && (eqIgnoreCase(ident, "var") || eqIgnoreCase(ident, "env"))) {
                i = skipBalancedParens(s, i);
            }
            continue;
        }
        if (isCssDigit(c) || (c == '.' && i + 1 < s.len && isCssDigit(s[i + 1]))) {
            while (i < s.len && isCssDigit(s[i])) i += 1;
            if (i < s.len && s[i] == '.') {
                i += 1;
                while (i < s.len && isCssDigit(s[i])) i += 1;
            }
            if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
                let exp_i = i + 1;
                if (exp_i < s.len && (s[exp_i] == '+' || s[exp_i] == '-')) exp_i += 1;
                if (exp_i < s.len && isCssDigit(s[exp_i])) {
                    i = exp_i;
                    while (i < s.len && isCssDigit(s[i])) i += 1;
                }
            }
            if (i < s.len && (s[i] == '%' || isCssIdentStart(s[i]))) {
                return false;
            }
            continue;
        }
        i += 1;
    }
    return true;
}

fn isNonNegativeCssNumber(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    let is_neg = false;
    if (s[i] == '+' || s[i] == '-') {
        is_neg = (s[i] == '-');
        i += 1;
    }
    if (i >= s.len) return false;

    let has_digit = false;
    let has_nonzero = false;
    while (i < s.len && isCssDigit(s[i])) {
        has_digit = true;
        if (s[i] != '0') has_nonzero = true;
        i += 1;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        if (i >= s.len || !isCssDigit(s[i])) return false;
        while (i < s.len && isCssDigit(s[i])) {
            has_digit = true;
            if (s[i] != '0') has_nonzero = true;
            i += 1;
        }
    }
    if (!has_digit) return false;

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        let exp_i = i + 1;
        if (exp_i < s.len && (s[exp_i] == '+' || s[exp_i] == '-')) exp_i += 1;
        if (exp_i >= s.len || !isCssDigit(s[exp_i])) return false;
        i = exp_i;
        while (i < s.len && isCssDigit(s[i])) i += 1;
    }

    if (i != s.len) return false;
    if (is_neg && has_nonzero) return false;
    return true;
}

/// Zero-allocation validator for CSS `z-index` property values.
pub fn isValidCssZIndexValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;
    if (eqIgnoreCase(s, "auto")) return true;
    if (isValidVarOrEnv(s)) return true;
    if (isValidCssMathSyntax(s)) {
        return checkMathPureNumberOnly(s);
    }

    let i: usize = 0;
    if (s[i] == '+' || s[i] == '-') i += 1;
    if (i >= s.len) return false;
    while (i < s.len) : (i += 1) {
        if (!isCssDigit(s[i])) return false;
    }
    return true;
}

/// Zero-allocation validator for CSS `flex-basis` property values.
pub fn isValidCssFlexBasisValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;

    if (eqIgnoreCase(s, "content") ||
        eqIgnoreCase(s, "auto") ||
        eqIgnoreCase(s, "min-content") ||
        eqIgnoreCase(s, "max-content") ||
        eqIgnoreCase(s, "fit-content") ||
        eqIgnoreCase(s, "stretch"))
    {
        return true;
    }

    if (s.len >= 7 && eqIgnoreCase(s[0..7], "anchor(")) return false;
    if (s.len >= 12 && eqIgnoreCase(s[0..12], "anchor-size(")) return false;

    if (s.len >= 13 && eqIgnoreCase(s[0..12], "fit-content(") && s[s.len - 1] == ')') {
        if (skipBalancedParens(s, 11) != s.len) return false;
        const inner = trimCssWs(s[12 .. s.len - 1]);
        if (inner.len == 0) return false;
        return isValidCssLenPct(inner, false);
    }

    return isValidCssLenPct(s, false);
}

/// Zero-allocation validator for CSS `gap` / `row-gap` / `column-gap` property values.
pub fn isValidCssGapValue(val: []const u8, max_tokens: usize): bool {
    const s = trimCssWs(val);
    if (s.len == 0 || max_tokens == 0) return false;

    let i: usize = 0;
    let tok_count: usize = 0;
    while (i < s.len) {
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i >= s.len) break;

        const tok_start = i;
        let p_depth: usize = 0;
        let b_depth: usize = 0;
        let c_depth: usize = 0;
        while (i < s.len) {
            const ch = s[i];
            if (p_depth == 0 && b_depth == 0 && c_depth == 0 && isCssWs(ch)) {
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
        if (!eqIgnoreCase(tok, "normal") && !isValidCssLenPct(tok, false)) {
            return false;
        }
    }

    return tok_count >= 1 && tok_count <= max_tokens;
}

/// Zero-allocation validator for CSS `animation-iteration-count` property values.
pub fn isValidCssAnimIterCountValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;

    let i: usize = 0;
    let layer_start: usize = 0;
    let p_depth: usize = 0;
    let b_depth: usize = 0;
    let c_depth: usize = 0;
    while (i <= s.len) {
        const at_end = (i == s.len);
        const at_comma = (!at_end && s[i] == ',' && p_depth == 0 && b_depth == 0 && c_depth == 0);
        if (at_end || at_comma) {
            if (at_end && (p_depth != 0 || b_depth != 0 || c_depth != 0)) return false;
            const layer = trimCssWs(s[layer_start..i]);
            if (layer.len == 0) return false;
            if (!eqIgnoreCase(layer, "infinite") &&
                !isValidVarOrEnv(layer) &&
                !(isValidCssMathSyntax(layer) && checkMathPureNumberOnly(layer)) &&
                !isNonNegativeCssNumber(layer))
            {
                return false;
            }
            layer_start = i + 1;
        } else {
            const ch = s[i];
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
        }
        i += 1;
    }
    return true;
}

/// Zero-allocation validator for CSS `writing-mode` property values.
pub fn isValidCssWritingModeValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return eqIgnoreCase(s, "horizontal-tb") ||
        eqIgnoreCase(s, "vertical-rl") ||
        eqIgnoreCase(s, "vertical-lr") ||
        eqIgnoreCase(s, "sideways-rl") ||
        eqIgnoreCase(s, "sideways-lr") ||
        eqIgnoreCase(s, "lr") ||
        eqIgnoreCase(s, "lr-tb") ||
        eqIgnoreCase(s, "rl") ||
        eqIgnoreCase(s, "rl-tb") ||
        eqIgnoreCase(s, "tb") ||
        eqIgnoreCase(s, "tb-rl");
}

/// Zero-allocation validator for CSS `direction` property values.
pub fn isValidCssDirectionValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return eqIgnoreCase(s, "ltr") || eqIgnoreCase(s, "rtl");
}

/// Zero-allocation validator for CSS `unicode-bidi` property values.
pub fn isValidCssUnicodeBidiValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return eqIgnoreCase(s, "normal") ||
        eqIgnoreCase(s, "embed") ||
        eqIgnoreCase(s, "isolate") ||
        eqIgnoreCase(s, "bidi-override") ||
        eqIgnoreCase(s, "isolate-override") ||
        eqIgnoreCase(s, "plaintext");
}

/// Zero-allocation validator for CSS `float` property values.
pub fn isValidCssFloatValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return eqIgnoreCase(s, "none") ||
        eqIgnoreCase(s, "left") ||
        eqIgnoreCase(s, "right") ||
        eqIgnoreCase(s, "inline-start") ||
        eqIgnoreCase(s, "inline-end");
}

/// Zero-allocation validator for CSS `clear` property values.
pub fn isValidCssClearValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return eqIgnoreCase(s, "none") ||
        eqIgnoreCase(s, "left") ||
        eqIgnoreCase(s, "right") ||
        eqIgnoreCase(s, "both") ||
        eqIgnoreCase(s, "inline-start") ||
        eqIgnoreCase(s, "inline-end");
}

/// Zero-allocation validator for CSS `object-fit` property values.
pub fn isValidCssObjectFitValue(val: []const u8): bool {
    const s = trimCssWs(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    if (eqIgnoreCase(s, "fill") ||
        eqIgnoreCase(s, "contain") ||
        eqIgnoreCase(s, "cover") ||
        eqIgnoreCase(s, "none") ||
        eqIgnoreCase(s, "scale-down")) {
        return true;
    }
    let it = std.mem.tokenizeAny(u8, s, " \t\r\n\x0c");
    const a = it.next() ?? return false;
    const b = it.next() ?? return false;
    if (it.next() != null) return false;
    const a_cc = eqIgnoreCase(a, "contain") || eqIgnoreCase(a, "cover");
    const b_cc = eqIgnoreCase(b, "contain") || eqIgnoreCase(b, "cover");
    return (a_cc && eqIgnoreCase(b, "scale-down")) || (eqIgnoreCase(a, "scale-down") && b_cc);
}
