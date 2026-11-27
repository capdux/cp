// Type definitions for CP

import { isAlpha, isDigit, isHexDigit, toLowerByte, toUpperByte, eqIgnoreCase, startsWith, findByte, strEq } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export class UriComponents {
    valid: bool,
    scheme: string,
    userinfo: string,
    host: string,
    port: i32,
    path: string,
    query: string,
    has_query: bool,
    fragment: string,
    has_fragment: bool,
    fn invalid(): UriComponents;
    fn empty(): UriComponents;
}

export fn furiIsSchemeChar(b: u8): bool;
export fn furiPush(out: []u8, pos: usize, b: u8): usize;
export fn furiPushSlice(out: []u8, pos: usize, s: string): usize;
export fn furiPushLower(out: []u8, pos: usize, s: string): usize;
export fn furiPushPercentUpper(out: []u8, pos: usize, s: string): usize;
export fn furiPushPort(out: []u8, pos: usize, port: i32): usize;
export fn furiDefaultPort(scheme: string): i32;
export fn furiHasColon(s: string): bool;
export fn furiValidateIpv6Inner(s: string): bool;
export fn furiParsePortStr(s: string): i32;
export fn parseUri(input: string): UriComponents;
export fn serializeUri(c: UriComponents, out: []u8): usize;
export fn furiPopSegment(out: []u8, out_len: usize): usize;
export fn furiRemoveDotSegments(input: string, out: []u8): usize;
export fn resolveUri(base: string, rel: string, out: []u8): usize;
export fn furiNormalizeForCompare(input: string, out: []u8): usize;
export fn equalUri(a: string, b: string): bool;
