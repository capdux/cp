// Type definitions for CP

import { strEq, isHexDigit, isAlnum, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class DidUrl {
    did: string,
    method: string,
    id: string,
    path: string,
    query: string,
    fragment: string,
    valid: bool,
}

export fn isValidMethod(method: string): bool;
export fn isValidMethodId(id: string): bool;
export fn parseDid(input: string): DidUrl;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn formatDid(method: string, id: string, fragment: string, out: []u8): usize;
