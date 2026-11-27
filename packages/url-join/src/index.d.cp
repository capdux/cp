// Type definitions for CP

import { strEq, eqIgnoreCase, isAlpha, isAlnum, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn isSchemeChar(c: u8): bool;
export fn schemePrefixLen(s: string): usize;
export class PartSplit {
    path: string,
    query: string,
    hash: string,
}

export fn splitPart(p: string): PartSplit;
export fn trimLeadingSlashes(s: string): string;
export fn trimTrailingSlashes(s: string): string;
export fn urlJoin(parts: []string, out: []u8): string;
export fn normalizeUrlPath(path: string, out: []u8): string;
export fn withQuery(base_url: string, key: string, value: string, out: []u8): string;
export fn findQueryOrHash(url: string): usize;
export fn withTrailingSlash(url: string, out: []u8): string;
export fn withoutTrailingSlash(url: string, out: []u8): string;
