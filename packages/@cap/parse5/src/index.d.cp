// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
export class HtmlToken {
    kind: u8,
    name: string,
    raw_attrs: string,
    text: string,
    start: usize,
    end: usize,
}

export class HtmlAttr {
    name: string,
    value: string,
}

export fn p5IsWs(b: u8): bool;
export fn p5IsTagNameChar(b: u8): bool;
export fn p5Trim(s: string): string;
export fn p5FindSubstr(s: string, start_pos: usize, needle: string): usize;
export fn isVoidElement(tag: string): bool;
export class HtmlTokenizer {
    src: string,
    pos: usize,
    fn new(src: string): HtmlTokenizer;
    fn next(_: auto): HtmlToken;
}

export fn parseAttributes(raw_attrs: string, out_attrs: []HtmlAttr): usize;
export fn extractTextContent(html: string, out: []u8): usize;
