// @cap/dom-serializer — Zero-heap HTML/XML DOM node serializer
// Clean-room CP implementation inspired by dom-serializer (MIT).

import { eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

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

fn dsrlWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) return pos;
    out[pos] = b;
    return pos + 1;
}

fn dsrlWriteAttrValue(out: []u8, pos: usize, val: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        const c: u8 = val[i];
        if (c == '&') {
            p = writeStr("&amp;", out, p);
        } else if (c == '"') {
            p = writeStr("&quot;", out, p);
        } else {
            p = dsrlWriteByte(out, p, c);
        }
    }
    return p;
}

pub fn isVoidElement(tag: string): bool {
    if (eqIgnoreCase(tag, "area")) return true;
    if (eqIgnoreCase(tag, "base")) return true;
    if (eqIgnoreCase(tag, "br")) return true;
    if (eqIgnoreCase(tag, "col")) return true;
    if (eqIgnoreCase(tag, "embed")) return true;
    if (eqIgnoreCase(tag, "hr")) return true;
    if (eqIgnoreCase(tag, "img")) return true;
    if (eqIgnoreCase(tag, "input")) return true;
    if (eqIgnoreCase(tag, "link")) return true;
    if (eqIgnoreCase(tag, "meta")) return true;
    if (eqIgnoreCase(tag, "param")) return true;
    if (eqIgnoreCase(tag, "source")) return true;
    if (eqIgnoreCase(tag, "track")) return true;
    if (eqIgnoreCase(tag, "wbr")) return true;
    return false;
}

pub fn serializeText(text: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const c: u8 = text[i];
        if (c == '&') {
            p = writeStr("&amp;", out, p);
        } else if (c == '<') {
            p = writeStr("&lt;", out, p);
        } else if (c == '>') {
            p = writeStr("&gt;", out, p);
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
    p = writeStr(tag, out, p);

    let i: usize = 0;
    while (i < attrs.len) : (i += 1) {
        p = dsrlWriteByte(out, p, ' ');
        p = writeStr(attrs[i].name, out, p);
        p = writeStr("=\"", out, p);
        p = dsrlWriteAttrValue(out, p, attrs[i].value);
        p = dsrlWriteByte(out, p, '"');
    }

    if (self_close) {
        p = writeStr("/>", out, p);
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
    p = writeStr(inner_html, out, p);
    p = writeStr("</", out, p);
    p = writeStr(tag, out, p);
    p = dsrlWriteByte(out, p, '>');
    return p;
}

pub fn serializeComment(text: string, out: []u8): usize {
    let p: usize = 0;
    p = writeStr("<!--", out, p);
    p = writeStr(text, out, p);
    p = writeStr("-->", out, p);
    return p;
}

pub fn serializeCdata(text: string, out: []u8): usize {
    let p: usize = 0;
    p = writeStr("<![CDATA[", out, p);
    p = writeStr(text, out, p);
    p = writeStr("]]>", out, p);
    return p;
}
