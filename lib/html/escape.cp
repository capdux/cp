// lib/html/escape.cp
// Zero-copy fast-path HTML Entity Escaper & Unescaper
// Absorbed from Bun src/md/entity.cp & HTML escaping utilities

const std = @import("std");

pub struct HtmlEscape {
    /// Returns `true` if `input` contains any HTML-special character (`&`, `<`, `>`, `"`, `'`).
    pub static fn needsEscape(input: []const u8): bool {
        let i: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'') {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    /// Escapes `&`, `<`, `>`, `"`, and `'` into standard HTML entities in `out_buf`.
    /// If no characters require escaping, returns `input` directly (zero-copy fast path) when `out_buf` is not needed,
    /// or copies into `out_buf` so the caller always receives a valid slice.
    pub static fn escapeToBuf(input: []const u8, out_buf: []u8): []const u8 {
        if (!HtmlEscape.needsEscape(input)) {
            if (input.len <= out_buf.len) {
                const end = HtmlEscape.appendSlice(out_buf, 0, input);
                return out_buf[0..end];
            }
            return input;
        }

        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len && pos < out_buf.len) {
            const c = input[i];
            if (c == '&') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&amp;");
            } else if (c == '<') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&lt;");
            } else if (c == '>') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&gt;");
            } else if (c == '"') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&quot;");
            } else if (c == '\'') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&#39;");
            } else {
                out_buf[pos] = c;
                pos += 1;
            }
            i += 1;
        }
        return out_buf[0..pos];
    }

    /// Unescapes standard HTML entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&apos;`) into `out_buf`.
    pub static fn unescapeToBuf(input: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len && pos < out_buf.len) {
            if (input[i] == '&') {
                const rem = input[i..];
                if (std.mem.startsWith(u8, rem, "&amp;")) {
                    out_buf[pos] = '&';
                    pos += 1;
                    i += 5;
                    continue;
                }
                if (std.mem.startsWith(u8, rem, "&lt;")) {
                    out_buf[pos] = '<';
                    pos += 1;
                    i += 4;
                    continue;
                }
                if (std.mem.startsWith(u8, rem, "&gt;")) {
                    out_buf[pos] = '>';
                    pos += 1;
                    i += 4;
                    continue;
                }
                if (std.mem.startsWith(u8, rem, "&quot;")) {
                    out_buf[pos] = '"';
                    pos += 1;
                    i += 6;
                    continue;
                }
                if (std.mem.startsWith(u8, rem, "&#39;")) {
                    out_buf[pos] = '\'';
                    pos += 1;
                    i += 5;
                    continue;
                }
                if (std.mem.startsWith(u8, rem, "&apos;")) {
                    out_buf[pos] = '\'';
                    pos += 1;
                    i += 6;
                    continue;
                }
            }
            out_buf[pos] = input[i];
            pos += 1;
            i += 1;
        }
        return out_buf[0..pos];
    }
}
