// @cap/parse5 — Zero-heap WHATWG HTML5 streaming tokenizer & attribute extractor.
// Clean-room MIT port of npm `parse5` v7 (170M+/week); reuses `@cap/unicase`.

import { eqAscii } from "../../unicase/src/index.cp";

pub const HTML_TOK_EOF: u8 = 0;
pub const HTML_TOK_DOCTYPE: u8 = 1;
pub const HTML_TOK_START_TAG: u8 = 2;
pub const HTML_TOK_END_TAG: u8 = 3;
pub const HTML_TOK_SELF_CLOSING: u8 = 4;
pub const HTML_TOK_COMMENT: u8 = 5;
pub const HTML_TOK_TEXT: u8 = 6;

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
    return b == 32 || b == 9 || b == 10 || b == 13 || b == 12;
}

fn p5IsTagNameChar(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122) ||
        (b >= 48 && b <= 57) || b == 45 || b == 95 || b == 58;
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
    let i = start_pos;
    while (i + needle.len <= s.len) {
        let j: usize = 0;
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
            while (_.pos < s.len && s[_.pos] != 60) {
                _.pos = _.pos + 1;
            }
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
            let gt = start + 2;
            while (gt < s.len && s[gt] != 62) { gt = gt + 1; }
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
            while (p < s.len && s[p] != 62) { p = p + 1; }
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
            let c = s[p];
            if (in_quote != 0) {
                if (c == in_quote) { in_quote = 0; }
            } else {
                if (c == 34 || c == 39) { in_quote = c; }
                else if (c == 62) { break; }
            }
            p = p + 1;
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
                while (p < raw_attrs.len && raw_attrs[p] != q) { p = p + 1; }
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
        if (t.kind == HTML_TOK_TEXT) {
            let i: usize = 0;
            while (i < t.text.len && p < out.len) {
                out[p] = t.text[i];
                p = p + 1;
                i = i + 1;
            }
        }
    }
    return p;
}
