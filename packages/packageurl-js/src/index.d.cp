// Type definitions for CP

import { isAlpha, isDigit, toLowerByte, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
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
export fn purlIsSpace(b: u8): bool;
export fn isValidType(pkg_type: string): bool;
export fn trimSlashes(s: string): string;
export fn parsePurl(input: string): Purl;
export fn getQualifier(purl: Purl, key: string): string;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;
export fn formatPurl(p: Purl, out: []u8): usize;
