// methods — HTTP methods registry & RFC 9110 / WebDAV predicates
// Clean-room MIT CP implementation of npm `methods` (120M+/wk, Express / Koa core dependency)
// Zero-heap, O(log n) case-insensitive lookup over all 34 standard HTTP methods.

import { toLowerByte } from "cap:ascii";

pub const HTTP_METHOD_COUNT: usize = 34;

/// Sorted ascending lowercase ASCII IANA/Node HTTP method names (matching npm `methods`).
pub const METHODS: [34]string = [
    "acl", "bind", "checkout", "connect", "copy", "delete", "get", "head",
    "link", "lock", "m-search", "merge", "mkactivity", "mkcalendar", "mkcol",
    "move", "notify", "options", "patch", "post", "pri", "propfind", "proppatch",
    "purge", "put", "rebind", "report", "search", "source", "subscribe",
    "trace", "unbind", "unlink", "unlock",
];

/// Sorted ascending UPPERCASE ASCII IANA/Node HTTP method names.
pub const METHODS_UPPER: [34]string = [
    "ACL", "BIND", "CHECKOUT", "CONNECT", "COPY", "DELETE", "GET", "HEAD",
    "LINK", "LOCK", "M-SEARCH", "MERGE", "MKACTIVITY", "MKCALENDAR", "MKCOL",
    "MOVE", "NOTIFY", "OPTIONS", "PATCH", "POST", "PRI", "PROPFIND", "PROPPATCH",
    "PURGE", "PUT", "REBIND", "REPORT", "SEARCH", "SOURCE", "SUBSCRIBE",
    "TRACE", "UNBIND", "UNLINK", "UNLOCK",
];

pub fn hmCmpLower(input: string, lower_target: string): i32 {
    let min_len = if (input.len < lower_target.len) { input.len } else { lower_target.len };
    let i: usize = 0;
    while (i < min_len) {
        let a = toLowerByte(input[i]);
        let b = lower_target[i];
        if (a < b) { return -1; }
        if (a > b) { return 1; }
        i = i + 1;
    }
    if (input.len < lower_target.len) { return -1; }
    if (input.len > lower_target.len) { return 1; }
    return 0;
}

/// Binary-searches `method` (case-insensitive) in the 34-entry HTTP method table.
/// Returns index `0..33`, or `-1` if unknown.
pub fn findMethodIndex(method: string): i32 {
    if (method.len == 0) { return -1; }
    let lo: usize = 0;
    let hi: usize = HTTP_METHOD_COUNT;
    while (lo < hi) {
        let mid = lo + (hi - lo) / 2;
        let cmp = hmCmpLower(method, METHODS[mid]);
        if (cmp == 0) { return mid as i32; }
        if (cmp < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return -1;
}

/// Returns true if `method` is one of the 34 standard HTTP/WebDAV methods (case-insensitive).
pub fn isHttpMethod(method: string): bool {
    return findMethodIndex(method) >= 0;
}

/// Returns the canonical lowercase method name (e.g. `"get"`, `"post"`), or `""` if unknown.
pub fn toLowerMethod(method: string): string {
    let idx = findMethodIndex(method);
    if (idx < 0) { return ""; }
    return METHODS[idx as usize];
}

/// Alias for `toLowerMethod`.
pub fn methodToLower(method: string): string {
    return toLowerMethod(method);
}

/// Returns the canonical UPPERCASE method name (e.g. `"GET"`, `"POST"`), or `""` if unknown.
pub fn toUpperMethod(method: string): string {
    let idx = findMethodIndex(method);
    if (idx < 0) { return ""; }
    return METHODS_UPPER[idx as usize];
}

/// Alias for `toUpperMethod`.
pub fn methodToUpper(method: string): string {
    return toUpperMethod(method);
}

/// Returns the lowercase method name at index `idx` (`0..HTTP_METHOD_COUNT-1`).
pub fn methodAt(idx: usize): string {
    if (idx >= HTTP_METHOD_COUNT) { return ""; }
    return METHODS[idx];
}

/// Returns the UPPERCASE method name at index `idx` (`0..HTTP_METHOD_COUNT-1`).
pub fn methodUpperAt(idx: usize): string {
    if (idx >= HTTP_METHOD_COUNT) { return ""; }
    return METHODS_UPPER[idx];
}

/// RFC 9110 §9.2.1 — Safe methods (read-only): GET, HEAD, OPTIONS, TRACE, PROPFIND, REPORT, SEARCH.
pub fn isSafeMethod(method: string): bool {
    let idx = findMethodIndex(method);
    // indices: 6=get, 7=head, 17=options, 21=propfind, 26=report, 27=search, 30=trace
    return idx == 6 || idx == 7 || idx == 17 || idx == 21 || idx == 26 || idx == 27 || idx == 30;
}

/// RFC 9110 §9.2.2 — Idempotent methods: all safe methods + PUT, DELETE, COPY, MOVE, MKCOL, PROPPATCH, LOCK, UNLOCK, UNLINK.
pub fn isIdempotentMethod(method: string): bool {
    let idx = findMethodIndex(method);
    if (idx < 0) { return false; }
    if (isSafeMethod(method)) { return true; }
    // 4=copy, 5=delete, 9=lock, 14=mkcol, 15=move, 22=proppatch, 24=put, 32=unlink, 33=unlock
    return idx == 4 || idx == 5 || idx == 9 || idx == 14 || idx == 15
        || idx == 22 || idx == 24 || idx == 32 || idx == 33;
}

/// RFC 9110 §9.2.3 — Cacheable methods: GET, HEAD, POST.
pub fn isCacheableMethod(method: string): bool {
    let idx = findMethodIndex(method);
    return idx == 6 || idx == 7 || idx == 19;
}

/// Returns true if the method conventionally carries a request payload body (POST, PUT, PATCH, PROPFIND, PROPPATCH, REPORT, SEARCH, MERGE, LOCK).
pub fn mayHaveRequestBody(method: string): bool {
    let idx = findMethodIndex(method);
    return idx == 9 || idx == 11 || idx == 18 || idx == 19 || idx == 21
        || idx == 22 || idx == 24 || idx == 26 || idx == 27;
}

/// Returns true if `method` is one of the 9 core RFC 9110 / RFC 5789 HTTP methods
/// (GET, HEAD, POST, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH).
pub fn isStandardRfc9110Method(method: string): bool {
    let idx = findMethodIndex(method);
    // 3=connect, 5=delete, 6=get, 7=head, 17=options, 18=patch, 19=post, 24=put, 30=trace
    return idx == 3 || idx == 5 || idx == 6 || idx == 7 || idx == 17
        || idx == 18 || idx == 19 || idx == 24 || idx == 30;
}

/// Returns true if `method` is a WebDAV / CalDAV / Versioning extension method.
pub fn isWebDavMethod(method: string): bool {
    let idx = findMethodIndex(method);
    // 0=acl, 1=bind, 2=checkout, 4=copy, 9=lock, 11=merge, 12=mkactivity, 13=mkcalendar,
    // 14=mkcol, 15=move, 21=propfind, 22=proppatch, 25=rebind, 26=report, 27=search,
    // 31=unbind, 33=unlock
    return idx == 0 || idx == 1 || idx == 2 || idx == 4 || idx == 9 || idx == 11
        || idx == 12 || idx == 13 || idx == 14 || idx == 15 || idx == 21 || idx == 22
        || idx == 25 || idx == 26 || idx == 27 || idx == 31 || idx == 33;
}

/// Returns true if a valid HTTP method is permitted to return a response message body (all except HEAD).
pub fn allowsResponseBody(method: string): bool {
    let idx = findMethodIndex(method);
    return idx >= 0 && idx != 7;
}
