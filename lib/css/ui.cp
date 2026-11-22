// Source: Absorbed from /data/dev/bun/src/css/properties/ui.cp | Synced: 2026-11-22T12:49:17Z
//! CSS Basic User Interface Level 4 properties:
//! `color-scheme`, `resize`, `user-select`, `caret-shape`, `appearance`, and `cursor`.

export struct ColorScheme {
    light: bool = false,
    dark: bool = false,
    only: bool = false,

    pub static fn normal(): ColorScheme {
        return ColorScheme{ light: false, dark: false, only: false };
    }

    pub fn isNormal(): bool {
        return !_.light && !_.dark && !_.only;
    }

    pub fn eql(other: ColorScheme): bool {
        return _.light == other.light && _.dark == other.dark && _.only == other.only;
    }

    pub static fn parse(input: []const u8): ColorScheme? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let res = ColorScheme.normal();
        let pos: usize = 0;
        let tok_idx: usize = 0;
        let saw_normal = false;

        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            const tok = s[start..pos];

            if (asciiCaseEql(tok, "normal")) {
                if (tok_idx > 0) return null;
                saw_normal = true;
            } else if (saw_normal) {
                return null;
            } else if (asciiCaseEql(tok, "only")) {
                if (res.only) return null;
                res.only = true;
                // If `only` appeared after light/dark, it must be the last token
                if (res.light || res.dark) {
                    while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
                    if (pos < s.len) return null;
                }
            } else if (asciiCaseEql(tok, "light")) {
                res.light = true;
            } else if (asciiCaseEql(tok, "dark")) {
                res.dark = true;
            } else {
                return null;
            }
            tok_idx += 1;
        }

        if (tok_idx == 0) return null;
        if (res.only && !res.light && !res.dark) return null;
        return res;
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.isNormal()) return copyStr(out, "normal");
        let len: usize = 0;
        if (_.light) {
            len = appendStr(out, len, "light");
            if (_.dark) len = appendStr(out, len, " ");
        }
        if (_.dark) {
            len = appendStr(out, len, "dark");
        }
        if (_.only) {
            len = appendStr(out, len, " only");
        }
        return out[0..len];
    }
}

export enum Resize: u8 {
    none = 0,
    both = 1,
    horizontal = 2,
    vertical = 3,
    block = 4,
    inline_axis = 5,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .none => "none",
            .both => "both",
            .horizontal => "horizontal",
            .vertical => "vertical",
            .block => "block",
            .inline_axis => "inline",
        };
    }

    pub static fn parse(input: []const u8): Resize? {
        const s = trimAscii(input);
        if (asciiCaseEql(s, "none")) return .none;
        if (asciiCaseEql(s, "both")) return .both;
        if (asciiCaseEql(s, "horizontal")) return .horizontal;
        if (asciiCaseEql(s, "vertical")) return .vertical;
        if (asciiCaseEql(s, "block")) return .block;
        if (asciiCaseEql(s, "inline")) return .inline_axis;
        return null;
    }
}

export enum UserSelect: u8 {
    auto = 0,
    text = 1,
    none = 2,
    contain = 3,
    all = 4,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .auto => "auto",
            .text => "text",
            .none => "none",
            .contain => "contain",
            .all => "all",
        };
    }

    pub static fn parse(input: []const u8): UserSelect? {
        const s = trimAscii(input);
        if (asciiCaseEql(s, "auto")) return .auto;
        if (asciiCaseEql(s, "text")) return .text;
        if (asciiCaseEql(s, "none")) return .none;
        if (asciiCaseEql(s, "contain")) return .contain;
        if (asciiCaseEql(s, "all")) return .all;
        return null;
    }
}

export enum CaretShape: u8 {
    auto = 0,
    bar = 1,
    block = 2,
    underscore = 3,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .auto => "auto",
            .bar => "bar",
            .block => "block",
            .underscore => "underscore",
        };
    }

    pub static fn parse(input: []const u8): CaretShape? {
        const s = trimAscii(input);
        if (asciiCaseEql(s, "auto")) return .auto;
        if (asciiCaseEql(s, "bar")) return .bar;
        if (asciiCaseEql(s, "block")) return .block;
        if (asciiCaseEql(s, "underscore")) return .underscore;
        return null;
    }
}

export enum Appearance: u8 {
    none = 0,
    auto = 1,
    textfield = 2,
    menulist_button = 3,
    button = 4,
    checkbox = 5,
    listbox = 6,
    menulist = 7,
    meter = 8,
    progress_bar = 9,
    push_button = 10,
    radio = 11,
    searchfield = 12,
    slider_horizontal = 13,
    square_button = 14,
    textarea = 15,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .none => "none",
            .auto => "auto",
            .textfield => "textfield",
            .menulist_button => "menulist-button",
            .button => "button",
            .checkbox => "checkbox",
            .listbox => "listbox",
            .menulist => "menulist",
            .meter => "meter",
            .progress_bar => "progress-bar",
            .push_button => "push-button",
            .radio => "radio",
            .searchfield => "searchfield",
            .slider_horizontal => "slider-horizontal",
            .square_button => "square-button",
            .textarea => "textarea",
        };
    }

    pub static fn parse(input: []const u8): Appearance? {
        const s = trimAscii(input);
        if (asciiCaseEql(s, "none")) return .none;
        if (asciiCaseEql(s, "auto")) return .auto;
        if (asciiCaseEql(s, "textfield")) return .textfield;
        if (asciiCaseEql(s, "menulist-button")) return .menulist_button;
        if (asciiCaseEql(s, "button")) return .button;
        if (asciiCaseEql(s, "checkbox")) return .checkbox;
        if (asciiCaseEql(s, "listbox")) return .listbox;
        if (asciiCaseEql(s, "menulist")) return .menulist;
        if (asciiCaseEql(s, "meter")) return .meter;
        if (asciiCaseEql(s, "progress-bar")) return .progress_bar;
        if (asciiCaseEql(s, "push-button")) return .push_button;
        if (asciiCaseEql(s, "radio")) return .radio;
        if (asciiCaseEql(s, "searchfield")) return .searchfield;
        if (asciiCaseEql(s, "slider-horizontal")) return .slider_horizontal;
        if (asciiCaseEql(s, "square-button")) return .square_button;
        if (asciiCaseEql(s, "textarea")) return .textarea;
        return null;
    }

    pub fn isCompatAuto(): bool {
        const v = @intFromEnum(_);
        return v >= 4 && v <= 15;
    }
}

export enum CursorKeyword: u8 {
    auto = 0,
    default_cursor = 1,
    none = 2,
    context_menu = 3,
    help = 4,
    pointer = 5,
    progress = 6,
    wait = 7,
    cell = 8,
    crosshair = 9,
    text = 10,
    vertical_text = 11,
    alias = 12,
    copy = 13,
    move_cursor = 14,
    no_drop = 15,
    not_allowed = 16,
    grab = 17,
    grabbing = 18,
    e_resize = 19,
    n_resize = 20,
    ne_resize = 21,
    nw_resize = 22,
    s_resize = 23,
    se_resize = 24,
    sw_resize = 25,
    w_resize = 26,
    ew_resize = 27,
    ns_resize = 28,
    nesw_resize = 29,
    nwse_resize = 30,
    col_resize = 31,
    row_resize = 32,
    all_scroll = 33,
    zoom_in = 34,
    zoom_out = 35,

    pub fn toCss(): []const u8 {
        return switch (_) {
            .auto => "auto",
            .default_cursor => "default",
            .none => "none",
            .context_menu => "context-menu",
            .help => "help",
            .pointer => "pointer",
            .progress => "progress",
            .wait => "wait",
            .cell => "cell",
            .crosshair => "crosshair",
            .text => "text",
            .vertical_text => "vertical-text",
            .alias => "alias",
            .copy => "copy",
            .move_cursor => "move",
            .no_drop => "no-drop",
            .not_allowed => "not-allowed",
            .grab => "grab",
            .grabbing => "grabbing",
            .e_resize => "e-resize",
            .n_resize => "n-resize",
            .ne_resize => "ne-resize",
            .nw_resize => "nw-resize",
            .s_resize => "s-resize",
            .se_resize => "se-resize",
            .sw_resize => "sw-resize",
            .w_resize => "w-resize",
            .ew_resize => "ew-resize",
            .ns_resize => "ns-resize",
            .nesw_resize => "nesw-resize",
            .nwse_resize => "nwse-resize",
            .col_resize => "col-resize",
            .row_resize => "row-resize",
            .all_scroll => "all-scroll",
            .zoom_in => "zoom-in",
            .zoom_out => "zoom-out",
        };
    }

    pub static fn parse(input: []const u8): CursorKeyword? {
        const s = trimAscii(input);
        if (asciiCaseEql(s, "auto")) return .auto;
        if (asciiCaseEql(s, "default")) return .default_cursor;
        if (asciiCaseEql(s, "none")) return .none;
        if (asciiCaseEql(s, "context-menu")) return .context_menu;
        if (asciiCaseEql(s, "help")) return .help;
        if (asciiCaseEql(s, "pointer")) return .pointer;
        if (asciiCaseEql(s, "progress")) return .progress;
        if (asciiCaseEql(s, "wait")) return .wait;
        if (asciiCaseEql(s, "cell")) return .cell;
        if (asciiCaseEql(s, "crosshair")) return .crosshair;
        if (asciiCaseEql(s, "text")) return .text;
        if (asciiCaseEql(s, "vertical-text")) return .vertical_text;
        if (asciiCaseEql(s, "alias")) return .alias;
        if (asciiCaseEql(s, "copy")) return .copy;
        if (asciiCaseEql(s, "move")) return .move_cursor;
        if (asciiCaseEql(s, "no-drop")) return .no_drop;
        if (asciiCaseEql(s, "not-allowed")) return .not_allowed;
        if (asciiCaseEql(s, "grab")) return .grab;
        if (asciiCaseEql(s, "grabbing")) return .grabbing;
        if (asciiCaseEql(s, "e-resize")) return .e_resize;
        if (asciiCaseEql(s, "n-resize")) return .n_resize;
        if (asciiCaseEql(s, "ne-resize")) return .ne_resize;
        if (asciiCaseEql(s, "nw-resize")) return .nw_resize;
        if (asciiCaseEql(s, "s-resize")) return .s_resize;
        if (asciiCaseEql(s, "se-resize")) return .se_resize;
        if (asciiCaseEql(s, "sw-resize")) return .sw_resize;
        if (asciiCaseEql(s, "w-resize")) return .w_resize;
        if (asciiCaseEql(s, "ew-resize")) return .ew_resize;
        if (asciiCaseEql(s, "ns-resize")) return .ns_resize;
        if (asciiCaseEql(s, "nesw-resize")) return .nesw_resize;
        if (asciiCaseEql(s, "nwse-resize")) return .nwse_resize;
        if (asciiCaseEql(s, "col-resize")) return .col_resize;
        if (asciiCaseEql(s, "row-resize")) return .row_resize;
        if (asciiCaseEql(s, "all-scroll")) return .all_scroll;
        if (asciiCaseEql(s, "zoom-in")) return .zoom_in;
        if (asciiCaseEql(s, "zoom-out")) return .zoom_out;
        return null;
    }

    pub fn isResize(): bool {
        const v = @intFromEnum(_);
        return v >= 19 && v <= 32;
    }
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
