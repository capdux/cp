// Type definitions for CP

import { isAlnum, toLowerByte, eqIgnoreCase, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class Urn {
    nid: string,
    nss: string,
    r_component: string,
    q_component: string,
    f_component: string,
    valid: bool,
}

export fn invalidUrn(): Urn;
export fn urnIsSpace(b: u8): bool;
export fn isValidNid(nid: string): bool;
export fn parseUrn(input: string): Urn;
export fn urnEquivalent(a: Urn, b: Urn): bool;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;
export fn formatUrn(u: Urn, out: []u8): usize;
