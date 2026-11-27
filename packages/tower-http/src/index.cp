// @cap/cors-match — Zero-heap CORS origin & preflight validation engine
// Clean-room MIT implementation for CP (compatible with npm cors & Rust tower-http::cors).

import { toLowerByte, isSpace, eqIgnoreCase, trim } from "cap:ascii";

fn findSchemeSep(s: string): i64 {
    let i: usize = 0;
    while (i + 2 < s.len) {
        if (s[i] == ':' && s[i + 1] == '/' && s[i + 2] == '/') {
            return i as i64;
        }
        i += 1;
    }
    return -1;
}

/// Validate basic RFC 6454 origin syntax (`scheme://host[:port]`, no path/query/fragment/whitespace).
pub fn isValidOrigin(origin: string): bool {
    if (origin.len == 0) { return false; }
    let sep: i64 = findSchemeSep(origin);
    if (sep <= 0) { return false; }
    let scheme_end: usize = sep as usize;
    let i: usize = 0;
    while (i < scheme_end) {
        let c: u8 = toLowerByte(origin[i]);
        let ok: bool = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
        if (!ok) { return false; }
        i += 1;
    }
    let host_start: usize = scheme_end + 3;
    if (host_start >= origin.len) { return false; }
    let j: usize = host_start;
    while (j < origin.len) {
        let c: u8 = origin[j];
        if (c == '/' || c == '?' || c == '#' || isSpace(c)) {
            return false;
        }
        j += 1;
    }
    return true;
}

/// Match an incoming `Origin` header against a pattern:
/// - `"*"` matches any valid origin (unless `allow_credentials` is true, where wildcard is rejected per Fetch spec).
/// - `"https://*.example.com"` matches subdomains like `"https://api.example.com"` (requires matching scheme and dot boundary; rejects `"https://evil-example.com"`).
/// - Exact origin `"https://app.example.com:8443"` matches case-insensitively.
pub fn matchOrigin(origin: string, pattern: string, allow_credentials: bool): bool {
    if (!isValidOrigin(origin)) {
        return false;
    }
    if (pattern.len == 1 && pattern[0] == '*') {
        return !allow_credentials;
    }
    let p_sep: i64 = findSchemeSep(pattern);
    let o_sep: i64 = findSchemeSep(origin);
    if (p_sep <= 0 || o_sep <= 0) {
        return false;
    }
    let p_scheme: string = pattern[0..(p_sep as usize)];
    let o_scheme: string = origin[0..(o_sep as usize)];
    if (!eqIgnoreCase(p_scheme, o_scheme)) {
        return false;
    }

    let p_host: string = pattern[((p_sep as usize) + 3)..pattern.len];
    let o_host: string = origin[((o_sep as usize) + 3)..origin.len];

    // Wildcard subdomain pattern: `*.example.com` (optional `:port` suffix)
    if (p_host.len >= 3 && p_host[0] == '*' && p_host[1] == '.') {
        let suffix: string = p_host[1..p_host.len]; // includes leading '.'
        if (o_host.len <= suffix.len) {
            return false;
        }
        let prefix_len: usize = o_host.len - suffix.len;
        // Subdomain prefix cannot be empty or contain '@' or '/'
        let k: usize = 0;
        while (k < prefix_len) {
            if (o_host[k] == '@' || o_host[k] == '/') {
                return false;
            }
            k += 1;
        }
        return eqIgnoreCase(o_host[prefix_len..o_host.len], suffix);
    }

    return eqIgnoreCase(o_host, p_host);
}

/// Match an incoming `Origin` against a list of allowed patterns.
pub fn matchOriginList(origin: string, patterns: []const string, allow_credentials: bool): bool {
    let i: usize = 0;
    while (i < patterns.len) {
        if (matchOrigin(origin, patterns[i], allow_credentials)) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Check if an HTTP method (case-insensitive) is allowed by a comma-separated `allowed_methods` list (or `"*"`).
pub fn isMethodAllowed(method: string, allowed_methods: string): bool {
    let m: string = trim(method);
    if (m.len == 0) { return false; }
    let i: usize = 0;
    while (i < allowed_methods.len) {
        let seg_start: usize = i;
        while (i < allowed_methods.len && allowed_methods[i] != ',') {
            i += 1;
        }
        let token: string = trim(allowed_methods[seg_start..i]);
        if ((token.len == 1 && token[0] == '*') || eqIgnoreCase(m, token)) {
            return true;
        }
        if (i < allowed_methods.len) { i += 1; }
    }
    return false;
}

/// Validate every header in a comma-separated `Access-Control-Request-Headers` list against `allowed_headers` (or `"*"`).
pub fn areHeadersAllowed(req_headers: string, allowed_headers: string): bool {
    let trimmed_req: string = trim(req_headers);
    if (trimmed_req.len == 0) { return true; }

    let i: usize = 0;
    while (i < trimmed_req.len) {
        let seg_start: usize = i;
        while (i < trimmed_req.len && trimmed_req[i] != ',') {
            i += 1;
        }
        let h: string = trim(trimmed_req[seg_start..i]);
        if (h.len > 0) {
            if (!isMethodAllowed(h, allowed_headers)) {
                return false;
            }
        }
        if (i < trimmed_req.len) { i += 1; }
    }
    return true;
}

/// Check if a method is a Fetch CORS-safelisted simple method (`GET`, `HEAD`, `POST`).
pub fn isSimpleMethod(method: string): bool {
    return eqIgnoreCase(method, "GET") || eqIgnoreCase(method, "HEAD") || eqIgnoreCase(method, "POST");
}
