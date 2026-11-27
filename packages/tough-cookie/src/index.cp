// @cap/cookie-jar — Zero-heap RFC 6265 HTTP Cookie & Set-Cookie parser, serializer, and jar.
// Clean-room MIT implementation; reuses `@cap/unicase` for case-insensitive attributes & domain matching.

import { eqAscii, endsWithIgnoreCase } from "../../unicase/src/index.cp";

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

fn cjIsWs(b: u8): bool {
    return b == 32 || b == 9 || b == 13 || b == 10;
}

fn cjTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && cjIsWs(s[start])) { start = start + 1; }
    let end: usize = s.len;
    while (end > start && cjIsWs(s[end - 1])) { end = end - 1; }
    return s[start..end];
}

fn cjStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn cjPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn cjPushI64(out: []u8, pos: usize, v: i64): usize {
    let p = pos;
    let neg = v < 0;
    let abs_v: u64 = (if (neg) { -v } else { v }) as u64;
    if (neg && p < out.len) {
        out[p] = 45; // '-'
        p = p + 1;
    }
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    if (abs_v == 0) {
        tmp[0] = 48;
        len = 1;
    } else {
        let cur = abs_v;
        while (cur > 0 && len < 24) {
            tmp[len] = (48 + (cur % 10)) as u8;
            cur = cur / 10;
            len = len + 1;
        }
    }
    let k = len;
    while (k > 0 && p < out.len) {
        k = k - 1;
        out[p] = tmp[k];
        p = p + 1;
    }
    return p;
}

fn cjParseI64(s: string, ok_out: []bool): i64 {
    let t = cjTrim(s);
    if (t.len == 0) { ok_out[0] = false; return 0; }
    let i: usize = 0;
    let neg = false;
    if (t[0] == 45) { neg = true; i = 1; }
    else if (t[0] == 43) { i = 1; }
    if (i >= t.len) { ok_out[0] = false; return 0; }
    let acc: i64 = 0;
    while (i < t.len) {
        let b = t[i];
        if (b < 48 || b > 57) { ok_out[0] = false; return 0; }
        acc = acc * 10 + ((b - 48) as i64);
        i = i + 1;
    }
    ok_out[0] = true;
    return if (neg) { -acc } else { acc };
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
        let sep = pos;
        while (sep < header.len && header[sep] != 59) { // ';'
            sep = sep + 1;
        }
        let seg = cjTrim(header[pos..sep]);
        if (seg.len > 0) {
            let eq: usize = 0;
            while (eq < seg.len && seg[eq] != 61) { eq = eq + 1; }
            if (part_idx == 0) {
                if (eq < seg.len) {
                    c.name = cjTrim(seg[0..eq]);
                    let raw_val = cjTrim(seg[eq + 1..seg.len]);
                    if (raw_val.len >= 2 && raw_val[0] == 34 && raw_val[raw_val.len - 1] == 34) {
                        c.value = raw_val[1..raw_val.len - 1];
                    } else {
                        c.value = raw_val;
                    }
                }
            } else {
                let attr_name = if (eq < seg.len) { cjTrim(seg[0..eq]) } else { seg };
                let attr_val = if (eq < seg.len) { cjTrim(seg[eq + 1..seg.len]) } else { "" };
                if (eqAscii(attr_name, "Domain")) {
                    if (attr_val.len > 0 && attr_val[0] == 46) {
                        c.domain = attr_val[1..attr_val.len];
                    } else {
                        c.domain = attr_val;
                    }
                } else if (eqAscii(attr_name, "Path")) {
                    if (attr_val.len > 0) { c.path = attr_val; }
                } else if (eqAscii(attr_name, "Max-Age")) {
                    let ok: [1]bool = [false];
                    let v = cjParseI64(attr_val, ok[0..1]);
                    if (ok[0]) {
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
    p = cjPush(out, p, c.name);
    p = cjPush(out, p, "=");
    p = cjPush(out, p, c.value);
    if (c.domain.len > 0) {
        p = cjPush(out, p, "; Domain=");
        p = cjPush(out, p, c.domain);
    }
    if (c.path.len > 0) {
        p = cjPush(out, p, "; Path=");
        p = cjPush(out, p, c.path);
    }
    if (c.has_max_age) {
        p = cjPush(out, p, "; Max-Age=");
        p = cjPushI64(out, p, c.max_age);
    }
    if (c.secure) {
        p = cjPush(out, p, "; Secure");
    }
    if (c.http_only) {
        p = cjPush(out, p, "; HttpOnly");
    }
    if (c.same_site == SAMESITE_STRICT) {
        p = cjPush(out, p, "; SameSite=Strict");
    } else if (c.same_site == SAMESITE_LAX) {
        p = cjPush(out, p, "; SameSite=Lax");
    } else if (c.same_site == SAMESITE_NONE) {
        p = cjPush(out, p, "; SameSite=None");
    }
    return p;
}

/// Looks up a cookie value by `name` inside a `Cookie:` request header (`"a=1; b=2"`).
pub fn getCookieHeaderValue(header: string, name: string): string? {
    let pos: usize = 0;
    while (pos <= header.len) {
        let sep = pos;
        while (sep < header.len && header[sep] != 59) { sep = sep + 1; }
        let seg = cjTrim(header[pos..sep]);
        let eq: usize = 0;
        while (eq < seg.len && seg[eq] != 61) { eq = eq + 1; }
        if (eq < seg.len) {
            let k = cjTrim(seg[0..eq]);
            if (cjStrEq(k, name)) {
                let v = cjTrim(seg[eq + 1..seg.len]);
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
    if (cjStrEq(req_path, cookie_path)) { return true; }
    if (req_path.len > cookie_path.len) {
        let i: usize = 0;
        while (i < cookie_path.len) {
            if (req_path[i] != cookie_path[i]) { return false; }
            i = i + 1;
        }
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
            if (cjStrEq(_.items[i].name, c.name) && eqAscii(_.items[i].domain, c.domain) && cjStrEq(_.items[i].path, c.path)) {
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
                    p = cjPush(out, p, "; ");
                }
                p = cjPush(out, p, c.name);
                p = cjPush(out, p, "=");
                p = cjPush(out, p, c.value);
                first = false;
            }
            i = i + 1;
        }
        return p;
    }
}
