// @cap/dom-serializer — Zero-heap HTML/XML DOM node serializer
// Clean-room CP implementation inspired by dom-serializer (MIT).

pub enum DomNodeKind {
    Element,
    Text,
    Comment,
    Cdata,
    Doctype,
}

pub struct DomAttr {
    name: string,
    value: string,
}

pub struct SerializeOptions {
    xml_mode: bool,
    self_closing_tags: bool,
}

fn dsrlToLowerByte(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn dsrlEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (dsrlToLowerByte(a[i]) != dsrlToLowerByte(b[i])) return false;
    }
    return true;
}

fn dsrlWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) return pos;
    out[pos] = b;
    return pos + 1;
}

fn dsrlWriteStr(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn dsrlWriteAttrValue(out: []u8, pos: usize, val: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        const c: u8 = val[i];
        if (c == '&') {
            p = dsrlWriteStr(out, p, "&amp;");
        } else if (c == '"') {
            p = dsrlWriteStr(out, p, "&quot;");
        } else {
            p = dsrlWriteByte(out, p, c);
        }
    }
    return p;
}

pub fn isVoidElement(tag: string): bool {
    if (dsrlEqIgnoreCase(tag, "area")) return true;
    if (dsrlEqIgnoreCase(tag, "base")) return true;
    if (dsrlEqIgnoreCase(tag, "br")) return true;
    if (dsrlEqIgnoreCase(tag, "col")) return true;
    if (dsrlEqIgnoreCase(tag, "embed")) return true;
    if (dsrlEqIgnoreCase(tag, "hr")) return true;
    if (dsrlEqIgnoreCase(tag, "img")) return true;
    if (dsrlEqIgnoreCase(tag, "input")) return true;
    if (dsrlEqIgnoreCase(tag, "link")) return true;
    if (dsrlEqIgnoreCase(tag, "meta")) return true;
    if (dsrlEqIgnoreCase(tag, "param")) return true;
    if (dsrlEqIgnoreCase(tag, "source")) return true;
    if (dsrlEqIgnoreCase(tag, "track")) return true;
    if (dsrlEqIgnoreCase(tag, "wbr")) return true;
    return false;
}

pub fn serializeText(text: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const c: u8 = text[i];
        if (c == '&') {
            p = dsrlWriteStr(out, p, "&amp;");
        } else if (c == '<') {
            p = dsrlWriteStr(out, p, "&lt;");
        } else if (c == '>') {
            p = dsrlWriteStr(out, p, "&gt;");
        } else {
            p = dsrlWriteByte(out, p, c);
        }
    }
    return p;
}

pub fn serializeOpenTag(
    tag: string,
    attrs: []const DomAttr,
    self_close: bool,
    out: []u8,
): usize {
    let p: usize = 0;
    p = dsrlWriteByte(out, p, '<');
    p = dsrlWriteStr(out, p, tag);

    let i: usize = 0;
    while (i < attrs.len) : (i += 1) {
        p = dsrlWriteByte(out, p, ' ');
        p = dsrlWriteStr(out, p, attrs[i].name);
        p = dsrlWriteStr(out, p, "=\"");
        p = dsrlWriteAttrValue(out, p, attrs[i].value);
        p = dsrlWriteByte(out, p, '"');
    }

    if (self_close) {
        p = dsrlWriteStr(out, p, "/>");
    } else {
        p = dsrlWriteByte(out, p, '>');
    }
    return p;
}

pub fn serializeElement(
    tag: string,
    attrs: []const DomAttr,
    inner_html: string,
    opts: SerializeOptions,
    out: []u8,
): usize {
    if (opts.xml_mode && inner_html.len == 0) {
        return serializeOpenTag(tag, attrs, true, out);
    }

    if (!opts.xml_mode && isVoidElement(tag)) {
        return serializeOpenTag(tag, attrs, opts.self_closing_tags, out);
    }

    let p: usize = serializeOpenTag(tag, attrs, false, out);
    p = dsrlWriteStr(out, p, inner_html);
    p = dsrlWriteStr(out, p, "</");
    p = dsrlWriteStr(out, p, tag);
    p = dsrlWriteByte(out, p, '>');
    return p;
}

pub fn serializeComment(text: string, out: []u8): usize {
    let p: usize = 0;
    p = dsrlWriteStr(out, p, "<!--");
    p = dsrlWriteStr(out, p, text);
    p = dsrlWriteStr(out, p, "-->");
    return p;
}

pub fn serializeCdata(text: string, out: []u8): usize {
    let p: usize = 0;
    p = dsrlWriteStr(out, p, "<![CDATA[");
    p = dsrlWriteStr(out, p, text);
    p = dsrlWriteStr(out, p, "]]>");
    return p;
}
