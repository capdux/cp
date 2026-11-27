// @cap/html-escape — Zero-heap HTML entity escaper & unescaper
// Clean-room MIT implementation for CP (compatible with npm escape-html, he & Rust html-escape).

import { strEq, hexVal, isDigit } from "cap:ascii";
import { writeStr } from "cap:fmt";

fn appendStr(dst: []u8, pos: usize, src: string): usize {
    if (pos + src.len > dst.len) { return 0; }
    return writeStr(src, dst, pos);
}

/// Fast check whether `input` contains any HTML-unsafe character (`&`, `<`, `>`, `"`, `'`).
pub fn needsEscape(input: string): bool {
    let i: usize = 0;
    while (i < input.len) {
        let c: u8 = input[i];
        if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'') {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Escape HTML special characters (`&` -> `&amp;`, `<` -> `&lt;`, `>` -> `&gt;`, `"` -> `&quot;`, `'` -> `&#39;`).
/// Returns bytes written to `out`, or 0 if `input` is non-empty and `out` is too small.
pub fn escapeHtml(input: string, out: []u8): usize {
    if (input.len == 0) { return 0; }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let c: u8 = input[i];
        if (c == '&') {
            pos = appendStr(out, pos, "&amp;");
            if (pos == 0) { return 0; }
        } else if (c == '<') {
            pos = appendStr(out, pos, "&lt;");
            if (pos == 0) { return 0; }
        } else if (c == '>') {
            pos = appendStr(out, pos, "&gt;");
            if (pos == 0) { return 0; }
        } else if (c == '"') {
            pos = appendStr(out, pos, "&quot;");
            if (pos == 0) { return 0; }
        } else if (c == '\'') {
            pos = appendStr(out, pos, "&#39;");
            if (pos == 0) { return 0; }
        } else {
            if (pos >= out.len) { return 0; }
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

fn encodeUtf8(cp: u32, out: []u8, pos: usize): usize {
    if (cp <= 0x7f) {
        if (pos + 1 > out.len) { return 0; }
        out[pos] = cp as u8;
        return pos + 1;
    }
    if (cp <= 0x7ff) {
        if (pos + 2 > out.len) { return 0; }
        out[pos] = (0xc0 | ((cp >> 6) & 0x1f)) as u8;
        out[pos + 1] = (0x80 | (cp & 0x3f)) as u8;
        return pos + 2;
    }
    if (cp <= 0xffff) {
        if (pos + 3 > out.len) { return 0; }
        out[pos] = (0xe0 | ((cp >> 12) & 0x0f)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 6) & 0x3f)) as u8;
        out[pos + 2] = (0x80 | (cp & 0x3f)) as u8;
        return pos + 3;
    }
    if (cp <= 0x10ffff) {
        if (pos + 4 > out.len) { return 0; }
        out[pos] = (0xf0 | ((cp >> 18) & 0x07)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 12) & 0x3f)) as u8;
        out[pos + 2] = (0x80 | ((cp >> 6) & 0x3f)) as u8;
        out[pos + 3] = (0x80 | (cp & 0x3f)) as u8;
        return pos + 4;
    }
    return 0;
}

fn decodeNamedEntity(name: string): u32 {
    if (strEq(name, "amp")) { return 0x26; }
    if (strEq(name, "lt")) { return 0x3c; }
    if (strEq(name, "gt")) { return 0x3e; }
    if (strEq(name, "quot")) { return 0x22; }
    if (strEq(name, "apos")) { return 0x27; }
    if (strEq(name, "nbsp")) { return 0xa0; }
    if (strEq(name, "copy")) { return 0xa9; }
    if (strEq(name, "reg")) { return 0xae; }
    if (strEq(name, "trade")) { return 0x2122; }
    if (strEq(name, "mdash")) { return 0x2014; }
    if (strEq(name, "ndash")) { return 0x2013; }
    if (strEq(name, "hellip")) { return 0x2026; }
    if (strEq(name, "euro")) { return 0x20ac; }
    if (strEq(name, "pound")) { return 0xa3; }
    if (strEq(name, "yen")) { return 0xa5; }
    if (strEq(name, "times")) { return 0xd7; }
    if (strEq(name, "divide")) { return 0xf7; }
    return 0;
}

fn decodeNumericEntity(body: string): u32 {
    if (body.len < 2 || body[0] != '#') { return 0; }
    let cp: u32 = 0;
    if (body[1] == 'x' || body[1] == 'X') {
        if (body.len < 3 || body.len > 8) { return 0; }
        let i: usize = 2;
        while (i < body.len) {
            const d_opt = hexVal(body[i]);
            if (d_opt == null) { return 0; }
            cp = (cp << 4) | (d_opt.? as u32);
            if (cp > 0x10ffff) { return 0; }
            i += 1;
        }
    } else {
        if (body.len > 8) { return 0; }
        let i: usize = 1;
        while (i < body.len) {
            let c: u8 = body[i];
            if (!isDigit(c)) { return 0; }
            cp = cp * 10 + ((c - '0') as u32);
            if (cp > 0x10ffff) { return 0; }
            i += 1;
        }
    }
    // Reject surrogate range and NUL
    if (cp == 0 || (cp >= 0xd800 && cp <= 0xdfff)) { return 0; }
    return cp;
}

/// Unescape HTML named entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`, `&nbsp;`, `&copy;`, ...)
/// and decimal/hex numeric character references (`&#39;`, `&#x27;`, `&#x1F600;`) into UTF-8 `out`.
/// Unrecognized entities are copied verbatim. Returns bytes written to `out`.
pub fn unescapeHtml(input: string, out: []u8): usize {
    if (input.len == 0) { return 0; }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == '&') {
            let semi: usize = i + 1;
            let limit: usize = i + 12;
            if (limit > input.len) { limit = input.len; }
            let found_semi: bool = false;
            while (semi < limit) {
                if (input[semi] == ';') {
                    found_semi = true;
                    break;
                }
                if (input[semi] == '&' || input[semi] == ' ') {
                    break;
                }
                semi += 1;
            }
            if (found_semi && semi > i + 1) {
                let body: string = input[(i + 1)..semi];
                let cp: u32 = 0;
                if (body[0] == '#') {
                    cp = decodeNumericEntity(body);
                } else {
                    cp = decodeNamedEntity(body);
                }
                if (cp != 0) {
                    let next_pos: usize = encodeUtf8(cp, out, pos);
                    if (next_pos == 0) { return 0; }
                    pos = next_pos;
                    i = semi + 1;
                    continue;
                }
            }
        }
        if (pos >= out.len) { return 0; }
        out[pos] = input[i];
        pos += 1;
        i += 1;
    }
    return pos;
}
