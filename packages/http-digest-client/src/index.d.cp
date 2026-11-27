// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class DigestChallenge {
    realm: string,
    nonce: string,
    opaque_val: string,
    qop: string,
    algorithm: string,
    stale: bool,
    valid: bool,
}

export class DigestResponse {
    username: string,
    realm: string,
    nonce: string,
    uri: string,
    qop: string,
    nc: string,
    cnonce: string,
    response: string,
    opaque_val: string,
    valid: bool,
}

export fn digIsSpace(b: u8): bool;
export fn trimSlice(s: string): string;
export fn stripDigestPrefix(header: string): string;
export fn unquoteVal(v: string): string;
export fn parseChallenge(header: string): DigestChallenge;
export fn parseAuthorization(header: string): DigestResponse;
export fn supportsQop(qop_list: string, target: string): bool;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushQuotedField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;
export fn pushTokenField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;
export fn formatAuthorization(resp: DigestResponse, out: []u8): usize;
export fn formatChallenge(ch: DigestChallenge, out: []u8): usize;
