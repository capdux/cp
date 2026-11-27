// Type definitions for CP

export class DocPragma {
    key: string,
    value: string,
}

export fn jdblIsWs(c: u8): bool;
export fn jdblIsLineWs(c: u8): bool;
export fn jdblTrimLine(s: string): string;
export fn jdblWriteStr(src: string, out: []u8, pos: usize): usize;
export fn extractDocblock(code: string): string;
export fn parseDocblock(docblock: string, out: []DocPragma): usize;
export fn printDocblock(pragmas: &[]DocPragma, out: []u8): usize;
