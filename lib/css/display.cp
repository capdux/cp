// Source: Absorbed from /data/dev/bun/src/css/properties/display.cp | Synced: 2026-11-22T08:16:01Z
//! CSS Display Level 3 (`display` and `visibility` properties):
//! supports single keywords, multi-keyword outside/inside/list-item combinations,
//! legacy shorthands (`inline-block`, `inline-flex`, `inline-grid`, vendor prefixes),
//! and canonical CSS serialization.

export enum Visibility: u8 {
    visible = 0,
    hidden = 1,
    collapse = 2,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .visible => "visible",
            .hidden => "hidden",
            .collapse => "collapse",
        };
    }

    pub static fn parse(input: []const u8): Visibility? {
        if (asciiCaseEql(input, "visible")) return .visible;
        if (asciiCaseEql(input, "hidden")) return .hidden;
        if (asciiCaseEql(input, "collapse")) return .collapse;
        return null;
    }
}

export enum DisplayKeyword: u8 {
    none = 0,
    contents = 1,
    table_row_group = 2,
    table_header_group = 3,
    table_footer_group = 4,
    table_row = 5,
    table_cell = 6,
    table_column_group = 7,
    table_column = 8,
    table_caption = 9,
    ruby_base = 10,
    ruby_text = 11,
    ruby_base_container = 12,
    ruby_text_container = 13,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .none => "none",
            .contents => "contents",
            .table_row_group => "table-row-group",
            .table_header_group => "table-header-group",
            .table_footer_group => "table-footer-group",
            .table_row => "table-row",
            .table_cell => "table-cell",
            .table_column_group => "table-column-group",
            .table_column => "table-column",
            .table_caption => "table-caption",
            .ruby_base => "ruby-base",
            .ruby_text => "ruby-text",
            .ruby_base_container => "ruby-base-container",
            .ruby_text_container => "ruby-text-container",
        };
    }

    pub static fn parse(input: []const u8): DisplayKeyword? {
        if (asciiCaseEql(input, "none")) return .none;
        if (asciiCaseEql(input, "contents")) return .contents;
        if (asciiCaseEql(input, "table-row-group")) return .table_row_group;
        if (asciiCaseEql(input, "table-header-group")) return .table_header_group;
        if (asciiCaseEql(input, "table-footer-group")) return .table_footer_group;
        if (asciiCaseEql(input, "table-row")) return .table_row;
        if (asciiCaseEql(input, "table-cell")) return .table_cell;
        if (asciiCaseEql(input, "table-column-group")) return .table_column_group;
        if (asciiCaseEql(input, "table-column")) return .table_column;
        if (asciiCaseEql(input, "table-caption")) return .table_caption;
        if (asciiCaseEql(input, "ruby-base")) return .ruby_base;
        if (asciiCaseEql(input, "ruby-text")) return .ruby_text;
        if (asciiCaseEql(input, "ruby-base-container")) return .ruby_base_container;
        if (asciiCaseEql(input, "ruby-text-container")) return .ruby_text_container;
        return null;
    }

    pub fn isTableInternal(): bool {
        const v = @intFromEnum(_);
        return v >= 2 && v <= 9;
    }

    pub fn isRubyInternal(): bool {
        const v = @intFromEnum(_);
        return v >= 10 && v <= 13;
    }
}

export enum DisplayOutside: u8 {
    block = 0,
    inline_box = 1,
    run_in = 2,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .block => "block",
            .inline_box => "inline",
            .run_in => "run-in",
        };
    }

    pub static fn parse(input: []const u8): DisplayOutside? {
        if (asciiCaseEql(input, "block")) return .block;
        if (asciiCaseEql(input, "inline")) return .inline_box;
        if (asciiCaseEql(input, "run-in")) return .run_in;
        return null;
    }
}

export enum DisplayInside: u8 {
    flow = 0,
    flow_root = 1,
    table = 2,
    flex = 3,
    webkit_flex = 4,
    ms_flexbox = 5,
    webkit_box = 6,
    moz_box = 7,
    grid = 8,
    ruby = 9,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .flow => "flow",
            .flow_root => "flow-root",
            .table => "table",
            .flex => "flex",
            .webkit_flex => "-webkit-flex",
            .ms_flexbox => "-ms-flexbox",
            .webkit_box => "-webkit-box",
            .moz_box => "-moz-box",
            .grid => "grid",
            .ruby => "ruby",
        };
    }

    pub static fn parse(input: []const u8): DisplayInside? {
        if (asciiCaseEql(input, "flow")) return .flow;
        if (asciiCaseEql(input, "flow-root")) return .flow_root;
        if (asciiCaseEql(input, "table")) return .table;
        if (asciiCaseEql(input, "flex")) return .flex;
        if (asciiCaseEql(input, "-webkit-flex")) return .webkit_flex;
        if (asciiCaseEql(input, "-ms-flexbox")) return .ms_flexbox;
        if (asciiCaseEql(input, "-webkit-box")) return .webkit_box;
        if (asciiCaseEql(input, "-moz-box")) return .moz_box;
        if (asciiCaseEql(input, "grid")) return .grid;
        if (asciiCaseEql(input, "ruby")) return .ruby;
        return null;
    }

    pub fn isFlex(): bool {
        return _ == .flex || _ == .webkit_flex || _ == .ms_flexbox;
    }
}

export struct DisplayPair {
    outside: DisplayOutside = .block,
    inside: DisplayInside = .flow,
    is_list_item: bool = false,

    pub fn eql(other: DisplayPair): bool {
        return _.outside == other.outside && _.inside == other.inside && _.is_list_item == other.is_list_item;
    }

    /// Format as canonical shortest CSS representation into `out`.
    pub fn toCssBuf(out: []u8): []const u8 {
        if (_.outside == .inline_box && !_.is_list_item) {
            if (_.inside == .flow_root) return copyStr(out, "inline-block");
            if (_.inside == .table) return copyStr(out, "inline-table");
            if (_.inside == .flex) return copyStr(out, "inline-flex");
            if (_.inside == .webkit_flex) return copyStr(out, "-webkit-inline-flex");
            if (_.inside == .ms_flexbox) return copyStr(out, "-ms-inline-flexbox");
            if (_.inside == .webkit_box) return copyStr(out, "-webkit-inline-box");
            if (_.inside == .moz_box) return copyStr(out, "-moz-inline-box");
            if (_.inside == .grid) return copyStr(out, "inline-grid");
        }

        const default_outside: DisplayOutside = if (_.inside == .ruby) .inline_box else .block;
        let len: usize = 0;

        if (_.outside != default_outside || (_.inside == .flow && !_.is_list_item)) {
            len = appendStr(out, len, _.outside.toCss());
        }
        if (_.inside != .flow) {
            if (len > 0) len = appendStr(out, len, " ");
            len = appendStr(out, len, _.inside.toCss());
        }
        if (_.is_list_item) {
            if (len > 0) len = appendStr(out, len, " ");
            len = appendStr(out, len, "list-item");
        }
        return out[0..len];
    }

    pub static fn parse(input: []const u8): DisplayPair? {
        const trimmed = trimAscii(input);
        if (trimmed.len == 0) return null;

        // Legacy single-token shorthands
        if (asciiCaseEql(trimmed, "inline-block")) return DisplayPair{ outside: .inline_box, inside: .flow_root, is_list_item: false };
        if (asciiCaseEql(trimmed, "inline-table")) return DisplayPair{ outside: .inline_box, inside: .table, is_list_item: false };
        if (asciiCaseEql(trimmed, "inline-flex")) return DisplayPair{ outside: .inline_box, inside: .flex, is_list_item: false };
        if (asciiCaseEql(trimmed, "-webkit-inline-flex")) return DisplayPair{ outside: .inline_box, inside: .webkit_flex, is_list_item: false };
        if (asciiCaseEql(trimmed, "-ms-inline-flexbox")) return DisplayPair{ outside: .inline_box, inside: .ms_flexbox, is_list_item: false };
        if (asciiCaseEql(trimmed, "-webkit-inline-box")) return DisplayPair{ outside: .inline_box, inside: .webkit_box, is_list_item: false };
        if (asciiCaseEql(trimmed, "-moz-inline-box")) return DisplayPair{ outside: .inline_box, inside: .moz_box, is_list_item: false };
        if (asciiCaseEql(trimmed, "inline-grid")) return DisplayPair{ outside: .inline_box, inside: .grid, is_list_item: false };

        let outside: DisplayOutside? = null;
        let inside: DisplayInside? = null;
        let list_item = false;
        let pos: usize = 0;
        let token_count: usize = 0;

        while (pos < trimmed.len) {
            while (pos < trimmed.len && isWs(trimmed[pos])) : (pos += 1) {}
            if (pos >= trimmed.len) break;
            const start = pos;
            while (pos < trimmed.len && !isWs(trimmed[pos])) : (pos += 1) {}
            const tok = trimmed[start..pos];
            token_count += 1;

            if (asciiCaseEql(tok, "list-item")) {
                if (list_item) return null;
                list_item = true;
            } else if (outside == null && DisplayOutside.parse(tok) != null) {
                outside = DisplayOutside.parse(tok);
            } else if (inside == null && DisplayInside.parse(tok) != null) {
                inside = DisplayInside.parse(tok);
            } else {
                return null;
            }
        }

        if (token_count == 0) return null;
        const final_inside: DisplayInside = inside ?? .flow;
        const final_outside: DisplayOutside = outside ?? (if (final_inside == .ruby) DisplayOutside.inline_box else DisplayOutside.block);

        if (list_item && !(final_inside == .flow || final_inside == .flow_root)) {
            return null;
        }

        return DisplayPair{
            outside: final_outside,
            inside: final_inside,
            is_list_item: list_item,
        };
    }
}

export enum DisplayKind: u8 {
    keyword = 0,
    pair = 1,
}

export struct Display {
    kind: DisplayKind,
    keyword: DisplayKeyword = .none,
    pair: DisplayPair = DisplayPair{},

    pub static fn parse(input: []const u8): Display? {
        const trimmed = trimAscii(input);
        if (kw := DisplayKeyword.parse(trimmed)) {
            return Display{ kind: .keyword, keyword: kw };
        }
        if (p := DisplayPair.parse(trimmed)) {
            return Display{ kind: .pair, pair: p };
        }
        return null;
    }

    pub fn isNone(): bool {
        return _.kind == .keyword && _.keyword == .none;
    }

    pub fn toCssBuf(out: []u8): []const u8 {
        if (_.kind == .keyword) return copyStr(out, _.keyword.toCss());
        return _.pair.toCssBuf(out);
    }
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\x0c';
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

fn copyStr(out: []u8, src: []const u8): []const u8 {
    const n = if (src.len < out.len) src.len else out.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[i] = src[i];
    }
    return out[0..n];
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

fn isCssDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isCssIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

fn isCssIdentChar(c: u8): bool {
    return isCssIdentStart(c) || isCssDigit(c) || c == '-';
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
    if (s.len >= 5 && asciiCaseEql(s[0..4], "var(")) {
        let i: usize = 4;
        while (i < s.len && isWs(s[i])) i += 1;
        if (i + 2 >= s.len || s[i] != '-' || s[i + 1] != '-') return false;
        i += 2;
        const name_start = i;
        while (i < s.len && isCssIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len && isWs(s[i])) i += 1;
        if (i >= s.len) return false;
        if (s[i] == ')') return i + 1 == s.len;
        if (s[i] != ',') return false;
        return consumeBalancedCallRest(s, i + 1);
    }
    if (s.len >= 5 && asciiCaseEql(s[0..4], "env(")) {
        let i: usize = 4;
        while (i < s.len && isWs(s[i])) i += 1;
        if (i >= s.len || (!isCssIdentStart(s[i]) && s[i] != '-')) return false;
        const name_start = i;
        while (i < s.len && isCssIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len) {
            while (i < s.len && isWs(s[i])) i += 1;
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

/// Zero-allocation validator for CSS `display` values.
pub fn isValidCssDisplayValue(val: []const u8): bool {
    const s = trimAscii(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return Display.parse(s) != null;
}

/// Zero-allocation validator for CSS `position` values.
pub fn isValidCssPositionValue(val: []const u8): bool {
    const s = trimAscii(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return asciiCaseEql(s, "static") ||
        asciiCaseEql(s, "relative") ||
        asciiCaseEql(s, "absolute") ||
        asciiCaseEql(s, "fixed") ||
        asciiCaseEql(s, "sticky");
}

/// Zero-allocation validator for CSS `visibility` values.
pub fn isValidCssVisibilityValue(val: []const u8): bool {
    const s = trimAscii(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return Visibility.parse(s) != null;
}

/// Zero-allocation validator for CSS `box-sizing` values.
pub fn isValidCssBoxSizingValue(val: []const u8): bool {
    const s = trimAscii(val);
    if (s.len == 0) return false;
    if (isValidVarOrEnv(s)) return true;
    return asciiCaseEql(s, "content-box") || asciiCaseEql(s, "border-box");
}
