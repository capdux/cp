// Type definitions for CP

export enum CssTokKind {
    Ident,
    AtKeyword,
    Hash,
    String,
    Number,
    Percentage,
    Dimension,
    Function,
    Colon,
    Semicolon,
    Comma,
    LBrace,
    RBrace,
    LParen,
    RParen,
    LBracket,
    RBracket,
    Delim,
    Eof
}

export class CssToken {
    kind: CssTokKind,
    text: string,
    unit: string,
    start: usize,
    end: usize,
}

export fn csspIsWs(c: u8): bool;
export fn csspIsDigit(c: u8): bool;
export fn csspIsIdentStart(c: u8): bool;
export fn csspIsIdentChar(c: u8): bool;
export fn csspStartsIdent(s: string, pos: usize): bool;
export fn csspStartsNumber(s: string, pos: usize): bool;
export fn csspScanIdent(s: string, start: usize): usize;
export fn csspScanNumber(s: string, start: usize): usize;
export fn csspSkipWsAndComments(s: string, start: usize): usize;
export class CssLexer {
    src: string,
    pos: usize,
    fn new(src: string): CssLexer;
    fn nextToken(_: auto): CssToken;
}

