// Type definitions for CP

export class ParsedCookie {
    name: string,
    value: string,
    path: string,
    domain: string,
    expires: string,
    same_site: string,
    max_age: i64,
    has_max_age: bool,
    secure: bool,
    http_only: bool,
    partitioned: bool,
}

export fn scpIsSpace(b: u8): bool;
export fn scpTrim(s: string): string;
export fn scpToLower(b: u8): u8;
export fn scpEqIgnoreCase(a: string, b: string): bool;
export fn scpFindByte(s: string, ch: u8, start_pos: usize): usize?;
export fn scpParseI64(s: string): i64?;
export fn scpStripQuotes(s: string): string;
export fn parseSetCookie(header: string): ParsedCookie;
export fn splitCookiesCount(combined: string): usize;
export fn splitCookiesGet(combined: string, target_idx: usize): string?;
