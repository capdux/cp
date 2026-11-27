// @cap/saxes — Zero-allocation streaming SAX XML 1.0 event parser
// Clean-room ISC implementation inspired by npm:saxes
// Accelerated with comptime 256-entry LUTs, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

type SaxVec16 = @Vector(16, u8);

pub const SAX_MAX_ATTRS: usize = 8;
pub const SAX_MAX_DEPTH: usize = 32;

const SAX_BIT_WS: u8 = 1;
const SAX_BIT_NAME_START: u8 = 2;
const SAX_BIT_NAME_CHAR: u8 = 4;

fn saxBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c == 32 || c == 9 || c == 10 || c == 13) {
            f |= SAX_BIT_WS;
        }
        const is_ns = (c >= 97 && c <= 122) || (c >= 65 && c <= 90) || c == 95 || c == 58 || c >= 0x80;
        if (is_ns) {
            f |= SAX_BIT_NAME_START | SAX_BIT_NAME_CHAR;
        } else if ((c >= 48 && c <= 57) || c == 45 || c == 46) {
            f |= SAX_BIT_NAME_CHAR;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

fn saxBuildHexLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = (c - 48) as u8;
        c += 1;
    }
    c = 65;
    while (c <= 70) {
        lut[c] = (c - 55) as u8;
        c += 1;
    }
    c = 97;
    while (c <= 102) {
        lut[c] = (c - 87) as u8;
        c += 1;
    }
    return lut;
}

const SAX_CHAR_LUT: [256]u8 = saxBuildCharLut();
const SAX_HEX_LUT: [256]u8 = saxBuildHexLut();

pub enum XmlEventKind {
    OpenTag,
    CloseTag,
    SelfClosingTag,
    Text,
    Cdata,
    Comment,
    ProcessingInstruction,
    Error,
}

pub struct XmlAttr {
    name: string,
    value: string,
}

pub struct XmlEvent {
    kind: XmlEventKind,
    name: string,
    text: string,
    attrs: [8]XmlAttr,
    attr_count: usize,
}

fn saxEmptyAttrs(): [8]XmlAttr {
    let attrs: [8]XmlAttr = undefined;
    let i: usize = 0;
    while (i < SAX_MAX_ATTRS) : (i += 1) {
        attrs[i] = XmlAttr{ name: "", value: "" };
    }
    return attrs;
}

fn saxMakeEvent(kind: XmlEventKind, name: string, text: string): XmlEvent {
    return XmlEvent{
        kind: kind,
        name: name,
        text: text,
        attrs: saxEmptyAttrs(),
        attr_count: 0,
    };
}

fn saxIsWs(ch: u8): bool {
    return (SAX_CHAR_LUT[ch as usize] & SAX_BIT_WS) != 0;
}

fn saxIsNameStart(ch: u8): bool {
    return (SAX_CHAR_LUT[ch as usize] & SAX_BIT_NAME_START) != 0;
}

fn saxIsNameChar(ch: u8): bool {
    return (SAX_CHAR_LUT[ch as usize] & SAX_BIT_NAME_CHAR) != 0;
}

fn saxFindByte(s: string, start: usize, target: u8): usize {
    let i = start;
    const len = s.len;
    const v_t: SaxVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: SaxVec16 = s[i .. i + 16][0..16].*;
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

fn saxFindAttrEnd(s: string, start: usize, quote: u8): usize {
    let i = start;
    const len = s.len;
    const v_q: SaxVec16 = @splat(quote);
    const v_lt: SaxVec16 = @splat(60 as u8);
    while (i + 16 <= len) {
        const v: SaxVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q) || @reduce(.Or, v == v_lt)) {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == quote || c == '<') { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != quote && s[i] != '<') {
        i += 1;
    }
    return i;
}

fn saxStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn saxStartsWithAt(s: string, pos: usize, prefix: string): bool {
    if (pos + prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[pos + i] != prefix[i]) return false;
    }
    return true;
}

fn saxFindSubstrFrom(s: string, start: usize, needle: string): i64 {
    if (needle.len == 0) return i64(start);
    if (start + needle.len > s.len) return -1;
    const first = needle[0];
    let i = start;
    while (i + needle.len <= s.len) {
        i = saxFindByte(s, i, first);
        if (i + needle.len > s.len) return -1;
        if (needle.len == 1 || saxStartsWithAt(s, i, needle)) return i64(i);
        i += 1;
    }
    return -1;
}

fn saxAllWs(s: string): bool {
    let i: usize = 0;
    const v_sp: SaxVec16 = @splat(32 as u8);
    const v_tab: SaxVec16 = @splat(9 as u8);
    const v_lf: SaxVec16 = @splat(10 as u8);
    const v_cr: SaxVec16 = @splat(13 as u8);
    while (i + 16 <= s.len) {
        const v: SaxVec16 = s[i .. i + 16][0..16].*;
        const is_ws = (v == v_sp) | (v == v_tab) | (v == v_lf) | (v == v_cr);
        if (!@reduce(.And, is_ws)) return false;
        i += 16;
    }
    while (i < s.len) : (i += 1) {
        if (!saxIsWs(s[i])) return false;
    }
    return true;
}

fn saxEmit(out: []XmlEvent, count: usize, ev: XmlEvent): usize {
    if (count < out.len) {
        out[count] = ev;
    }
    return count + 1;
}

fn saxEncodeUtf8(cp: u32, out: []u8, pos: usize): usize {
    let p = pos;
    if (cp <= 0x7F) {
        if (p < out.len) {
            out[p] = u8(cp);
            p += 1;
        }
    } else if (cp <= 0x7FF) {
        if (p + 2 <= out.len) {
            out[p] = u8(0xC0 | (cp >> 6));
            out[p + 1] = u8(0x80 | (cp & 0x3F));
            p += 2;
        }
    } else if (cp <= 0xFFFF) {
        if (p + 3 <= out.len) {
            out[p] = u8(0xE0 | (cp >> 12));
            out[p + 1] = u8(0x80 | ((cp >> 6) & 0x3F));
            out[p + 2] = u8(0x80 | (cp & 0x3F));
            p += 3;
        }
    } else if (cp <= 0x10FFFF) {
        if (p + 4 <= out.len) {
            out[p] = u8(0xF0 | (cp >> 18));
            out[p + 1] = u8(0x80 | ((cp >> 12) & 0x3F));
            out[p + 2] = u8(0x80 | ((cp >> 6) & 0x3F));
            out[p + 3] = u8(0x80 | (cp & 0x3F));
            p += 4;
        }
    }
    return p;
}

fn saxHexVal(ch: u8): i32 {
    const v = SAX_HEX_LUT[ch as usize];
    if (v == 255) return -1;
    return i32(v);
}

fn saxDecodeEntitiesImpl(raw: string, out: []u8): usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < raw.len) {
        const amp = saxFindByte(raw, i, '&');
        if (amp > i) {
            const avail = if (w < out.len) { out.len - w } else { 0 };
            const chunk_len = amp - i;
            const n = if (chunk_len < avail) { chunk_len } else { avail };
            if (n > 0) {
                @memcpy(out[w .. w + n], raw[i .. i + n]);
                w += n;
            }
            i = amp;
            if (i >= raw.len) break;
        }
        if (raw[i] == '&') {
            const semi_pos = saxFindSubstrFrom(raw, i + 1, ";");
            if (semi_pos > i64(i + 1)) {
                const semi = usize(semi_pos);
                const ent = raw[(i + 1)..semi];
                if (saxStrEq(ent, "lt")) {
                    if (w < out.len) { out[w] = '<'; w += 1; }
                    i = semi + 1;
                    continue;
                }
                if (saxStrEq(ent, "gt")) {
                    if (w < out.len) { out[w] = '>'; w += 1; }
                    i = semi + 1;
                    continue;
                }
                if (saxStrEq(ent, "amp")) {
                    if (w < out.len) { out[w] = '&'; w += 1; }
                    i = semi + 1;
                    continue;
                }
                if (saxStrEq(ent, "quot")) {
                    if (w < out.len) { out[w] = '"'; w += 1; }
                    i = semi + 1;
                    continue;
                }
                if (saxStrEq(ent, "apos")) {
                    if (w < out.len) { out[w] = '\''; w += 1; }
                    i = semi + 1;
                    continue;
                }
                if (ent.len >= 2 && ent[0] == '#') {
                    let ok = true;
                    let cp: u32 = 0;
                    if (ent[1] == 'x' || ent[1] == 'X') {
                        if (ent.len < 3) {
                            ok = false;
                        } else {
                            let k: usize = 2;
                            while (k < ent.len) : (k += 1) {
                                const hv = saxHexVal(ent[k]);
                                if (hv < 0 || cp > 0x10FFFF) {
                                    ok = false;
                                    break;
                                }
                                cp = (cp << 4) | u32(hv);
                            }
                        }
                    } else {
                        let k: usize = 1;
                        while (k < ent.len) : (k += 1) {
                            const c = ent[k];
                            if (c < '0' || c > '9' || cp > 0x10FFFF) {
                                ok = false;
                                break;
                            }
                            cp = cp * 10 + u32(c - '0');
                        }
                    }
                    if (ok && cp > 0 && cp <= 0x10FFFF) {
                        w = saxEncodeUtf8(cp, out, w);
                        i = semi + 1;
                        continue;
                    }
                }
            }
        }
        if (w < out.len) {
            out[w] = raw[i];
            w += 1;
        }
        i += 1;
    }
    return w;
}

pub struct SaxesParser {
    stack: [32]string,
    depth: usize,
    has_err: bool,
    root_count: usize,

    static fn new(): SaxesParser {
        let empty_stack: [32]string = undefined;
        let i: usize = 0;
        while (i < SAX_MAX_DEPTH) : (i += 1) {
            empty_stack[i] = "";
        }
        return SaxesParser{
            stack: empty_stack,
            depth: 0,
            has_err: false,
            root_count: 0,
        };
    }

    fn parseAll(*_, xml: string, out: []XmlEvent): usize {
        _.depth = 0;
        _.has_err = false;
        _.root_count = 0;

        let pos: usize = 0;
        let count: usize = 0;

        while (pos < xml.len) {
            if (xml[pos] != '<') {
                const start = pos;
                pos = saxFindByte(xml, pos + 1, '<');
                const txt = xml[start..pos];
                if (saxFindSubstrFrom(txt, 0, "]]>") >= 0) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "unexpected ]]> in text"));
                    return count;
                }
                if (_.depth == 0 && !saxAllWs(txt)) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "text outside root element"));
                    return count;
                }
                if (txt.len > 0) {
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Text, "", txt));
                }
                continue;
            }

            // CDATA: <![CDATA[ ... ]]>
            if (saxStartsWithAt(xml, pos, "<![CDATA[")) {
                const body_start = pos + 9;
                const end_pos = saxFindSubstrFrom(xml, body_start, "]]>");
                if (end_pos < 0) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "unterminated CDATA"));
                    return count;
                }
                const end_u = usize(end_pos);
                count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Cdata, "", xml[body_start..end_u]));
                pos = end_u + 3;
                continue;
            }

            // Comment: <!-- ... -->
            if (saxStartsWithAt(xml, pos, "<!--")) {
                const body_start = pos + 4;
                const end_pos = saxFindSubstrFrom(xml, body_start, "-->");
                if (end_pos < 0) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "unterminated comment"));
                    return count;
                }
                const end_u = usize(end_pos);
                const body = xml[body_start..end_u];
                if (saxFindSubstrFrom(body, 0, "--") >= 0) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "double hyphen in comment"));
                    return count;
                }
                count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Comment, "", body));
                pos = end_u + 3;
                continue;
            }

            // ProcessingInstruction: <?target data?>
            if (saxStartsWithAt(xml, pos, "<?")) {
                const body_start = pos + 2;
                const end_pos = saxFindSubstrFrom(xml, body_start, "?>");
                if (end_pos < 0) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "unterminated processing instruction"));
                    return count;
                }
                const end_u = usize(end_pos);
                let t_end = body_start;
                while (t_end < end_u && saxIsNameChar(xml[t_end])) {
                    t_end += 1;
                }
                if (t_end == body_start) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "empty processing instruction target"));
                    return count;
                }
                const target = xml[body_start..t_end];
                let d_start = t_end;
                while (d_start < end_u && saxIsWs(xml[d_start])) {
                    d_start += 1;
                }
                count = saxEmit(out, count, saxMakeEvent(XmlEventKind.ProcessingInstruction, target, xml[d_start..end_u]));
                pos = end_u + 2;
                continue;
            }

            // DOCTYPE declaration: <!DOCTYPE ...>
            if (saxStartsWithAt(xml, pos, "<!DOCTYPE")) {
                const gt_pos = saxFindByte(xml, pos + 9, '>');
                if (gt_pos >= xml.len) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "unterminated DOCTYPE"));
                    return count;
                }
                pos = gt_pos + 1;
                continue;
            }

            // Close tag: </name>
            if (saxStartsWithAt(xml, pos, "</")) {
                pos += 2;
                const name_start = pos;
                if (pos >= xml.len || !saxIsNameStart(xml[pos])) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "invalid close tag name"));
                    return count;
                }
                while (pos < xml.len && saxIsNameChar(xml[pos])) {
                    pos += 1;
                }
                const tag_name = xml[name_start..pos];
                while (pos < xml.len && saxIsWs(xml[pos])) {
                    pos += 1;
                }
                if (pos >= xml.len || xml[pos] != '>') {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "unterminated close tag"));
                    return count;
                }
                pos += 1;

                if (_.depth == 0) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "unexpected close tag"));
                    return count;
                }
                const expected = _.stack[_.depth - 1];
                if (!saxStrEq(expected, tag_name)) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "mismatched close tag"));
                    return count;
                }
                _.depth -= 1;
                count = saxEmit(out, count, saxMakeEvent(XmlEventKind.CloseTag, tag_name, ""));
                continue;
            }

            // Open or SelfClosing tag: <name ...> / <name .../>
            pos += 1;
            if (pos >= xml.len || !saxIsNameStart(xml[pos])) {
                _.has_err = true;
                count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "invalid open tag name"));
                return count;
            }
            const name_start = pos;
            while (pos < xml.len && saxIsNameChar(xml[pos])) {
                pos += 1;
            }
            const tag_name = xml[name_start..pos];

            if (_.depth == 0) {
                _.root_count += 1;
                if (_.root_count > 1) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "multiple root elements"));
                    return count;
                }
            }

            let ev = saxMakeEvent(XmlEventKind.OpenTag, tag_name, "");
            let self_closing = false;

            while (pos < xml.len) {
                const ws_start = pos;
                while (pos < xml.len && saxIsWs(xml[pos])) {
                    pos += 1;
                }
                if (pos >= xml.len) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "unterminated tag"));
                    return count;
                }
                if (xml[pos] == '>') {
                    pos += 1;
                    break;
                }
                if (xml[pos] == '/' && pos + 1 < xml.len && xml[pos + 1] == '>') {
                    self_closing = true;
                    pos += 2;
                    break;
                }
                if (pos == ws_start) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "missing whitespace before attribute"));
                    return count;
                }
                if (!saxIsNameStart(xml[pos])) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "invalid attribute name"));
                    return count;
                }
                const a_start = pos;
                while (pos < xml.len && saxIsNameChar(xml[pos])) {
                    pos += 1;
                }
                const a_name = xml[a_start..pos];
                while (pos < xml.len && saxIsWs(xml[pos])) {
                    pos += 1;
                }
                if (pos >= xml.len || xml[pos] != '=') {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, a_name, "expected '=' after attribute name"));
                    return count;
                }
                pos += 1;
                while (pos < xml.len && saxIsWs(xml[pos])) {
                    pos += 1;
                }
                if (pos >= xml.len || (xml[pos] != '"' && xml[pos] != '\'')) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, a_name, "expected quoted attribute value"));
                    return count;
                }
                const quote = xml[pos];
                pos += 1;
                const v_start = pos;
                pos = saxFindAttrEnd(xml, pos, quote);
                if (pos >= xml.len) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, a_name, "unterminated attribute value"));
                    return count;
                }
                if (xml[pos] == '<') {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, a_name, "unescaped '<' in attribute value"));
                    return count;
                }
                const a_val = xml[v_start..pos];
                pos += 1;

                // Duplicate attribute check
                let d: usize = 0;
                while (d < ev.attr_count) : (d += 1) {
                    if (saxStrEq(ev.attrs[d].name, a_name)) {
                        _.has_err = true;
                        count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, a_name, "duplicate attribute"));
                        return count;
                    }
                }
                if (ev.attr_count < SAX_MAX_ATTRS) {
                    ev.attrs[ev.attr_count] = XmlAttr{ name: a_name, value: a_val };
                    ev.attr_count += 1;
                }
            }

            if (self_closing) {
                ev.kind = XmlEventKind.SelfClosingTag;
                count = saxEmit(out, count, ev);
            } else {
                if (_.depth >= SAX_MAX_DEPTH) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, tag_name, "max nesting depth exceeded"));
                    return count;
                }
                _.stack[_.depth] = tag_name;
                _.depth += 1;
                count = saxEmit(out, count, ev);
            }
        }

        if (_.depth > 0) {
            _.has_err = true;
            count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, _.stack[_.depth - 1], "unclosed tag"));
        }
        return count;
    }

    fn isWellFormed(*_, xml: string): bool {
        let scratch: [1]XmlEvent = undefined;
        * = _.parseAll(xml, scratch[0..0]);
        return !_.has_err && _.depth == 0 && _.root_count == 1;
    }

    fn decodeEntities(raw: string, out: []u8): usize {
        const d = _.depth;
        * = d;
        return saxDecodeEntitiesImpl(raw, out);
    }
}
