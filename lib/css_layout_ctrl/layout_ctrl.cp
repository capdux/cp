// Source: Absorbed from /data/dev/bun/src/css/properties/{overflow,position,contain}.cp
// Pure-CP zero-allocation CSS Layout Control properties: Overflow, TextOverflow,
// Position (with -webkit-sticky), ContainerType, ContainFlags, and ZIndex.

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
