// Type definitions for CP

export class DocPragma {
    key: string,
    value: string,
}

export class ParsedDocblock {
    comments_len: usize,
    pragma_count: usize,
}

export fn jdblIsWs(c: u8): bool;
export fn jdblIsLineWs(c: u8): bool;
export fn jdblStrEq(a: string, b: string): bool;
export fn jdblTrimLine(s: string): string;
export fn jdblWriteStr(src: string, out: []u8, pos: usize): usize;
export fn jdblStripDelimiters(docblock: string): string;
export fn extractDocblock(code: string): string;
export fn extract(code: string): string;
export fn strip(code: string, out: []u8): usize;
export fn stripSlice(code: string): string;
export fn parseDocblock(docblock: string, out: []DocPragma): usize;
export fn parse(docblock: string, out: []DocPragma): usize;
export fn parseWithComments(docblock: string, comments_out: []u8, pragmas_out: []DocPragma): ParsedDocblock;
export fn printDocblock(pragmas: &[]DocPragma, out: []u8): usize;
export fn print(pragmas: &[]DocPragma, out: []u8): usize;
export fn printWithComments(comments: string, pragmas: &[]DocPragma, out: []u8): usize;
export fn getPragma(pragmas: &[]DocPragma, key: string): string?;
export fn hasPragma(pragmas: &[]DocPragma, key: string): bool;
