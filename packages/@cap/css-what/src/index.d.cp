// Type definitions for CP

export enum CssTokenKind {
    Tag,
    Universal,
    Id,
    Class,
    Attribute,
    Pseudo,
    PseudoElement,
    Descendant,
    Child,
    Adjacent,
    Sibling,
    Comma
}

export enum AttrAction {
    Exists,
    Equals,
    Start,
    End,
    Any,
    Not,
    Element,
    Hyphen
}

export class CssSelectorToken {
    kind: CssTokenKind,
    name: string,
    value: string,
    action: AttrAction,
    ignore_case: bool,
}

export class CwhWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn writeByte(_: auto, b: u8);
    fn writeStr(_: auto, s: string);
}

export fn cwhIsSpace(c: u8): bool;
export fn cwhIsIdentStart(c: u8): bool;
export fn cwhIsIdentChar(c: u8): bool;
export fn cwhSkipSpace(s: string, start: usize): usize;
export fn cwhScanIdent(s: string, start: usize): usize;
export fn cwhIsCombinatorChar(c: u8): bool;
export fn cwhToken(kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool): CssSelectorToken;
export fn parseCssSelector(selector: string, out: []CssSelectorToken): i64;
export fn stringifyCssSelector(tokens: &[]CssSelectorToken, out: []u8): usize;
