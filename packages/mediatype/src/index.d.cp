// Type definitions for CP

import { trim, toLowerByte, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class MediaType {
    top: string,
    subtype: string,
    suffix: string,
    params: string,
}

export fn mtToLower(c: u8): u8;
export fn mtIsOws(c: u8): bool;
export fn mtTrimOws(s: &[]u8): &[]u8;
export fn mtEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn mtIsTchar(c: u8): bool;
export fn isValidToken(s: string): bool;
export fn parseMediaType(input: string, out: &mut MediaType): bool;
export fn getParam(mt: MediaType, name: string, out: []u8): usize;
export fn formatMediaType(mt: MediaType, out: []u8): usize;
export fn isJsonLike(mt: MediaType): bool;
export fn isTextLike(mt: MediaType): bool;
export class MediaParam {
    name: string,
    value: string,
}

export class ParsedMediaType {
    valid: bool,
    top_level: string,
    sub_level: string,
    suffix: string,
    params: [8]MediaParam,
    param_count: usize,
    fn invalid(): ParsedMediaType;
    fn parse(raw: string): ParsedMediaType;
    fn getParam(key: string): string?;
    fn hasSuffix(suf: string): bool;
    fn formatEssence(out: []u8): usize;
    fn formatFull(out: []u8): usize;
    fn isJson(): bool;
    fn isText(): bool;
}

export fn essenceEq(a: string, b: string): bool;
