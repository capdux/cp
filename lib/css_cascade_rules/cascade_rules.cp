// Source: Absorbed from /data/dev/bun/src/css/rules/{import,scope,viewport}.cp
// Pure-CP zero-allocation CSS Cascade / Scoping rule prelude parsers & serializers:
// `@import` (with layer/supports/media), `@scope (<start>) [to (<end>)]`, and `@layer`.

pub enum ImportLayerKind: u8 {
    none = 0,
    anonymous = 1,
    named = 2,
}

/// Parsed `@import` rule header:
/// `@import <string | url(...)> [layer | layer(<name>)]? [supports(<cond>)]? [<media>]?`
pub struct ImportRule {
    url: []const u8,
    layer_kind: ImportLayerKind,
    layer_name: []const u8,
    supports: []const u8,
    media: []const u8,

    pub fn hasConditions(): bool {
        return _.layer_kind != .none || _.supports.len > 0 || _.media.len > 0;
    }

    pub static fn parse(input: []const u8): ImportRule? {
        let s = trimAscii(input);
        if (s.len == 0) return null;
        // Strip optional trailing ';'
        if (s[s.len - 1] == ';') {
            s = trimAscii(s[0 .. s.len - 1]);
        }
        // Strip optional leading '@import'
        if (startsWithIgnoreCase(s, "@import")) {
            if (s.len == 7 || (!isWs(s[7]) && s[7] != '"' && s[7] != '\'')) return null;
            s = trimAscii(s[7..]);
        }
        if (s.len == 0) return null;

        let pos: usize = 0;
        let url: []const u8 = "";

        if (s[0] == '"' || s[0] == '\'') {
            const q = s[0];
            pos = 1;
            const start = pos;
            while (pos < s.len && s[pos] != q) : (pos += 1) {
                if (s[pos] == '\\' && pos + 1 < s.len) pos += 1;
            }
            if (pos >= s.len) return null;
            url = s[start..pos];
            pos += 1;
        } else if (startsWithFunc(s, 0, "url")) {
            const open_p: usize = 3;
            const close_p = findMatchingParen(s, open_p) ?? return null;
            url = stripQuotes(trimAscii(s[open_p + 1 .. close_p]));
            pos = close_p + 1;
        } else {
            return null;
        }

        if (url.len == 0) return null;
        skipWs(s, &pos);

        let layer_kind: ImportLayerKind = .none;
        let layer_name: []const u8 = "";
        let supports: []const u8 = "";

        // Optional `layer` or `layer(<name>)`
        if (startsWithFunc(s, pos, "layer")) {
            const open_p = pos + 5;
            const close_p = findMatchingParen(s, open_p) ?? return null;
            const lname = trimAscii(s[open_p + 1 .. close_p]);
            if (lname.len == 0) return null;
            layer_kind = .named;
            layer_name = lname;
            pos = close_p + 1;
            skipWs(s, &pos);
        } else if (consumeKeyword(s, &pos, "layer")) {
            layer_kind = .anonymous;
            skipWs(s, &pos);
        }

        // Optional `supports(...)`
        if (startsWithFunc(s, pos, "supports")) {
            const open_p = pos + 8;
            const close_p = findMatchingParen(s, open_p) ?? return null;
            const sup = trimAscii(s[open_p + 1 .. close_p]);
            if (sup.len == 0) return null;
            supports = sup;
            pos = close_p + 1;
            skipWs(s, &pos);
        }

        const media = trimAscii(s[pos..]);
        return ImportRule{
            url: url,
            layer_kind: layer_kind,
            layer_name: layer_name,
            supports: supports,
            media: media,
        };
    }

    pub fn toCss(out: []u8): []const u8 {
        let p = appendStr(out, 0, "@import \"");
        p = appendStr(out, p, _.url);
        p = appendStr(out, p, "\"");
        if (_.layer_kind == .anonymous) {
            p = appendStr(out, p, " layer");
        } else if (_.layer_kind == .named) {
            p = appendStr(out, p, " layer(");
            p = appendStr(out, p, _.layer_name);
            p = appendStr(out, p, ")");
        }
        if (_.supports.len > 0) {
            p = appendStr(out, p, " supports(");
            p = appendStr(out, p, _.supports);
            p = appendStr(out, p, ")");
        }
        if (_.media.len > 0) {
            p = appendStr(out, p, " ");
            p = appendStr(out, p, _.media);
        }
        p = appendStr(out, p, ";");
        return out[0..p];
    }
}

/// Parsed `@scope` rule prelude: `@scope [(<scope-start>)]? [to (<scope-end>)]?`.
pub struct ScopePrelude {
    scope_start: []const u8,
    scope_end: []const u8,

    pub fn isImplicit(): bool {
        return _.scope_start.len == 0;
    }

    pub fn hasLimit(): bool {
        return _.scope_end.len > 0;
    }

    pub static fn parse(input: []const u8): ScopePrelude? {
        let s = trimAscii(input);
        if (startsWithIgnoreCase(s, "@scope")) {
            if (s.len > 6 && !isWs(s[6]) && s[6] != '(') return null;
            s = trimAscii(s[6..]);
        }

        let pos: usize = 0;
        let scope_start: []const u8 = "";
        let scope_end: []const u8 = "";

        if (pos < s.len && s[pos] == '(') {
            const close_p = findMatchingParen(s, pos) ?? return null;
            scope_start = trimAscii(s[pos + 1 .. close_p]);
            if (scope_start.len == 0) return null;
            pos = close_p + 1;
            skipWs(s, &pos);
        }

        if (pos < s.len) {
            if (!consumeKeyword(s, &pos, "to")) return null;
            skipWs(s, &pos);
            if (pos >= s.len || s[pos] != '(') return null;
            const close_p = findMatchingParen(s, pos) ?? return null;
            scope_end = trimAscii(s[pos + 1 .. close_p]);
            if (scope_end.len == 0) return null;
            pos = close_p + 1;
            skipWs(s, &pos);
        }

        if (pos != s.len) return null;
        return ScopePrelude{
            scope_start: scope_start,
            scope_end: scope_end,
        };
    }

    pub fn toCss(out: []u8): []const u8 {
        let p = appendStr(out, 0, "@scope");
        if (_.scope_start.len > 0) {
            p = appendStr(out, p, " (");
            p = appendStr(out, p, _.scope_start);
            p = appendStr(out, p, ")");
        }
        if (_.scope_end.len > 0) {
            p = appendStr(out, p, " to (");
            p = appendStr(out, p, _.scope_end);
            p = appendStr(out, p, ")");
        }
        return out[0..p];
    }
}

/// Parsed `@layer a, b.c, d;` statement (up to 16 layer names).
pub struct LayerStatement {
    names: [16][]const u8,
    len: usize,

    pub static fn parse(input: []const u8): LayerStatement? {
        let s = trimAscii(input);
        if (s.len == 0) return null;
        if (s[s.len - 1] == ';') s = trimAscii(s[0 .. s.len - 1]);
        if (startsWithIgnoreCase(s, "@layer")) {
            if (s.len == 6 || !isWs(s[6])) return null;
            s = trimAscii(s[6..]);
        }
        if (s.len == 0) return null;

        let res = LayerStatement{ names: [""; 16], len: 0 };
        let pos: usize = 0;
        while (pos <= s.len) {
            const start = pos;
            while (pos < s.len && s[pos] != ',') : (pos += 1) {}
            const item = trimAscii(s[start..pos]);
            if (item.len == 0 || !isValidLayerName(item)) return null;
            if (res.len >= 16) return null;
            res.names[res.len] = item;
            res.len += 1;
            if (pos >= s.len) break;
            pos += 1;
        }
        if (res.len == 0) return null;
        return res;
    }

    pub fn toCss(out: []u8): []const u8 {
        let p = appendStr(out, 0, "@layer ");
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (i > 0) p = appendStr(out, p, ", ");
            p = appendStr(out, p, _.names[i]);
        }
        p = appendStr(out, p, ";");
        return out[0..p];
    }
}

// ---------- Helpers ----------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn skipWs(s: []const u8, pos: *usize) {
    while (pos.* < s.len && isWs(s[pos.*])) : (pos.* += 1) {}
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && isWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn stripQuotes(s: []const u8): []const u8 {
    if (s.len >= 2 && ((s[0] == '"' && s[s.len - 1] == '"') || (s[0] == '\'' && s[s.len - 1] == '\''))) {
        return s[1 .. s.len - 1];
    }
    return s;
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

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

fn isValidLayerName(s: []const u8): bool {
    if (s.len == 0 || s[0] == '.' || s[s.len - 1] == '.') return false;
    let prev_dot = false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '.') {
            if (prev_dot) return false;
            prev_dot = true;
        } else if (isIdentChar(s[i])) {
            prev_dot = false;
        } else {
            return false;
        }
    }
    return true;
}

fn startsWithFunc(s: []const u8, pos: usize, name: []const u8): bool {
    if (pos + name.len + 1 > s.len) return false;
    return eqIgnoreCase(s[pos .. pos + name.len], name) && s[pos + name.len] == '(';
}

fn consumeKeyword(s: []const u8, pos: *usize, kw: []const u8): bool {
    if (pos.* + kw.len > s.len) return false;
    if (!eqIgnoreCase(s[pos.* .. pos.* + kw.len], kw)) return false;
    if (pos.* + kw.len < s.len && isIdentChar(s[pos.* + kw.len])) return false;
    pos.* += kw.len;
    return true;
}

fn findMatchingParen(s: []const u8, open_p: usize): usize? {
    if (open_p >= s.len || s[open_p] != '(') return null;
    let depth: u32 = 0;
    let i = open_p;
    while (i < s.len) : (i += 1) {
        if (s[i] == '(') {
            depth += 1;
        } else if (s[i] == ')') {
            depth -= 1;
            if (depth == 0) return i;
        }
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
