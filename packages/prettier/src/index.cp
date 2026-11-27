// @cap/prettier-fmt — Zero-heap JSON & CSS pretty-printer formatter
// Clean-room MIT CP implementation inspired by npm `prettier`
// Supports configurable tabWidth, useTabs, trailingNewline, string-safe lexing.

pub struct FormatOptions {
    pub tab_width: usize,
    pub use_tabs: bool,
    pub trailing_newline: bool,

    pub static fn defaults(): FormatOptions {
        return FormatOptions {
            tab_width: 2,
            use_tabs: false,
            trailing_newline: true,
        };
    }
}

fn pfIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn pfPushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pfPushIndent(out: []u8, pos: usize, depth: usize, opts: FormatOptions): usize {
    let p = pos;
    let d: usize = 0;
    while (d < depth) {
        if (opts.use_tabs) {
            p = pfPushByte(out, p, 9); // '\t'
        } else {
            let s: usize = 0;
            while (s < opts.tab_width) {
                p = pfPushByte(out, p, 32);
                s = s + 1;
            }
        }
        d = d + 1;
    }
    return p;
}

fn pfPeekNextNonSpace(s: string, start: usize): u8 {
    let i = start;
    while (i < s.len && pfIsSpace(s[i])) {
        i = i + 1;
    }
    if (i < s.len) { return s[i]; }
    return 0;
}

/// Pretty-prints a JSON document with configurable indentation (`tab_width`, `use_tabs`, `trailing_newline`).
/// Preserves string literals verbatim and keeps empty `{}` / `[]` compact.
pub fn formatJson(input: string, opts: FormatOptions, out: []u8): usize {
    let pos: usize = 0;
    let depth: usize = 0;
    let i: usize = 0;
    let n = input.len;

    while (i < n) {
        let c = input[i];
        if (pfIsSpace(c)) {
            i = i + 1;
        } else if (c == 34) { // '"' string literal
            pos = pfPushByte(out, pos, c);
            i = i + 1;
            while (i < n) {
                let sc = input[i];
                pos = pfPushByte(out, pos, sc);
                i = i + 1;
                if (sc == 92 && i < n) {
                    pos = pfPushByte(out, pos, input[i]);
                    i = i + 1;
                } else if (sc == 34) {
                    break;
                }
            }
        } else if (c == 123 || c == 91) { // '{' or '['
            pos = pfPushByte(out, pos, c);
            let close_ch: u8 = if (c == 123) { 125 } else { 93 };
            let next = pfPeekNextNonSpace(input, i + 1);
            if (next == close_ch) {
                // Keep empty {} or [] on one line
                i = i + 1;
                while (i < n && pfIsSpace(input[i])) { i = i + 1; }
                if (i < n) {
                    pos = pfPushByte(out, pos, input[i]);
                    i = i + 1;
                }
            } else {
                depth = depth + 1;
                pos = pfPushByte(out, pos, 10); // '\n'
                pos = pfPushIndent(out, pos, depth, opts);
                i = i + 1;
            }
        } else if (c == 125 || c == 93) { // '}' or ']'
            if (depth > 0) { depth = depth - 1; }
            pos = pfPushByte(out, pos, 10);
            pos = pfPushIndent(out, pos, depth, opts);
            pos = pfPushByte(out, pos, c);
            i = i + 1;
        } else if (c == 44) { // ','
            pos = pfPushByte(out, pos, c);
            pos = pfPushByte(out, pos, 10);
            pos = pfPushIndent(out, pos, depth, opts);
            i = i + 1;
        } else if (c == 58) { // ':'
            pos = pfPushByte(out, pos, c);
            pos = pfPushByte(out, pos, 32);
            i = i + 1;
        } else {
            pos = pfPushByte(out, pos, c);
            i = i + 1;
        }
    }

    if (pos > 0 && opts.trailing_newline) {
        pos = pfPushByte(out, pos, 10);
    }
    return pos;
}

/// Pretty-prints a CSS stylesheet with clean block indentation and normalized `prop: value;` spacing.
pub fn formatCss(input: string, opts: FormatOptions, out: []u8): usize {
    let pos: usize = 0;
    let depth: usize = 0;
    let i: usize = 0;
    let n = input.len;
    let pending_space = false;

    while (i < n) {
        let c = input[i];
        if (pfIsSpace(c)) {
            if (pos > 0 && out[pos - 1] != 10 && out[pos - 1] != 32 && out[pos - 1] != 9) {
                pending_space = true;
            }
            i = i + 1;
        } else if (c == 34 || c == 39) { // string literal
            if (pending_space) { pos = pfPushByte(out, pos, 32); pending_space = false; }
            let q = c;
            pos = pfPushByte(out, pos, c);
            i = i + 1;
            while (i < n) {
                let sc = input[i];
                pos = pfPushByte(out, pos, sc);
                i = i + 1;
                if (sc == 92 && i < n) {
                    pos = pfPushByte(out, pos, input[i]);
                    i = i + 1;
                } else if (sc == q) {
                    break;
                }
            }
        } else if (c == 123) { // '{'
            pending_space = false;
            if (pos > 0 && out[pos - 1] != 32 && out[pos - 1] != 10) {
                pos = pfPushByte(out, pos, 32);
            }
            pos = pfPushByte(out, pos, 123);
            depth = depth + 1;
            let next = pfPeekNextNonSpace(input, i + 1);
            if (next != 125) {
                pos = pfPushByte(out, pos, 10);
                pos = pfPushIndent(out, pos, depth, opts);
            }
            i = i + 1;
        } else if (c == 125) { // '}'
            pending_space = false;
            if (depth > 0) { depth = depth - 1; }
            if (pos > 0 && out[pos - 1] != 10 && out[pos - 1] != 123) {
                pos = pfPushByte(out, pos, 10);
            }
            pos = pfPushIndent(out, pos, depth, opts);
            pos = pfPushByte(out, pos, 125);
            let next = pfPeekNextNonSpace(input, i + 1);
            if (next != 0) {
                pos = pfPushByte(out, pos, 10);
                if (depth == 0) {
                    pos = pfPushByte(out, pos, 10);
                } else {
                    pos = pfPushIndent(out, pos, depth, opts);
                }
            }
            i = i + 1;
        } else if (c == 59) { // ';'
            pending_space = false;
            pos = pfPushByte(out, pos, 59);
            let next = pfPeekNextNonSpace(input, i + 1);
            if (next != 0 && next != 125) {
                pos = pfPushByte(out, pos, 10);
                pos = pfPushIndent(out, pos, depth, opts);
            }
            i = i + 1;
        } else if (c == 58) { // ':'
            pending_space = false;
            pos = pfPushByte(out, pos, 58);
            if (depth > 0) {
                pos = pfPushByte(out, pos, 32);
            }
            i = i + 1;
        } else {
            if (pending_space) {
                pos = pfPushByte(out, pos, 32);
                pending_space = false;
            }
            pos = pfPushByte(out, pos, c);
            i = i + 1;
        }
    }

    if (pos > 0 && opts.trailing_newline && out[pos - 1] != 10) {
        pos = pfPushByte(out, pos, 10);
    }
    return pos;
}
