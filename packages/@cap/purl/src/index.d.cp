// Type definitions for CP

export class Purl {
    pkg_type: string,
    namespace: string,
    name: string,
    version: string,
    qualifiers: string,
    subpath: string,
    valid: bool,
}

export fn invalidPurl(): Purl;
export fn isSpace(b: u8): bool;
export fn isAlpha(b: u8): bool;
export fn isDigit(b: u8): bool;
export fn toLowerAscii(b: u8): u8;
export fn isValidType(pkg_type: string): bool;
export fn trimSlashes(s: string): string;
export fn parsePurl(input: string): Purl;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn getQualifier(purl: Purl, key: string): string;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;
export fn formatPurl(p: Purl, out: []u8): usize;
