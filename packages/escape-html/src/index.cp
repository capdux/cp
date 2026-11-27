// @cap/escape-html — Zero-heap single-pass HTML entity escaper & unescaper in pure CP
// MIT License — Clean-room implementation inspired by npm `escape-html`

import { startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";

/// Return true if `input` contains any of `&`, `<`, `>`, `"`, or `'`.
pub fn needsHtmlEscape(input: string): bool {
    let i: usize = 0;
    while (i < input.len) {
        const ch: u8 = input[i];
        if (ch == 38 || ch == 60 || ch == 62 || ch == 34 || ch == 39) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Precompute the exact escaped byte length of `input` in O(n) time without writing.
pub fn escapedHtmlLen(input: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const ch: u8 = input[i];
        if (ch == 38) { // '&' -> "&amp;"
            total += 5;
        } else if (ch == 60 || ch == 62) { // '<' / '>' -> "&lt;" / "&gt;"
            total += 4;
        } else if (ch == 34) { // '"' -> "&quot;"
            total += 6;
        } else if (ch == 39) { // '\'' -> "&#39;"
            total += 5;
        } else {
            total += 1;
        }
        i += 1;
    }
    return total;
}

/// Escape HTML special characters (`&`, `<`, `>`, `"`, `'`) into `out` in a single pass.
/// Safely truncates at `out.len` if `out` is smaller than `escapedHtmlLen(input)`.
/// Returns the number of bytes written to `out`.
pub fn escapeHtml(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len && w < out.len) {
        const ch: u8 = input[i];
        if (ch == 38) { // '&'
            w = writeStr("&amp;", out, w);
        } else if (ch == 60) { // '<'
            w = writeStr("&lt;", out, w);
        } else if (ch == 62) { // '>'
            w = writeStr("&gt;", out, w);
        } else if (ch == 34) { // '"'
            w = writeStr("&quot;", out, w);
        } else if (ch == 39) { // '\''
            w = writeStr("&#39;", out, w);
        } else {
            out[w] = ch;
            w += 1;
        }
        i += 1;
    }
    return w;
}

/// Unescape standard HTML entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&#x27;`, `&apos;`)
/// into `out`. Unknown or malformed entities (e.g. `&foo;`) are preserved verbatim.
/// Returns the number of bytes written to `out`.
pub fn unescapeHtml(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len && w < out.len) {
        if (input[i] == 38) { // '&'
            const rest = input[i..input.len];
            if (startsWith(rest, "&amp;")) {
                out[w] = 38;
                w += 1;
                i += 5;
                continue;
            }
            if (startsWith(rest, "&lt;")) {
                out[w] = 60;
                w += 1;
                i += 4;
                continue;
            }
            if (startsWith(rest, "&gt;")) {
                out[w] = 62;
                w += 1;
                i += 4;
                continue;
            }
            if (startsWith(rest, "&quot;")) {
                out[w] = 34;
                w += 1;
                i += 6;
                continue;
            }
            if (startsWith(rest, "&#39;")) {
                out[w] = 39;
                w += 1;
                i += 5;
                continue;
            }
            if (startsWith(rest, "&#x27;") || startsWith(rest, "&#X27;")) {
                out[w] = 39;
                w += 1;
                i += 6;
                continue;
            }
            if (startsWith(rest, "&apos;")) {
                out[w] = 39;
                w += 1;
                i += 6;
                continue;
            }
        }
        out[w] = input[i];
        w += 1;
        i += 1;
    }
    return w;
}
