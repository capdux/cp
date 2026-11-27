// Type definitions for CP

export enum JsTokenKind {
    IdentifierName,
    NumericLiteral,
    StringLiteral,
    NoSubstitutionTemplate,
    TemplateHead,
    Punctuator,
    SingleLineComment,
    MultiLineComment,
    WhiteSpace,
    LineTerminatorSequence,
    Invalid
}

export class JsToken {
    kind: JsTokenKind,
    start: usize,
    end: usize,
    closed: bool,
    fn text(src: string): string;
}

export fn jstIsIdentStart(b: u8): bool;
export fn jstIsIdentPart(b: u8): bool;
export fn jstIsDigit(b: u8): bool;
export fn jstIsHex(b: u8): bool;
export fn jstIsSpace(b: u8): bool;
export fn jstStartsWith(src: string, pos: usize, lit: string): bool;
export fn nextJsToken(src: string, pos: usize): JsToken?;
export fn tokenizeJs(src: string, out: []JsToken): usize;
export fn countJsTokens(src: string): usize;
