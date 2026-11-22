// Source: Absorbed from /data/dev/bun/src/http_types/Method.cp | Synced: 2026-11-22T10:18:27Z
// Pure-CP zero-allocation HTTP / WebDAV / QUERY Method enum (36 methods)
// with RFC 9110 safety, idempotency, cacheability, body, and CORS safelist predicates.

pub enum HttpMethod: u8 {
    ACL = 0,
    BIND = 1,
    CHECKOUT = 2,
    CONNECT = 3,
    COPY = 4,
    DELETE = 5,
    GET = 6,
    HEAD = 7,
    LINK = 8,
    LOCK = 9,
    M_SEARCH = 10,
    MERGE = 11,
    MKACTIVITY = 12,
    MKADDRESSBOOK = 13,
    MKCALENDAR = 14,
    MKCOL = 15,
    MOVE = 16,
    NOTIFY = 17,
    OPTIONS = 18,
    PATCH = 19,
    POST = 20,
    PROPFIND = 21,
    PROPPATCH = 22,
    PURGE = 23,
    PUT = 24,
    QUERY = 25,
    REBIND = 26,
    REPORT = 27,
    SEARCH = 28,
    SOURCE = 29,
    SUBSCRIBE = 30,
    TRACE = 31,
    UNBIND = 32,
    UNLINK = 33,
    UNLOCK = 34,
    UNSUBSCRIBE = 35,

    /// Parse an HTTP method token (case-insensitive, e.g. "GET", "post", "M-SEARCH").
    pub static fn parse(input: []const u8): HttpMethod? {
        if (input.len == 0 || input.len > 13) return null;
        if (eqIgnoreCase(input, "GET")) return .GET;
        if (eqIgnoreCase(input, "POST")) return .POST;
        if (eqIgnoreCase(input, "PUT")) return .PUT;
        if (eqIgnoreCase(input, "DELETE")) return .DELETE;
        if (eqIgnoreCase(input, "PATCH")) return .PATCH;
        if (eqIgnoreCase(input, "HEAD")) return .HEAD;
        if (eqIgnoreCase(input, "OPTIONS")) return .OPTIONS;
        if (eqIgnoreCase(input, "CONNECT")) return .CONNECT;
        if (eqIgnoreCase(input, "TRACE")) return .TRACE;
        if (eqIgnoreCase(input, "QUERY")) return .QUERY;
        if (eqIgnoreCase(input, "ACL")) return .ACL;
        if (eqIgnoreCase(input, "BIND")) return .BIND;
        if (eqIgnoreCase(input, "CHECKOUT")) return .CHECKOUT;
        if (eqIgnoreCase(input, "COPY")) return .COPY;
        if (eqIgnoreCase(input, "LINK")) return .LINK;
        if (eqIgnoreCase(input, "LOCK")) return .LOCK;
        if (eqIgnoreCase(input, "M-SEARCH")) return .M_SEARCH;
        if (eqIgnoreCase(input, "MERGE")) return .MERGE;
        if (eqIgnoreCase(input, "MKACTIVITY")) return .MKACTIVITY;
        if (eqIgnoreCase(input, "MKADDRESSBOOK")) return .MKADDRESSBOOK;
        if (eqIgnoreCase(input, "MKCALENDAR")) return .MKCALENDAR;
        if (eqIgnoreCase(input, "MKCOL")) return .MKCOL;
        if (eqIgnoreCase(input, "MOVE")) return .MOVE;
        if (eqIgnoreCase(input, "NOTIFY")) return .NOTIFY;
        if (eqIgnoreCase(input, "PROPFIND")) return .PROPFIND;
        if (eqIgnoreCase(input, "PROPPATCH")) return .PROPPATCH;
        if (eqIgnoreCase(input, "PURGE")) return .PURGE;
        if (eqIgnoreCase(input, "REBIND")) return .REBIND;
        if (eqIgnoreCase(input, "REPORT")) return .REPORT;
        if (eqIgnoreCase(input, "SEARCH")) return .SEARCH;
        if (eqIgnoreCase(input, "SOURCE")) return .SOURCE;
        if (eqIgnoreCase(input, "SUBSCRIBE")) return .SUBSCRIBE;
        if (eqIgnoreCase(input, "UNBIND")) return .UNBIND;
        if (eqIgnoreCase(input, "UNLINK")) return .UNLINK;
        if (eqIgnoreCase(input, "UNLOCK")) return .UNLOCK;
        if (eqIgnoreCase(input, "UNSUBSCRIBE")) return .UNSUBSCRIBE;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .ACL => "ACL",
            .BIND => "BIND",
            .CHECKOUT => "CHECKOUT",
            .CONNECT => "CONNECT",
            .COPY => "COPY",
            .DELETE => "DELETE",
            .GET => "GET",
            .HEAD => "HEAD",
            .LINK => "LINK",
            .LOCK => "LOCK",
            .M_SEARCH => "M-SEARCH",
            .MERGE => "MERGE",
            .MKACTIVITY => "MKACTIVITY",
            .MKADDRESSBOOK => "MKADDRESSBOOK",
            .MKCALENDAR => "MKCALENDAR",
            .MKCOL => "MKCOL",
            .MOVE => "MOVE",
            .NOTIFY => "NOTIFY",
            .OPTIONS => "OPTIONS",
            .PATCH => "PATCH",
            .POST => "POST",
            .PROPFIND => "PROPFIND",
            .PROPPATCH => "PROPPATCH",
            .PURGE => "PURGE",
            .PUT => "PUT",
            .QUERY => "QUERY",
            .REBIND => "REBIND",
            .REPORT => "REPORT",
            .SEARCH => "SEARCH",
            .SOURCE => "SOURCE",
            .SUBSCRIBE => "SUBSCRIBE",
            .TRACE => "TRACE",
            .UNBIND => "UNBIND",
            .UNLINK => "UNLINK",
            .UNLOCK => "UNLOCK",
            .UNSUBSCRIBE => "UNSUBSCRIBE",
        };
    }

    /// True if responses to this method may carry a body (all except HEAD and TRACE).
    pub fn hasResponseBody(): bool {
        return _ != .HEAD && _ != .TRACE;
    }

    /// True if requests with this method may carry a request body.
    pub fn hasRequestBody(): bool {
        return _ != .GET && _ != .HEAD && _ != .OPTIONS && _ != .TRACE;
    }

    /// RFC 9110 §9.2.1: Safe (read-only) HTTP methods.
    pub fn isSafe(): bool {
        return _ == .GET || _ == .HEAD || _ == .OPTIONS || _ == .TRACE || _ == .QUERY;
    }

    /// RFC 9110 §9.2.2: Idempotent HTTP methods.
    pub fn isIdempotent(): bool {
        return _ == .GET || _ == .HEAD || _ == .PUT || _ == .DELETE || _ == .OPTIONS || _ == .TRACE || _ == .QUERY;
    }

    /// RFC 9110 §9.2.3: Cacheable HTTP methods.
    pub fn isCacheable(): bool {
        return _ == .GET || _ == .HEAD || _ == .QUERY;
    }

    /// WHATWG Fetch: CORS-safelisted methods (`GET`, `HEAD`, `POST`).
    pub fn isCorsSafelisted(): bool {
        return _ == .GET || _ == .HEAD || _ == .POST;
    }

    /// Bitmask bit for compact 64-bit method sets (e.g. route allow-lists).
    pub fn bit(): u64 {
        return @as(u64, 1) << @as(u6, @intCast(@intFromEnum(_)));
    }
}

/// Compact 64-bit set of `HttpMethod` values (zero allocation).
pub struct MethodSet {
    bits: u64,

    pub static fn empty(): MethodSet {
        return MethodSet{ bits: 0 };
    }

    pub fn insert(*_, m: HttpMethod) {
        _.bits |= m.bit();
    }

    pub fn remove(*_, m: HttpMethod) {
        _.bits &= ~m.bit();
    }

    pub fn contains(m: HttpMethod): bool {
        return (_.bits & m.bit()) != 0;
    }

    /// Format an HTTP `Allow` header value (e.g. `"GET, HEAD, POST"`) into `out`.
    pub fn formatAllowHeader(out: []u8): []const u8 {
        let p: usize = 0;
        let idx: u8 = 0;
        while (idx < 36) : (idx += 1) {
            const m: HttpMethod = @enumFromInt(idx);
            if (_.contains(m)) {
                if (p > 0) p = appendStr(out, p, ", ");
                p = appendStr(out, p, m.asStr());
            }
        }
        return out[0..p];
    }
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}
