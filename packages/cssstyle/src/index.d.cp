// Type definitions for CP

import { isSpace, eqIgnoreCase, trim, toLower } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class CssProp {
    name: string,
    value: string,
    important: bool,
}

export fn cstSkipSpaceAndComments(s: string, pos: usize): usize;
export fn cstStripTrailingComments(s: string): string;
export class CstParsedValue {
    value: string,
    important: bool,
}

export fn cstExtractImportant(raw: string): CstParsedValue;
export class CssStyleDecl {
    props: [64]CssProp,
    count: usize,
    fn new(): CssStyleDecl;
    fn len(): usize;
    fn parse(_: auto, css_text: string);
    fn getPropertyValue(name: string): string;
    fn getPropertyPriority(name: string): string;
    fn setProperty(_: auto, name: string, value: string, important: bool): bool;
    fn removeProperty(_: auto, name: string): bool;
    fn toCssText(out: []u8): usize;
}

