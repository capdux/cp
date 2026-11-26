// Type definitions for CP

export class CookieEntry {
    name: string,
    value: string,
    domain: string,
    path: string,
    max_age: i64,
    has_max_age: bool,
    secure: bool,
    http_only: bool,
    same_site: u8,
}

export fn cjToLower(c: u8): u8;
export fn cjIsSpace(c: u8): bool;
export fn cjTrim(s: &[]u8): &[]u8;
export fn cjEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn cjIsIpv4Literal(host: &[]u8): bool;
export fn cjParseI64(s: &[]u8, out: &mut i64): bool;
export fn domainMatches(host: string, cookie_domain: string): bool;
export fn pathMatches(req_path: string, cookie_path: string): bool;
export fn defaultPath(req_uri_path: string): string;
export fn parseSetCookie(header: string, req_host: string, req_path: string, out: &mut CookieEntry): bool;
export fn shouldSend(cookie: CookieEntry, host: string, path: string, is_https: bool): bool;
export fn buildCookieHeader(cookies: &[]CookieEntry, host: string, path: string, is_https: bool, out: []u8): usize;
