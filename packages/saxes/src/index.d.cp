// Type definitions for CP

export enum XmlEventKind {
    OpenTag,
    CloseTag,
    SelfClosingTag,
    Text,
    Cdata,
    Comment,
    ProcessingInstruction,
    Error
}

export class XmlAttr {
    name: string,
    value: string,
}

export class XmlEvent {
    kind: XmlEventKind,
    name: string,
    text: string,
    attrs: [8]XmlAttr,
    attr_count: usize,
}

export fn saxEmptyAttrs(): [8]XmlAttr;
export fn saxMakeEvent(kind: XmlEventKind, name: string, text: string): XmlEvent;
export fn saxIsWs(ch: u8): bool;
export fn saxIsNameStart(ch: u8): bool;
export fn saxIsNameChar(ch: u8): bool;
export fn saxStrEq(a: string, b: string): bool;
export fn saxStartsWithAt(s: string, pos: usize, prefix: string): bool;
export fn saxFindSubstrFrom(s: string, start: usize, needle: string): i64;
export fn saxAllWs(s: string): bool;
export fn saxEmit(out: []XmlEvent, count: usize, ev: XmlEvent): usize;
export fn saxEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;
export fn saxHexVal(ch: u8): i32;
export fn saxDecodeEntitiesImpl(raw: string, out: []u8): usize;
export class SaxesParser {
    stack: [32]string,
    depth: usize,
    has_err: bool,
    root_count: usize,
    fn new(): SaxesParser;
    fn parseAll(_: auto, xml: string, out: []XmlEvent): usize;
    fn isWellFormed(_: auto, xml: string): bool;
    fn decodeEntities(raw: string, out: []u8): usize;
}

