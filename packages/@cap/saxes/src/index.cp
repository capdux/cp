// @cap/saxes — Zero-allocation streaming SAX XML 1.0 event parser
// Clean-room ISC implementation inspired by npm:saxes

pub const SAX_MAX_ATTRS: usize = 8;
pub const SAX_MAX_DEPTH: usize = 32;

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
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

fn saxIsNameStart(ch: u8): bool {
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    return ch == '_' || ch == ':' || ch >= 0x80;
}

fn saxIsNameChar(ch: u8): bool {
    if (saxIsNameStart(ch)) return true;
    if (ch >= '0' && ch <= '9') return true;
    return ch == '-' || ch == '.';
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
    let i = start;
    while (i + needle.len <= s.len) : (i += 1) {
        if (saxStartsWithAt(s, i, needle)) return i64(i);
    }
    return -1;
}

fn saxAllWs(s: string): bool {
    let i: usize = 0;
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
    if (ch >= '0' && ch <= '9') return i32(ch - '0');
    if (ch >= 'a' && ch <= 'f') return i32(ch - 'a') + 10;
    if (ch >= 'A' && ch <= 'F') return i32(ch - 'A') + 10;
    return -1;
}

fn saxDecodeEntitiesImpl(raw: string, out: []u8): usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < raw.len) {
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
                while (pos < xml.len && xml[pos] != '<') {
                    pos += 1;
                }
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
                const gt_pos = saxFindSubstrFrom(xml, pos + 9, ">");
                if (gt_pos < 0) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, "", "unterminated DOCTYPE"));
                    return count;
                }
                pos = usize(gt_pos) + 1;
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
                while (pos < xml.len && xml[pos] != quote) {
                    if (xml[pos] == '<') {
                        _.has_err = true;
                        count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, a_name, "unescaped '<' in attribute value"));
                        return count;
                    }
                    pos += 1;
                }
                if (pos >= xml.len) {
                    _.has_err = true;
                    count = saxEmit(out, count, saxMakeEvent(XmlEventKind.Error, a_name, "unterminated attribute value"));
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
