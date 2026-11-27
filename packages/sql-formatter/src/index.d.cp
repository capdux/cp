// Type definitions for CP

export class Token {
    kind: u8,
    start: usize,
    end: usize,
}

export class SqlFormatOptions {
    indent_width: u8,
    uppercase_keywords: bool,
    lines_between_queries: u8,
    fn defaults(): SqlFormatOptions;
}

export fn isSpace(c: u8): bool;
export fn isAlpha(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn isWordCont(c: u8): bool;
export fn toUpperAscii(c: u8): u8;
export fn eqIgnoreCase(sql: string, start: usize, end: usize, upper: string): bool;
export fn nextToken(sql: string, start_pos: usize): Token;
export fn isSqlFunctionKeyword(sql: string, s: usize, e: usize): bool;
export fn isSqlKeyword(sql: string, s: usize, e: usize): bool;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushSlice(out: []u8, pos: usize, src: string, start: usize, end: usize, upper: bool): usize;
export fn pushIndent(out: []u8, pos: usize, level: usize, indent_width: u8): usize;
export fn pushNewlineIndent(out: []u8, pos: usize, level: usize, indent_width: u8): usize;
export fn isJoinLeadWord(sql: string, s: usize, e: usize): bool;
export fn isTopClauseWord(sql: string, s: usize, e: usize): bool;
export fn formatSql(sql: string, out: []u8): string;
export fn formatSqlWith(sql: string, opts: SqlFormatOptions, out: []u8): string;
export fn isTightDelimiter(kind: u8): bool;
export fn minifySql(sql: string, out: []u8): string;
