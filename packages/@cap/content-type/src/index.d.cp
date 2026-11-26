// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
export fn ctypIsOws(c: u8): bool;
export fn ctypIsTokenChar(c: u8): bool;
export fn ctypIsValidToken(s: string): bool;
export fn ctypIsQuotedAllowed(c: u8): bool;
export fn ctypWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn ctypWriteSlice(out: []u8, pos: usize, s: string): usize;
export class ContentTypeParam {
    name: string,
    value: string,
    fn new(name: string, value: string): ContentTypeParam;
}

export class ContentType {
    valid: bool,
    media_type: string,
    params: [8]ContentTypeParam,
    param_count: usize,
    fn invalid(): ContentType;
    fn getParam(name: string): string?;
    fn getParamUnescaped(name: string, out: []u8): usize;
    fn charset(): string?;
    fn boundary(): string?;
}

export fn isValidMediaType(s: string): bool;
export fn unescapeQuotedString(raw: string, out: []u8): usize;
export fn parseContentType(header: string): ContentType;
export fn formatContentType(media_type: string, params: &[]ContentTypeParam, out: []u8): usize;
