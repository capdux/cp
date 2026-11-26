// Type definitions for CP

export class PathSegment {
    is_index: bool,
    key: string,
    index: usize,
}

export class ParsedDotPath {
    segs: [MAX_SEGMENTS]PathSegment,
    count: usize,
    valid: bool,
    fn get(idx: usize): PathSegment?;
}

export fn emptySegment(): PathSegment;
export fn invalidPath(): ParsedDotPath;
export fn isSpace(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn parseUsize(s: string): usize?;
export fn parseDotPath(path: string, key_buf: []u8): ParsedDotPath;
export fn skipSpaces(s: string, pos: usize): usize;
export fn skipJsonString(s: string, start: usize): usize?;
export fn skipJsonValue(s: string, start: usize): usize?;
export fn jsonStringEquals(rawQuoted: string, target: string): bool;
export fn lookupObjectKey(obj: string, key: string): string?;
export fn lookupArrayIndex(arr: string, targetIdx: usize): string?;
export fn getJsonPath(json: string, path: string): string?;
export fn getJsonString(json: string, path: string): string?;
export fn hasJsonPath(json: string, path: string): bool;
export fn escapePathSegment(seg: string, out: []u8): string;
