// Source: Absorbed from /data/dev/bun/src/css/properties/list.cp | Synced: 2026-11-22T04:57:13Z
// Pure-CP zero-allocation CSS Lists and Counters Module Level 3 properties:
// PredefinedCounterStyle, ListStyleType, ListStylePosition, MarkerSide, ListStyle shorthand.

pub enum PredefinedCounterStyle: u8 {
    disc = 0,
    circle = 1,
    square = 2,
    disclosure_open = 3,
    disclosure_closed = 4,
    decimal = 5,
    decimal_leading_zero = 6,
    lower_roman = 7,
    upper_roman = 8,
    lower_greek = 9,
    lower_latin = 10,
    upper_latin = 11,
    lower_alpha = 12,
    upper_alpha = 13,
    armenian = 14,
    georgian = 15,
    hebrew = 16,
    cjk_decimal = 17,

    pub static fn parse(input: []const u8): PredefinedCounterStyle? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "disc")) return .disc;
        if (eqIgnoreCase(s, "circle")) return .circle;
        if (eqIgnoreCase(s, "square")) return .square;
        if (eqIgnoreCase(s, "disclosure-open")) return .disclosure_open;
        if (eqIgnoreCase(s, "disclosure-closed")) return .disclosure_closed;
        if (eqIgnoreCase(s, "decimal")) return .decimal;
        if (eqIgnoreCase(s, "decimal-leading-zero")) return .decimal_leading_zero;
        if (eqIgnoreCase(s, "lower-roman")) return .lower_roman;
        if (eqIgnoreCase(s, "upper-roman")) return .upper_roman;
        if (eqIgnoreCase(s, "lower-greek")) return .lower_greek;
        if (eqIgnoreCase(s, "lower-latin")) return .lower_latin;
        if (eqIgnoreCase(s, "upper-latin")) return .upper_latin;
        if (eqIgnoreCase(s, "lower-alpha")) return .lower_alpha;
        if (eqIgnoreCase(s, "upper-alpha")) return .upper_alpha;
        if (eqIgnoreCase(s, "armenian")) return .armenian;
        if (eqIgnoreCase(s, "georgian")) return .georgian;
        if (eqIgnoreCase(s, "hebrew")) return .hebrew;
        if (eqIgnoreCase(s, "cjk-decimal")) return .cjk_decimal;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .disc => "disc",
            .circle => "circle",
            .square => "square",
            .disclosure_open => "disclosure-open",
            .disclosure_closed => "disclosure-closed",
            .decimal => "decimal",
            .decimal_leading_zero => "decimal-leading-zero",
            .lower_roman => "lower-roman",
            .upper_roman => "upper-roman",
            .lower_greek => "lower-greek",
            .lower_latin => "lower-latin",
            .upper_latin => "upper-latin",
            .lower_alpha => "lower-alpha",
            .upper_alpha => "upper-alpha",
            .armenian => "armenian",
            .georgian => "georgian",
            .hebrew => "hebrew",
            .cjk_decimal => "cjk-decimal",
        };
    }
}

pub enum ListStylePosition: u8 {
    outside = 0,
    inside = 1,

    pub static fn default(): ListStylePosition {
        return .outside;
    }

    pub static fn parse(input: []const u8): ListStylePosition? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "outside")) return .outside;
        if (eqIgnoreCase(s, "inside")) return .inside;
        return null;
    }

    pub fn asStr(): []const u8 {
        return if (_ == .outside) "outside" else "inside";
    }
}

pub enum MarkerSide: u8 {
    match_self = 0,
    match_parent = 1,

    pub static fn parse(input: []const u8): MarkerSide? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "match-self")) return .match_self;
        if (eqIgnoreCase(s, "match-parent")) return .match_parent;
        return null;
    }

    pub fn asStr(): []const u8 {
        return if (_ == .match_self) "match-self" else "match-parent";
    }
}

pub enum ListStyleTypeKind: u8 {
    none = 0,
    predefined = 1,
    string_marker = 2,
    custom_counter = 3,
}

pub struct ListStyleType {
    kind: ListStyleTypeKind,
    predefined: PredefinedCounterStyle,
    text: []const u8,

    pub static fn default(): ListStyleType {
        return ListStyleType{ kind: .predefined, predefined: .disc, text: "" };
    }

    pub static fn noneVal(): ListStyleType {
        return ListStyleType{ kind: .none, predefined: .disc, text: "" };
    }

    pub fn isDefault(): bool {
        return _.kind == .predefined && _.predefined == .disc;
    }

    pub fn eql(other: ListStyleType): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == .none) return true;
        if (_.kind == .predefined) return _.predefined == other.predefined;
        return strEq(_.text, other.text);
    }

    pub static fn parse(input: []const u8): ListStyleType? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "none")) return ListStyleType.noneVal();
        if (pre := PredefinedCounterStyle.parse(s)) {
            return ListStyleType{ kind: .predefined, predefined: pre, text: "" };
        }
        if (s.len >= 2 && ((s[0] == '"' && s[s.len - 1] == '"') || (s[0] == '\'' && s[s.len - 1] == '\''))) {
            return ListStyleType{ kind: .string_marker, predefined: .disc, text: s[1 .. s.len - 1] };
        }
        if (isValidCustomIdent(s) && !eqIgnoreCase(s, "inside") && !eqIgnoreCase(s, "outside")) {
            return ListStyleType{ kind: .custom_counter, predefined: .disc, text: s };
        }
        return null;
    }

    pub fn toCss(out: []u8): []const u8 {
        const l = writeListStyleType(out, 0, _);
        return out[0..l];
    }
}

/// CSS `list-style` shorthand: `<list-style-position> || <list-style-image> || <list-style-type>`.
pub struct ListStyle {
    position: ListStylePosition,
    image: []const u8, // empty or "none" means no image; otherwise e.g. `url(dot.png)`
    list_type: ListStyleType,

    pub static fn default(): ListStyle {
        return ListStyle{
            position: .outside,
            image: "",
            list_type: ListStyleType.default(),
        };
    }

    pub static fn parse(input: []const u8): ListStyle? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let pos_opt: ListStylePosition? = null;
        let img_set = false;
        let img_val: []const u8 = "";
        let type_opt: ListStyleType? = null;
        let none_count: u8 = 0;

        let idx: usize = 0;
        while (idx < s.len) {
            while (idx < s.len && isWs(s[idx])) : (idx += 1) {}
            if (idx >= s.len) break;

            const start = idx;
            let depth: u32 = 0;
            let in_quote: u8 = 0;
            while (idx < s.len) : (idx += 1) {
                const c = s[idx];
                if (in_quote != 0) {
                    if (c == '\\' && idx + 1 < s.len) {
                        idx += 1;
                    } else if (c == in_quote) {
                        in_quote = 0;
                    }
                } else {
                    if (c == '"' || c == '\'') {
                        in_quote = c;
                    } else if (c == '(') {
                        depth += 1;
                    } else if (c == ')') {
                        if (depth == 0) return null;
                        depth -= 1;
                    } else if (isWs(c) && depth == 0) {
                        break;
                    }
                }
            }
            if (depth != 0 || in_quote != 0) return null;
            const tok = s[start..idx];

            if (eqIgnoreCase(tok, "none")) {
                none_count += 1;
                if (none_count > 2) return null;
                continue;
            }
            if (pos_opt == null) {
                if (p := ListStylePosition.parse(tok)) {
                    pos_opt = p;
                    continue;
                }
            }
            if (!img_set && isImageFunc(tok)) {
                img_set = true;
                img_val = tok;
                continue;
            }
            if (type_opt == null) {
                if (t := ListStyleType.parse(tok)) {
                    type_opt = t;
                    continue;
                }
            }
            return null;
        }

        // Resolve ambiguous `none` tokens per CSS Lists 3 spec
        if (none_count == 2) {
            if (img_set || type_opt != null) return null;
            type_opt = ListStyleType.noneVal();
            img_set = true;
            img_val = "";
        } else if (none_count == 1) {
            if (type_opt == null) {
                type_opt = ListStyleType.noneVal();
            } else if (!img_set) {
                img_set = true;
                img_val = "";
            } else {
                return null;
            }
        }

        return ListStyle{
            position: pos_opt ?? .outside,
            image: img_val,
            list_type: type_opt ?? ListStyleType.default(),
        };
    }

    pub fn toCss(out: []u8): []const u8 {
        let p: usize = 0;
        if (_.position != .outside) {
            p = appendStr(out, p, _.position.asStr());
        }
        if (_.image.len > 0) {
            if (p > 0) p = appendStr(out, p, " ");
            p = appendStr(out, p, _.image);
        }
        if (!_.list_type.isDefault()) {
            if (p > 0) p = appendStr(out, p, " ");
            p = writeListStyleType(out, p, _.list_type);
        }
        if (p == 0) {
            p = appendStr(out, p, "disc");
        }
        return out[0..p];
    }
}

// ---------- Helpers ----------

fn writeListStyleType(out: []u8, pos: usize, t: ListStyleType): usize {
    switch (t.kind) {
        .none => return appendStr(out, pos, "none"),
        .predefined => return appendStr(out, pos, t.predefined.asStr()),
        .custom_counter => return appendStr(out, pos, t.text),
        .string_marker => {
            let p = appendStr(out, pos, "\"");
            p = appendStr(out, p, t.text);
            return appendStr(out, p, "\"");
        },
    }
}

fn isImageFunc(tok: []const u8): bool {
    if (tok.len < 5 || tok[tok.len - 1] != ')') return false;
    if (startsWithIgnoreCase(tok, "url(")) return true;
    if (startsWithIgnoreCase(tok, "linear-gradient(")) return true;
    if (startsWithIgnoreCase(tok, "radial-gradient(")) return true;
    return false;
}

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

fn isValidCustomIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) return false;
    }
    return true;
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
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

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}
