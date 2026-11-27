// @cap/cookie-jar — Zero-heap RFC 6265 HTTP Cookie & Set-Cookie parser, serializer, and jar.
// Clean-room MIT implementation; reuses `@cap/unicase` for case-insensitive attributes & domain matching.

import { eqAscii, endsWithIgnoreCase } from "../../unicase/src/index.cp";
import { trim, strEq, startsWith, findByte } from "cap:ascii";
import { writeI64, parseI64 } from "cap:fmt";

fn cjWriteStr(s: string, out: []u8, pos: usize): usize {
    if (pos >= out.len || s.len == 0) { return pos; }
    const avail = out.len - pos;
    const n = if (s.len < avail) s.len else avail;
    @memcpy(out[pos .. pos + n], s[0..n]);
    return pos + n;
}

pub const SAMESITE_NONE_UNSET: u8 = 0;
pub const SAMESITE_LAX: u8 = 1;
pub const SAMESITE_STRICT: u8 = 2;
pub const SAMESITE_NONE: u8 = 3;

pub const CJ_MAX_COOKIES: usize = 32;

pub struct Cookie {
    pub name: string,
    pub value: string,
    pub domain: string,
    pub path: string,
    pub max_age: i64,
    pub has_max_age: bool,
    pub secure: bool,
    pub http_only: bool,
    pub same_site: u8,
}

/// Parses a single `Set-Cookie` header value into a `Cookie` struct.
pub fn parseSetCookie(header: string): Cookie {
    let c = Cookie {
        name: "",
        value: "",
        domain: "",
        path: "/",
        max_age: 0,
        has_max_age: false,
        secure: false,
        http_only: false,
        same_site: SAMESITE_NONE_UNSET,
    };
    let pos: usize = 0;
    let part_idx: usize = 0;
    while (pos <= header.len) {
        let sep = findByte(header, 59, pos) ?? header.len; // ';'
        let seg = trim(header[pos..sep]);
        if (seg.len > 0) {
            let eq = findByte(seg, 61, 0) ?? seg.len; // '='
            if (part_idx == 0) {
                if (eq < seg.len) {
                    c.name = trim(seg[0..eq]);
                    let raw_val = trim(seg[eq + 1..seg.len]);
                    if (raw_val.len >= 2 && raw_val[0] == 34 && raw_val[raw_val.len - 1] == 34) {
                        c.value = raw_val[1..raw_val.len - 1];
                    } else {
                        c.value = raw_val;
                    }
                }
            } else {
                let attr_name = if (eq < seg.len) { trim(seg[0..eq]) } else { seg };
                let attr_val = if (eq < seg.len) { trim(seg[eq + 1..seg.len]) } else { "" };
                if (eqAscii(attr_name, "Domain")) {
                    if (attr_val.len > 0 && attr_val[0] == 46) {
                        c.domain = attr_val[1..attr_val.len];
                    } else {
                        c.domain = attr_val;
                    }
                } else if (eqAscii(attr_name, "Path")) {
                    if (attr_val.len > 0) { c.path = attr_val; }
                } else if (eqAscii(attr_name, "Max-Age")) {
                    if (v := parseI64(trim(attr_val))) {
                        c.max_age = v;
                        c.has_max_age = true;
                    }
                } else if (eqAscii(attr_name, "Secure")) {
                    c.secure = true;
                } else if (eqAscii(attr_name, "HttpOnly")) {
                    c.http_only = true;
                } else if (eqAscii(attr_name, "SameSite")) {
                    if (eqAscii(attr_val, "Strict")) { c.same_site = SAMESITE_STRICT; }
                    else if (eqAscii(attr_val, "Lax")) { c.same_site = SAMESITE_LAX; }
                    else if (eqAscii(attr_val, "None")) { c.same_site = SAMESITE_NONE; }
                }
            }
            part_idx = part_idx + 1;
        }
        if (sep >= header.len) { break; }
        pos = sep + 1;
    }
    return c;
}

/// Serializes a `Cookie` into a `Set-Cookie` header string in `out`, returning bytes written.
pub fn serializeSetCookie(c: Cookie, out: []u8): usize {
    let p: usize = 0;
    p = cjWriteStr(c.name, out, p);
    p = cjWriteStr("=", out, p);
    p = cjWriteStr(c.value, out, p);
    if (c.domain.len > 0) {
        p = cjWriteStr("; Domain=", out, p);
        p = cjWriteStr(c.domain, out, p);
    }
    if (c.path.len > 0) {
        p = cjWriteStr("; Path=", out, p);
        p = cjWriteStr(c.path, out, p);
    }
    if (c.has_max_age) {
        p = cjWriteStr("; Max-Age=", out, p);
        p = writeI64(c.max_age, out, p);
    }
    if (c.secure) {
        p = cjWriteStr("; Secure", out, p);
    }
    if (c.http_only) {
        p = cjWriteStr("; HttpOnly", out, p);
    }
    if (c.same_site == SAMESITE_STRICT) {
        p = cjWriteStr("; SameSite=Strict", out, p);
    } else if (c.same_site == SAMESITE_LAX) {
        p = cjWriteStr("; SameSite=Lax", out, p);
    } else if (c.same_site == SAMESITE_NONE) {
        p = cjWriteStr("; SameSite=None", out, p);
    }
    return p;
}

/// Looks up a cookie value by `name` inside a `Cookie:` request header (`"a=1; b=2"`).
pub fn getCookieHeaderValue(header: string, name: string): string? {
    let pos: usize = 0;
    while (pos <= header.len) {
        let sep = findByte(header, 59, pos) ?? header.len;
        let seg = trim(header[pos..sep]);
        let eq = findByte(seg, 61, 0) ?? seg.len;
        if (eq < seg.len) {
            let k = trim(seg[0..eq]);
            if (strEq(k, name)) {
                let v = trim(seg[eq + 1..seg.len]);
                if (v.len >= 2 && v[0] == 34 && v[v.len - 1] == 34) {
                    return v[1..v.len - 1];
                }
                return v;
            }
        }
        if (sep >= header.len) { break; }
        pos = sep + 1;
    }
    return null;
}

/// RFC 6265 domain matching (case-insensitive via `@cap/unicase`).
pub fn domainMatches(host: string, cookie_domain: string): bool {
    if (cookie_domain.len == 0) { return true; }
    if (eqAscii(host, cookie_domain)) { return true; }
    if (host.len > cookie_domain.len && endsWithIgnoreCase(host, cookie_domain)) {
        let dot_pos = host.len - cookie_domain.len - 1;
        return host[dot_pos] == 46; // '.'
    }
    return false;
}

/// RFC 6265 path matching.
pub fn pathMatches(req_path: string, cookie_path: string): bool {
    if (cookie_path.len == 0) { return true; }
    if (strEq(req_path, cookie_path)) { return true; }
    if (req_path.len > cookie_path.len && startsWith(req_path, cookie_path)) {
        if (cookie_path[cookie_path.len - 1] == 47) { return true; }
        return req_path[cookie_path.len] == 47;
    }
    return false;
}

/// Zero-heap in-memory RFC 6265 CookieJar (up to 32 cookies).
pub struct CookieJar {
    pub items: [32]Cookie,
    pub count: usize,

    pub static fn new(): CookieJar {
        let empty = Cookie {
            name: "", value: "", domain: "", path: "/",
            max_age: 0, has_max_age: false, secure: false, http_only: false,
            same_site: SAMESITE_NONE_UNSET,
        };
        let arr: [32]Cookie = undefined;
        let i: usize = 0;
        while (i < 32) {
            arr[i] = empty;
            i = i + 1;
        }
        return CookieJar { items: arr, count: 0 };
    }

    pub fn size(): usize {
        return _.count;
    }

    pub fn setFromHeader(*_, set_cookie_header: string): bool {
        let c = parseSetCookie(set_cookie_header);
        if (c.name.len == 0) { return false; }
        // Max-Age <= 0 deletes existing cookie
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.items[i].name, c.name) && eqAscii(_.items[i].domain, c.domain) && strEq(_.items[i].path, c.path)) {
                if (c.has_max_age && c.max_age <= 0) {
                    let j = i;
                    while (j + 1 < _.count) {
                        _.items[j] = _.items[j + 1];
                        j = j + 1;
                    }
                    _.count = _.count - 1;
                    return true;
                }
                _.items[i] = c;
                return true;
            }
            i = i + 1;
        }
        if (c.has_max_age && c.max_age <= 0) { return true; }
        if (_.count >= CJ_MAX_COOKIES) { return false; }
        _.items[_.count] = c;
        _.count = _.count + 1;
        return true;
    }

    /// Formats the `Cookie:` request header for `(host, req_path, is_https)` into `out`.
    pub fn buildCookieHeader(host: string, req_path: string, is_https: bool, out: []u8): usize {
        let p: usize = 0;
        let first = true;
        let i: usize = 0;
        while (i < _.count) {
            let c = _.items[i];
            if ((!c.secure || is_https) && domainMatches(host, c.domain) && pathMatches(req_path, c.path)) {
                if (!first) {
                    p = cjWriteStr("; ", out, p);
                }
                p = cjWriteStr(c.name, out, p);
                p = cjWriteStr("=", out, p);
                p = cjWriteStr(c.value, out, p);
                first = false;
            }
            i = i + 1;
        }
        return p;
    }
}
