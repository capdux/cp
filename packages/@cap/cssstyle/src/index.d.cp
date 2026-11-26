// Type definitions for CP

export class CssProp {
    name: string,
    value: string,
    important: bool,
}

export fn cstToLower(c: u8): u8;
export fn cstIsSpace(c: u8): bool;
export fn cstEqIgnoreCase(a: string, b: string): bool;
export fn cstTrim(s: string): string;
export fn cstSkipSpaceAndComments(s: string, pos: usize): usize;
export fn cstStripTrailingComments(s: string): string;
export class CstParsedValue {
    value: string,
    important: bool,
}

export fn cstExtractImportant(raw: string): CstParsedValue;
export fn cstWriteBytes(out: []u8, pos: usize, src: string, lowercase: bool): usize;
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

