// @cap/cookie — Zero-allocation RFC 6265 HTTP Cookie Parser & Set-Cookie Serializer
// Clean-room pure-CP implementation of RFC 6265 & CHIPS Partitioned cookies (inspired by npm `cookie` & Rust `cookie`, MIT License).

const std = @import("std");

pub err CookieError {
    InvalidName,
    InvalidValue,
    InvalidAttribute,
    BufferTooSmall,
}

pub enum SameSite: u8 {
    Unset = 0,
    Strict = 1,
    Lax = 2,
    None = 3,
}

/// A single parsed `name=value` pair from an HTTP `Cookie` request header.
pub struct CookiePair {
    name: []const u8,
    value: []const u8,
}

/// Options for building a `Set-Cookie` HTTP response header (RFC 6265 + CHIPS Partitioned).
pub struct SetCookieOptions {
    path: []const u8 = "",
    domain: []const u8 = "",
    expires: []const u8 = "",
    max_age: i64? = null,
    http_only: bool = false,
    secure: bool = false,
    partitioned: bool = false,
    same_site: SameSite = SameSite.Unset,
}

fn isTokenChar(c: u8): bool {
    if (c <= 0x20 || c >= 0x7F) return false;
    return c != '(' && c != ')' && c != '<' && c != '>' && c != '@'
        && c != ',' && c != ';' && c != ':' && c != '\\' && c != '"'
        && c != '/' && c != '[' && c != ']' && c != '?' && c != '='
        && c != '{' && c != '}';
}

fn isCookieOctet(c: u8): bool {
    return c == 0x21
        || (c >= 0x23 && c <= 0x2B)
        || (c >= 0x2D && c <= 0x3A)
        || (c >= 0x3C && c <= 0x5B)
        || (c >= 0x5D && c <= 0x7E);
}

fn isAttrSafe(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < 0x20 || c == 0x7F || c == ';') return false;
    }
    return true;
}

/// Validates an RFC 6265 cookie name (non-empty HTTP token).
pub fn isValidCookieName(name: []const u8): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        if (!isTokenChar(name[i])) return false;
    }
    return true;
}

/// Validates an RFC 6265 cookie value (cookie-octet sequence, optionally wrapped in double quotes).
pub fn isValidCookieValue(val: []const u8): bool {
    let s = val;
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        s = s[1 .. s.len - 1];
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isCookieOctet(s[i])) return false;
    }
    return true;
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

/// Zero-allocation iterator over `name=value` pairs in an HTTP `Cookie` request header.
pub struct CookieIter {
    src: []const u8,
    pos: usize = 0,

    pub static fn init(header: []const u8): CookieIter {
        return CookieIter{ src: header, pos: 0 };
    }

    pub fn next(*_): CookiePair? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            let end = start;
            while (end < _.src.len && _.src[end] != ';') : (end += 1) {}
            _.pos = if (end < _.src.len) end + 1 else end;

            const seg = trimOws(_.src[start..end]);
            if (seg.len == 0) continue;

            let eq: usize? = null;
            let i: usize = 0;
            while (i < seg.len) : (i += 1) {
                if (seg[i] == '=') {
                    eq = i;
                    break;
                }
            }
            const eq_idx = eq ?? continue;
            const name = trimOws(seg[0..eq_idx]);
            if (name.len == 0) continue;
            let val = trimOws(seg[eq_idx + 1 ..]);
            // Strip optional RFC 6265 surrounding quotes
            if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                val = val[1 .. val.len - 1];
            }
            return CookiePair{ name: name, value: val };
        }
        return null;
    }
}

/// Looks up the first cookie matching `name` (case-sensitive per RFC 6265) in a `Cookie` header.
pub fn getCookie(header: []const u8, name: []const u8): ([]const u8)? {
    let it = CookieIter.init(header);
    while (pair := it.next()) {
        if (std.mem.eql(u8, pair.name, name)) {
            return pair.value;
        }
    }
    return null;
}

/// Counts valid `name=value` pairs in a `Cookie` header.
pub fn countCookies(header: []const u8): usize {
    let it = CookieIter.init(header);
    let count: usize = 0;
    while (pair := it.next()) {
        * = pair;
        count += 1;
    }
    return count;
}

fn appendStr(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    @memcpy(out[pos.* .. pos.* + s.len], s);
    pos.* += s.len;
    return true;
}

/// Serializes a `Set-Cookie` header value into `out` with strict RFC 6265 validation
/// (rejecting CRLF/semicolon header injection in name, value, path, domain, or expires).
pub fn serializeSetCookie(
    name: []const u8,
    value: []const u8,
    opts: SetCookieOptions,
    out: []u8,
): ([]const u8) ! CookieError {
    if (!isValidCookieName(name)) return CookieError.InvalidName;
    if (!isValidCookieValue(value)) return CookieError.InvalidValue;

    let pos: usize = 0;
    if (!appendStr(out, &pos, name)) return CookieError.BufferTooSmall;
    if (!appendStr(out, &pos, "=")) return CookieError.BufferTooSmall;
    if (!appendStr(out, &pos, value)) return CookieError.BufferTooSmall;

    if (ma := opts.max_age) {
        let num_buf: [32]u8 = [0; 32];
        const ma_str = std.fmt.bufPrint(num_buf[0..], "{d}", .{ma}) catch return CookieError.BufferTooSmall;
        if (!appendStr(out, &pos, "; Max-Age=")) return CookieError.BufferTooSmall;
        if (!appendStr(out, &pos, ma_str)) return CookieError.BufferTooSmall;
    }

    if (opts.domain.len > 0) {
        if (!isAttrSafe(opts.domain)) return CookieError.InvalidAttribute;
        if (!appendStr(out, &pos, "; Domain=")) return CookieError.BufferTooSmall;
        if (!appendStr(out, &pos, opts.domain)) return CookieError.BufferTooSmall;
    }

    if (opts.path.len > 0) {
        if (!isAttrSafe(opts.path)) return CookieError.InvalidAttribute;
        if (!appendStr(out, &pos, "; Path=")) return CookieError.BufferTooSmall;
        if (!appendStr(out, &pos, opts.path)) return CookieError.BufferTooSmall;
    }

    if (opts.expires.len > 0) {
        if (!isAttrSafe(opts.expires)) return CookieError.InvalidAttribute;
        if (!appendStr(out, &pos, "; Expires=")) return CookieError.BufferTooSmall;
        if (!appendStr(out, &pos, opts.expires)) return CookieError.BufferTooSmall;
    }

    if (opts.http_only) {
        if (!appendStr(out, &pos, "; HttpOnly")) return CookieError.BufferTooSmall;
    }

    if (opts.secure) {
        if (!appendStr(out, &pos, "; Secure")) return CookieError.BufferTooSmall;
    }

    if (opts.partitioned) {
        if (!appendStr(out, &pos, "; Partitioned")) return CookieError.BufferTooSmall;
    }

    switch (opts.same_site) {
        .Unset => {},
        .Strict => {
            if (!appendStr(out, &pos, "; SameSite=Strict")) return CookieError.BufferTooSmall;
        },
        .Lax => {
            if (!appendStr(out, &pos, "; SameSite=Lax")) return CookieError.BufferTooSmall;
        },
        .None => {
            if (!appendStr(out, &pos, "; SameSite=None")) return CookieError.BufferTooSmall;
        },
    }

    return out[0..pos];
}

/// Builds a `Set-Cookie` header that expires/clears the named cookie immediately (`Max-Age=0`).
pub fn serializeClearCookie(name: []const u8, path: []const u8, out: []u8): ([]const u8) ! CookieError {
    return serializeSetCookie(
        name,
        "",
        SetCookieOptions{
            path: path,
            max_age: 0,
            expires: "Thu, 01 Jan 1970 00:00:00 GMT",
        },
        out,
    );
}
