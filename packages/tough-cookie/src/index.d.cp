// Type definitions for CP

import { eqAscii, endsWithIgnoreCase } from "../../unicase/src/index.cp";
export class Cookie {
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

export fn cjIsWs(b: u8): bool;
export fn cjTrim(s: string): string;
export fn cjStrEq(a: string, b: string): bool;
export fn cjPush(out: []u8, pos: usize, s: string): usize;
export fn cjPushI64(out: []u8, pos: usize, v: i64): usize;
export fn cjParseI64(s: string, ok_out: []bool): i64;
export fn parseSetCookie(header: string): Cookie;
export fn serializeSetCookie(c: Cookie, out: []u8): usize;
export fn getCookieHeaderValue(header: string, name: string): string?;
export fn domainMatches(host: string, cookie_domain: string): bool;
export fn pathMatches(req_path: string, cookie_path: string): bool;
export class CookieJar {
    items: [32]Cookie,
    count: usize,
    fn new(): CookieJar;
    fn size(): usize;
    fn setFromHeader(_: auto, set_cookie_header: string): bool;
    fn buildCookieHeader(host: string, req_path: string, is_https: bool, out: []u8): usize;
}

