// Type definitions for CP

export enum CharPred {
    Alpha = 0,
    Digit = 1,
    AlphaNum = 2,
    Space = 3,
    HexDigit = 4,
    Ident = 5,
    NonSpace = 6
}

export class CursorMark {
    pos: usize,
    line: usize,
    col: usize,
}

export class ParseCursor {
    input: string,
    pos: usize,
    line: usize,
    col: usize,
    fn new(input: string): ParseCursor;
    fn remaining(): string;
    fn isEof(): bool;
    fn peek(): u8?;
    fn advance(_: auto): u8?;
    fn save(): CursorMark;
    fn restore(_: auto, mark: CursorMark);
}

export fn cmbIsAlpha(ch: u8): bool;
export fn cmbIsDigit(ch: u8): bool;
export fn cmbIsSpace(ch: u8): bool;
export fn cmbIsHex(ch: u8): bool;
export fn cmbMatchPred(ch: u8, kind: CharPred): bool;
export fn cmbStartsWithAt(hay: string, pos: usize, needle: string): bool;
export fn tag(cursor: &mut ParseCursor, lit: string): bool;
export fn charByte(cursor: &mut ParseCursor, expected: u8): bool;
export fn takeWhile(cursor: &mut ParseCursor, pred_kind: CharPred): string;
export fn takeWhile1(cursor: &mut ParseCursor, pred_kind: CharPred): string?;
export fn takeUntil(cursor: &mut ParseCursor, delim: string): string?;
export fn skipSpaces(cursor: &mut ParseCursor): usize;
export fn parseU64(cursor: &mut ParseCursor): u64?;
export fn parseI64(cursor: &mut ParseCursor): i64?;
export fn parseQuotedString(cursor: &mut ParseCursor): string?;
export fn delimited(cursor: &mut ParseCursor, open: string, close: string): string?;
export fn separatedList(cursor: &mut ParseCursor, sep: string, out: []string): usize;
