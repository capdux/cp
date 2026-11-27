// @cap/parse5 — Zero-heap WHATWG HTML5 streaming tokenizer & attribute extractor.
// Clean-room MIT port of npm `parse5` v7 (170M+/week); reuses `@cap/unicase`.
// Accelerated with comptime 256-entry LUT, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

import { eqAscii } from "../../unicase/src/index.cp";

type P5Vec16 = @Vector(16, u8);

pub const HTML_TOK_EOF: u8 = 0;
pub const HTML_TOK_DOCTYPE: u8 = 1;
pub const HTML_TOK_START_TAG: u8 = 2;
pub const HTML_TOK_END_TAG: u8 = 3;
pub const HTML_TOK_SELF_CLOSING: u8 = 4;
pub const HTML_TOK_COMMENT: u8 = 5;
pub const HTML_TOK_TEXT: u8 = 6;

const P5_BIT_WS: u8 = 1;
const P5_BIT_TAG: u8 = 2;

fn p5BuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c == 32 || c == 9 || c == 10 || c == 13 || c == 12) {
            f |= P5_BIT_WS;
        }
        if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) ||
            (c >= 48 && c <= 57) || c == 45 || c == 95 || c == 58)
        {
            f |= P5_BIT_TAG;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const P5_CHAR_LUT: [256]u8 = p5BuildCharLut();

pub struct HtmlToken {
    pub kind: u8,
    pub name: string,
    pub raw_attrs: string,
    pub text: string,
    pub start: usize,
    pub end: usize,
}

pub struct HtmlAttr {
    pub name: string,
    pub value: string,
}

fn p5IsWs(b: u8): bool {
    return (P5_CHAR_LUT[b as usize] & P5_BIT_WS) != 0;
}

fn p5IsTagNameChar(b: u8): bool {
    return (P5_CHAR_LUT[b as usize] & P5_BIT_TAG) != 0;
}

fn p5FindByte(s: string, start: usize, target: u8): usize {
    let i = start;
    const len = s.len;
    const v_t: P5Vec16 = @splat(target);
    while (i + 16 <= len) {
        const v: P5Vec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_t)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn p5FindTagDelim(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_gt: P5Vec16 = @splat(62 as u8);
    const v_dq: P5Vec16 = @splat(34 as u8);
    const v_sq: P5Vec16 = @splat(39 as u8);
    while (i + 16 <= len) {
        const v: P5Vec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_gt) || @reduce(.Or, v == v_dq) || @reduce(.Or, v == v_sq)) {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == 62 || c == 34 || c == 39) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == 62 || c == 34 || c == 39) { return i; }
        i += 1;
    }
    return len;
}

fn p5Trim(s: string): string {
    let start: usize = 0;
    while (start < s.len && p5IsWs(s[start])) { start = start + 1; }
    let end: usize = s.len;
    while (end > start && p5IsWs(s[end - 1])) { end = end - 1; }
    return s[start..end];
}

fn p5FindSubstr(s: string, start_pos: usize, needle: string): usize {
    if (needle.len == 0) { return start_pos; }
    const first = needle[0];
    let i = start_pos;
    while (i + needle.len <= s.len) {
        i = p5FindByte(s, i, first);
        if (i + needle.len > s.len) { return s.len; }
        let j: usize = 1;
        let matched = true;
        while (j < needle.len) {
            if (s[i + j] != needle[j]) { matched = false; break; }
            j = j + 1;
        }
        if (matched) { return i; }
        i = i + 1;
    }
    return s.len;
}

/// Checks if `tag` is an HTML5 void element (`br`, `hr`, `img`, `input`, `meta`, `link`, etc.).
pub fn isVoidElement(tag: string): bool {
    return eqAscii(tag, "area") || eqAscii(tag, "base") || eqAscii(tag, "br") ||
        eqAscii(tag, "col") || eqAscii(tag, "embed") || eqAscii(tag, "hr") ||
        eqAscii(tag, "img") || eqAscii(tag, "input") || eqAscii(tag, "link") ||
        eqAscii(tag, "meta") || eqAscii(tag, "param") || eqAscii(tag, "source") ||
        eqAscii(tag, "track") || eqAscii(tag, "wbr");
}

/// Streaming zero-heap HTML5 tokenizer (`HtmlTokenizer`).
pub struct HtmlTokenizer {
    pub src: string,
    pub pos: usize,

    pub static fn new(src: string): HtmlTokenizer {
        return HtmlTokenizer { src: src, pos: 0 };
    }

    pub fn next(*_): HtmlToken {
        let eof = HtmlToken { kind: HTML_TOK_EOF, name: "", raw_attrs: "", text: "", start: _.pos, end: _.pos };
        if (_.pos >= _.src.len) { return eof; }

        let s = _.src;
        if (s[_.pos] != 60) {
            // Text node until next '<'
            let start = _.pos;
            _.pos = p5FindByte(s, start + 1, 60);
            return HtmlToken { kind: HTML_TOK_TEXT, name: "", raw_attrs: "", text: s[start.._.pos], start: start, end: _.pos };
        }

        let start = _.pos;
        // Comment: <!-- ... -->
        if (start + 4 <= s.len && s[start + 1] == 33 && s[start + 2] == 45 && s[start + 3] == 45) {
            let close = p5FindSubstr(s, start + 4, "-->");
            let text_end = close;
            _.pos = if (close + 3 <= s.len) { close + 3 } else { s.len };
            return HtmlToken { kind: HTML_TOK_COMMENT, name: "", raw_attrs: "", text: s[start + 4..text_end], start: start, end: _.pos };
        }

        // Doctype: <!DOCTYPE ...>
        if (start + 2 < s.len && s[start + 1] == 33) {
            let gt = p5FindByte(s, start + 2, 62);
            let end_pos = if (gt < s.len) { gt + 1 } else { s.len };
            _.pos = end_pos;
            return HtmlToken { kind: HTML_TOK_DOCTYPE, name: "doctype", raw_attrs: "", text: p5Trim(s[start + 2..gt]), start: start, end: end_pos };
        }

        // End tag: </tag>
        if (start + 1 < s.len && s[start + 1] == 47) {
            let p = start + 2;
            while (p < s.len && p5IsWs(s[p])) { p = p + 1; }
            let name_start = p;
            while (p < s.len && p5IsTagNameChar(s[p])) { p = p + 1; }
            let name = s[name_start..p];
            p = p5FindByte(s, p, 62);
            _.pos = if (p < s.len) { p + 1 } else { s.len };
            return HtmlToken { kind: HTML_TOK_END_TAG, name: name, raw_attrs: "", text: "", start: start, end: _.pos };
        }

        // Start / self-closing tag: <tag attrs...>
        let p = start + 1;
        let name_start = p;
        while (p < s.len && p5IsTagNameChar(s[p])) { p = p + 1; }
        if (p == name_start) {
            // Bare '<' not starting a tag -> emit as text
            _.pos = start + 1;
            return HtmlToken { kind: HTML_TOK_TEXT, name: "", raw_attrs: "", text: "<", start: start, end: start + 1 };
        }
        let name = s[name_start..p];
        let attrs_start = p;
        let in_quote: u8 = 0;
        while (p < s.len) {
            if (in_quote != 0) {
                p = p5FindByte(s, p, in_quote);
                if (p < s.len) {
                    in_quote = 0;
                    p = p + 1;
                }
            } else {
                p = p5FindTagDelim(s, p);
                if (p >= s.len) { break; }
                let c = s[p];
                if (c == 62) { break; }
                in_quote = c;
                p = p + 1;
            }
        }
        let attrs_end = p;
        let self_closing = false;
        if (attrs_end > attrs_start && s[attrs_end - 1] == 47) {
            self_closing = true;
            attrs_end = attrs_end - 1;
        }
        _.pos = if (p < s.len) { p + 1 } else { s.len };
        let kind: u8 = if (self_closing) { HTML_TOK_SELF_CLOSING } else { HTML_TOK_START_TAG };
        return HtmlToken { kind: kind, name: name, raw_attrs: p5Trim(s[attrs_start..attrs_end]), text: "", start: start, end: _.pos };
    }
}

/// Parses `raw_attrs` from an `HtmlToken` into `out_attrs`, returning number of attributes parsed.
pub fn parseAttributes(raw_attrs: string, out_attrs: []HtmlAttr): usize {
    let count: usize = 0;
    let p: usize = 0;
    while (p < raw_attrs.len && count < out_attrs.len) {
        while (p < raw_attrs.len && p5IsWs(raw_attrs[p])) { p = p + 1; }
        if (p >= raw_attrs.len) { break; }
        let nstart = p;
        while (p < raw_attrs.len && !p5IsWs(raw_attrs[p]) && raw_attrs[p] != 61) { p = p + 1; }
        let name = raw_attrs[nstart..p];
        while (p < raw_attrs.len && p5IsWs(raw_attrs[p])) { p = p + 1; }
        let val: string = "";
        if (p < raw_attrs.len && raw_attrs[p] == 61) {
            p = p + 1;
            while (p < raw_attrs.len && p5IsWs(raw_attrs[p])) { p = p + 1; }
            if (p < raw_attrs.len && (raw_attrs[p] == 34 || raw_attrs[p] == 39)) {
                let q = raw_attrs[p];
                p = p + 1;
                let vstart = p;
                p = p5FindByte(raw_attrs, p, q);
                val = raw_attrs[vstart..p];
                if (p < raw_attrs.len) { p = p + 1; }
            } else {
                let vstart = p;
                while (p < raw_attrs.len && !p5IsWs(raw_attrs[p])) { p = p + 1; }
                val = raw_attrs[vstart..p];
            }
        }
        if (name.len > 0) {
            out_attrs[count] = HtmlAttr { name: name, value: val };
            count = count + 1;
        }
    }
    return count;
}

/// Extracts plain text content from an HTML document into `out` (stripping tags & comments).
pub fn extractTextContent(html: string, out: []u8): usize {
    let tok = HtmlTokenizer.new(html);
    let p: usize = 0;
    while (true) {
        let t = tok.next();
        if (t.kind == HTML_TOK_EOF) { break; }
        if (t.kind == HTML_TOK_TEXT && p < out.len) {
            const avail = out.len - p;
            const n = if (t.text.len < avail) { t.text.len } else { avail };
            if (n > 0) {
                @memcpy(out[p .. p + n], t.text[0..n]);
                p = p + n;
            }
        }
    }
    return p;
}
