// Type definitions for CP

import { Cookie, parseSetCookie, serializeSetCookie, SAMESITE_LAX, SAMESITE_STRICT, SAMESITE_NONE, SAMESITE_NONE_UNSET } from "../../cookie-jar/src/index.cp";
import { componentSet, percentEncode, percentDecode } from "../../percent-encoding/src/index.cp";
export class EncodedCookieMeta {
    name_len: usize,
    value_len: usize,
    domain: string,
    path: string,
    max_age: i64,
    has_max_age: bool,
    secure: bool,
    http_only: bool,
    same_site: u8,
    ok: bool,
}

export fn ckcDefaultCookie(name: string, value: string): Cookie;
export class CookieBuilder {
    cookie: Cookie,
    fn new(name: string, value: string): CookieBuilder;
    fn domain(_: auto, d: string): &mut CookieBuilder;
    fn path(_: auto, p: string): &mut CookieBuilder;
    fn maxAge(_: auto, secs: i64): &mut CookieBuilder;
    fn secure(_: auto, v: bool): &mut CookieBuilder;
    fn httpOnly(_: auto, v: bool): &mut CookieBuilder;
    fn sameSite(_: auto, s: u8): &mut CookieBuilder;
    fn build(): Cookie;
}

export fn parseEncodedCookie(header: string, name_out: []u8, val_out: []u8): EncodedCookieMeta;
export fn formatEncodedCookie(c: Cookie, out: []u8): usize;
export fn makeRemovalCookie(name: string, path: string, out: []u8): usize;
