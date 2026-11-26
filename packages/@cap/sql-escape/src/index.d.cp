// Type definitions for CP

export fn escapeQuoted(input: string, quote: u8, allow_empty: bool, out: []u8): usize;
export fn escapeStringAnsi(input: string, out: []u8): usize;
export fn escapeIdentifierAnsi(ident: string, out: []u8): usize;
export fn escapeIdentifierMysql(ident: string, out: []u8): usize;
export fn escapeLike(pattern: string, escape_char: u8, out: []u8): usize;
export fn isAlphaOrUnder(b: u8): bool;
export fn isIdentRest(b: u8): bool;
export fn isSafeUnquotedIdent(ident: string): bool;
export fn countPlaceholders(sql: string): usize;
