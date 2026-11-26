// Type definitions for CP

export class MediaParam {
    key: string,
    value: string,
}

export class ContentType {
    ok: bool,
    type_str: string,
    subtype: string,
    suffix: string,
    essence: string,
    params: [MAX_PARAMS]MediaParam,
    param_count: usize,
    fn invalid(): ContentType;
    fn getParam(key: string): string?;
    fn charset(): string?;
    fn boundary(): string?;
}

export fn isSpace(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn isTchar(c: u8): bool;
export fn isValidToken(s: string): bool;
export fn trimSpaces(s: string): string;
export fn parseContentType(header: string): ContentType;
export fn isJsonContentType(header: string): bool;
export fn isTextContentType(header: string): bool;
export fn isMultipartContentType(header: string): bool;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushLower(out: []u8, pos: usize, s: string): usize;
export fn pushRaw(out: []u8, pos: usize, s: string): usize;
export fn formatContentType(ct: ContentType, out: []u8): string;
