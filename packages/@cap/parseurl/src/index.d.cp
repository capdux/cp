// Type definitions for CP

export class ParsedUrl {
    path_start: usize,
    path_len: usize,
    pathname_start: usize,
    pathname_len: usize,
    search_start: usize,
    search_len: usize,
    query_start: usize,
    query_len: usize,
    has_query: bool,
    has_scheme: bool,
    valid: bool,
}

export fn invalidUrl(): ParsedUrl;
export fn isAsciiAlpha(ch: u8): bool;
export fn isSchemeChar(ch: u8): bool;
export fn hasControlChar(s: string): bool;
export fn detectSchemeEnd(url: string): usize;
export fn parseUrl(url: string): ParsedUrl;
export fn hasDotDotSegment(pathname: string): bool;
export fn stripQueryAndHash(url: string): string;
