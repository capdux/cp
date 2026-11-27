// Type definitions for CP

export enum RonTokenKind {
    Ident,
    String,
    Int,
    Float,
    Bool,
    LParen,
    RParen,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    Colon,
    Comma,
    Eof,
    Invalid
}

export class RonToken {
    kind: RonTokenKind,
    text: string,
    start: usize,
    end: usize,
}

export fn ronpIsWs(c: u8): bool;
export fn ronpIsDigit(c: u8): bool;
export fn ronpIsHexDigit(c: u8): bool;
export fn ronpIsIdentStart(c: u8): bool;
export fn ronpIsIdentCont(c: u8): bool;
export fn ronpEq(a: string, b: string): bool;
export fn ronpSkipTrivia(src: string, start_pos: usize): usize;
export class RonLexer {
    src: string,
    pos: usize,
    fn new(src: string): RonLexer;
    fn nextToken(_: auto): RonToken;
}

export fn ronpIsOpenDelim(k: RonTokenKind): bool;
export fn ronpIsCloseDelim(k: RonTokenKind): bool;
export fn ronGetField(ron_src: string, field_name: string): string?;
