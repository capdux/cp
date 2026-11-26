// Type definitions for CP

export enum DomNodeKind {
    Element,
    Text,
    Comment,
    Cdata,
    Doctype
}

export class DomAttr {
    name: string,
    value: string,
}

export class SerializeOptions {
    xml_mode: bool,
    self_closing_tags: bool,
}

export fn dsrlToLowerByte(c: u8): u8;
export fn dsrlEqIgnoreCase(a: string, b: string): bool;
export fn dsrlWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn dsrlWriteStr(out: []u8, pos: usize, s: string): usize;
export fn dsrlWriteAttrValue(out: []u8, pos: usize, val: string): usize;
export fn isVoidElement(tag: string): bool;
export fn serializeText(text: string, out: []u8): usize;
export fn serializeOpenTag(tag: string, attrs: &[]DomAttr, self_close: bool, out: []u8): usize;
export fn serializeElement(tag: string, attrs: &[]DomAttr, inner_html: string, opts: SerializeOptions, out: []u8): usize;
export fn serializeComment(text: string, out: []u8): usize;
export fn serializeCdata(text: string, out: []u8): usize;
