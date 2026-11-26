// packages/@cap/cookie-jar/src/index.cp
// Zero-allocation RFC 6265 HTTP Cookie domain/path matcher & Set-Cookie engine.
// MIT clean-room pure CP implementation.

/// Zero-copy parsed HTTP Cookie entry with RFC 6265 attributes.
/// `same_site`: 0 = None/unset, 1 = Lax, 2 = Strict.
pub struct CookieEntry {
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

fn cjToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn cjIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8);
}

fn cjTrim(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && cjIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && cjIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn cjEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (cjToLower(a[i]) != cjToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn cjIsIpv4Literal(host: []const u8): bool {
    if (host.len == 0) {
        return false;
    }
    let dots: usize = 0;
    let i: usize = 0;
    while (i < host.len) {
        const c = host[i];
        if (c == ('.' as u8)) {
            dots += 1;
        } else if (c < ('0' as u8) || c > ('9' as u8)) {
            return false;
        }
        i += 1;
    }
    return dots == 3;
}

fn cjParseI64(s: []const u8, out: *i64): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == ('-' as u8)) {
        neg = true;
        i = 1;
    } else if (s[0] == ('+' as u8)) {
        i = 1;
    }
    if (i >= s.len) {
        return false;
    }
    let acc: i64 = 0 as i64;
    while (i < s.len) {
        const c = s[i];
        if (c < ('0' as u8) || c > ('9' as u8)) {
            return false;
        }
        if (acc <= (900000000000000000 as i64)) {
            acc = acc * (10 as i64) + ((c - ('0' as u8)) as i64);
        }
        i += 1;
    }
    if (neg) {
        out.* = -acc;
    } else {
        out.* = acc;
    }
    return true;
}

/// RFC 6265 Section 5.1.3 domain matching:
/// - Case-insensitive exact match or dot-bounded suffix match.
/// - Automatically ignores a leading '.' on `cookie_domain`.
/// - Prevents spoofed suffixes such as `evil-example.com` matching `example.com`.
pub fn domainMatches(host: string, cookie_domain: string): bool {
    if (host.len == 0 || cookie_domain.len == 0) {
        return false;
    }
    let dom = cookie_domain;
    if (dom[0] == ('.' as u8)) {
        dom = dom[1..dom.len];
    }
    if (dom.len == 0) {
        return false;
    }

    if (cjEqIgnoreCase(host, dom)) {
        return true;
    }

    if (cjIsIpv4Literal(host)) {
        return false;
    }

    if (host.len <= dom.len) {
        return false;
    }
    const prefixLen = host.len - dom.len;
    if (host[prefixLen - 1] != ('.' as u8)) {
        return false;
    }
    return cjEqIgnoreCase(host[prefixLen..host.len], dom);
}

/// RFC 6265 Section 5.1.4 path matching:
/// matches when `req_path == cookie_path`, or `req_path` starts with `cookie_path`
/// and either `cookie_path` ends with `'/'` or the next character in `req_path` is `'/'`.
pub fn pathMatches(req_path: string, cookie_path: string): bool {
    if (cookie_path.len == 0 || req_path.len < cookie_path.len) {
        return false;
    }
    let i: usize = 0;
    while (i < cookie_path.len) {
        if (req_path[i] != cookie_path[i]) {
            return false;
        }
        i += 1;
    }
    if (req_path.len == cookie_path.len) {
        return true;
    }
    if (cookie_path[cookie_path.len - 1] == ('/' as u8)) {
        return true;
    }
    return req_path[cookie_path.len] == ('/' as u8);
}

/// RFC 6265 Section 5.1.4 default-path calculation.
/// Examples: `"/a/b/c"` -> `"/a/b"`, `"/a"` -> `"/"`, `""` -> `"/"`.
pub fn defaultPath(req_uri_path: string): string {
    if (req_uri_path.len == 0 || req_uri_path[0] != ('/' as u8)) {
        return "/";
    }
    let lastSlash: usize = 0;
    let i: usize = 0;
    while (i < req_uri_path.len) {
        if (req_uri_path[i] == ('/' as u8)) {
            lastSlash = i;
        }
        i += 1;
    }
    if (lastSlash == 0) {
        return "/";
    }
    return req_uri_path[0..lastSlash];
}

/// Parse an HTTP `Set-Cookie` header value into `out.*`, populating default `domain` and `path`
/// from `req_host` and `req_path`. Returns `false` if the cookie-pair is missing or has an empty name.
pub fn parseSetCookie(header: string, req_host: string, req_path: string, out: *CookieEntry): bool {
    const trimmed = cjTrim(header);
    if (trimmed.len == 0) {
        return false;
    }

    // Split first cookie-pair up to ';'.
    let semi: usize = 0;
    while (semi < trimmed.len && trimmed[semi] != (';' as u8)) {
        semi += 1;
    }
    const pair = trimmed[0..semi];

    let eq: usize = 0;
    while (eq < pair.len && pair[eq] != ('=' as u8)) {
        eq += 1;
    }
    if (eq >= pair.len) {
        return false;
    }

    const name = cjTrim(pair[0..eq]);
    if (name.len == 0) {
        return false;
    }
    const value = cjTrim(pair[(eq + 1)..pair.len]);

    let entry = CookieEntry{
        name: name,
        value: value,
        domain: req_host,
        path: defaultPath(req_path),
        max_age: 0 as i64,
        has_max_age: false,
        secure: false,
        http_only: false,
        same_site: 0 as u8,
    };

    // Parse remaining ';'-delimited attributes.
    let pos = semi;
    while (pos < trimmed.len) {
        // Skip ';'.
        pos += 1;
        let nextSemi = pos;
        while (nextSemi < trimmed.len && trimmed[nextSemi] != (';' as u8)) {
            nextSemi += 1;
        }
        const attr = cjTrim(trimmed[pos..nextSemi]);
        pos = nextSemi;

        if (attr.len == 0) {
            continue;
        }

        let aeq: usize = 0;
        while (aeq < attr.len && attr[aeq] != ('=' as u8)) {
            aeq += 1;
        }
        const aName = cjTrim(attr[0..aeq]);
        let aVal = attr[aeq..aeq];
        if (aeq < attr.len) {
            aVal = cjTrim(attr[(aeq + 1)..attr.len]);
        }

        if (cjEqIgnoreCase(aName, "domain")) {
            if (aVal.len > 0) {
                let d = aVal;
                if (d[0] == ('.' as u8)) {
                    d = d[1..d.len];
                }
                if (d.len > 0) {
                    entry.domain = d;
                }
            }
        } else if (cjEqIgnoreCase(aName, "path")) {
            if (aVal.len > 0 && aVal[0] == ('/' as u8)) {
                entry.path = aVal;
            }
        } else if (cjEqIgnoreCase(aName, "max-age")) {
            let parsedAge: i64 = 0 as i64;
            if (cjParseI64(aVal, &parsedAge)) {
                entry.max_age = parsedAge;
                entry.has_max_age = true;
            }
        } else if (cjEqIgnoreCase(aName, "secure")) {
            entry.secure = true;
        } else if (cjEqIgnoreCase(aName, "httponly")) {
            entry.http_only = true;
        } else if (cjEqIgnoreCase(aName, "samesite")) {
            if (cjEqIgnoreCase(aVal, "lax")) {
                entry.same_site = 1 as u8;
            } else if (cjEqIgnoreCase(aVal, "strict")) {
                entry.same_site = 2 as u8;
            } else if (cjEqIgnoreCase(aVal, "none")) {
                entry.same_site = 0 as u8;
            }
        }
    }

    out.* = entry;
    return true;
}

/// Evaluate whether `cookie` should be sent to `(host, path, is_https)` per RFC 6265:
/// checks non-empty name, `Secure` flag requires HTTPS, `max_age > 0` if set,
/// and domain + path matching.
pub fn shouldSend(cookie: CookieEntry, host: string, path: string, is_https: bool): bool {
    if (cookie.name.len == 0) {
        return false;
    }
    if (cookie.secure && !is_https) {
        return false;
    }
    if (cookie.has_max_age && cookie.max_age <= (0 as i64)) {
        return false;
    }
    if (!domainMatches(host, cookie.domain)) {
        return false;
    }
    if (!pathMatches(path, cookie.path)) {
        return false;
    }
    return true;
}

/// Filter matching cookies and assemble an HTTP `Cookie` request header (`k1=v1; k2=v2`) into `out`.
/// Stops cleanly before overflowing `out` so no partial cookie is ever written.
/// Returns total bytes written into `out`.
pub fn buildCookieHeader(cookies: []const CookieEntry, host: string, path: string, is_https: bool, out: []u8): usize {
    let pos: usize = 0;
    let emitted: usize = 0;
    let i: usize = 0;

    while (i < cookies.len) {
        const c = cookies[i];
        if (shouldSend(c, host, path, is_https)) {
            let need: usize = c.name.len + 1 + c.value.len;
            if (emitted > 0) {
                need += 2; // "; "
            }
            if (pos + need > out.len) {
                break;
            }
            if (emitted > 0) {
                out[pos] = ';' as u8;
                out[pos + 1] = ' ' as u8;
                pos += 2;
            }
            let ni: usize = 0;
            while (ni < c.name.len) {
                out[pos] = c.name[ni];
                pos += 1;
                ni += 1;
            }
            out[pos] = '=' as u8;
            pos += 1;
            let vi: usize = 0;
            while (vi < c.value.len) {
                out[pos] = c.value[vi];
                pos += 1;
                vi += 1;
            }
            emitted += 1;
        }
        i += 1;
    }

    return pos;
}

export {
    CookieEntry,
    domainMatches,
    pathMatches,
    defaultPath,
    parseSetCookie,
    shouldSend,
    buildCookieHeader,
};
