// packages/@cap/http-types/src/index.cp
// Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types.
// Clean-room MIT parity with crates.io `http` v1 & RFC 9110.
// Composes `@cap/unicase` (`eqAscii`) for case-insensitive HeaderName & Version matching.

import {
    eqAscii,
} from "../../unicase/src/index.cp";

// --- HTTP Method constants ---
pub const METHOD_UNKNOWN: u8 = 0;
pub const METHOD_GET: u8 = 1;
pub const METHOD_HEAD: u8 = 2;
pub const METHOD_POST: u8 = 3;
pub const METHOD_PUT: u8 = 4;
pub const METHOD_DELETE: u8 = 5;
pub const METHOD_CONNECT: u8 = 6;
pub const METHOD_OPTIONS: u8 = 7;
pub const METHOD_TRACE: u8 = 8;
pub const METHOD_PATCH: u8 = 9;

// --- HTTP Version constants ---
pub const HTTP_UNKNOWN: u8 = 0;
pub const HTTP_09: u8 = 1;
pub const HTTP_10: u8 = 2;
pub const HTTP_11: u8 = 3;
pub const HTTP_2: u8 = 4;
pub const HTTP_3: u8 = 5;

fn htpStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// RFC 9110 Section 5.6.2 `tchar` check for valid HTTP header field-name bytes:
/// `!`, `#`, `$`, `%`, `&`, `'`, `*`, `+`, `-`, `.`, `^`, `_`, `` ` ``, `|`, `~`, DIGIT, ALPHA.
fn htpIsTchar(b: u8): bool {
    if (b >= (0x30 as u8) && b <= (0x39 as u8)) { return true; } // 0-9
    if (b >= (0x41 as u8) && b <= (0x5A as u8)) { return true; } // A-Z
    if (b >= (0x61 as u8) && b <= (0x7A as u8)) { return true; } // a-z
    if (b == (0x21 as u8) || b == (0x23 as u8) || b == (0x24 as u8) || b == (0x25 as u8)) { return true; } // ! # $ %
    if (b == (0x26 as u8) || b == (0x27 as u8) || b == (0x2A as u8) || b == (0x2B as u8)) { return true; } // & ' * +
    if (b == (0x2D as u8) || b == (0x2E as u8) || b == (0x5E as u8) || b == (0x5F as u8)) { return true; } // - . ^ _
    if (b == (0x60 as u8) || b == (0x7C as u8) || b == (0x7E as u8)) { return true; }                      // ` | ~
    return false;
}

/// Parse a case-sensitive RFC 9110 HTTP method token into a `METHOD_*` constant.
/// Returns `METHOD_UNKNOWN` (0) for unrecognized methods.
pub fn parseMethod(s: string): u8 {
    if (htpStrEq(s, "GET")) { return METHOD_GET; }
    if (htpStrEq(s, "HEAD")) { return METHOD_HEAD; }
    if (htpStrEq(s, "POST")) { return METHOD_POST; }
    if (htpStrEq(s, "PUT")) { return METHOD_PUT; }
    if (htpStrEq(s, "DELETE")) { return METHOD_DELETE; }
    if (htpStrEq(s, "CONNECT")) { return METHOD_CONNECT; }
    if (htpStrEq(s, "OPTIONS")) { return METHOD_OPTIONS; }
    if (htpStrEq(s, "TRACE")) { return METHOD_TRACE; }
    if (htpStrEq(s, "PATCH")) { return METHOD_PATCH; }
    return METHOD_UNKNOWN;
}

/// Return the canonical uppercase HTTP method string for `m`, or `""` if unknown.
pub fn methodAsStr(m: u8): string {
    if (m == METHOD_GET) { return "GET"; }
    if (m == METHOD_HEAD) { return "HEAD"; }
    if (m == METHOD_POST) { return "POST"; }
    if (m == METHOD_PUT) { return "PUT"; }
    if (m == METHOD_DELETE) { return "DELETE"; }
    if (m == METHOD_CONNECT) { return "CONNECT"; }
    if (m == METHOD_OPTIONS) { return "OPTIONS"; }
    if (m == METHOD_TRACE) { return "TRACE"; }
    if (m == METHOD_PATCH) { return "PATCH"; }
    return "";
}

/// RFC 9110 Section 9.2.1: returns `true` for safe methods (`GET`, `HEAD`, `OPTIONS`, `TRACE`).
pub fn isSafeMethod(m: u8): bool {
    return m == METHOD_GET || m == METHOD_HEAD || m == METHOD_OPTIONS || m == METHOD_TRACE;
}

/// RFC 9110 Section 9.2.2: returns `true` for idempotent methods
/// (`GET`, `HEAD`, `OPTIONS`, `TRACE`, `PUT`, `DELETE`).
pub fn isIdempotentMethod(m: u8): bool {
    return isSafeMethod(m) || m == METHOD_PUT || m == METHOD_DELETE;
}

/// Validated HTTP status code (100..=599).
pub struct StatusCode {
    pub code: u16,

    /// Construct a `StatusCode` if `code` is within the valid HTTP range `100..=599`.
    pub static fn fromU16(code: u16): StatusCode? {
        if (code < (100 as u16) || code > (599 as u16)) {
            return null;
        }
        return StatusCode { code: code };
    }

    /// Return the numeric status code as `u16`.
    pub fn asU16(): u16 {
        return _.code;
    }

    /// Return the IANA / RFC 9110 canonical reason phrase, or `""` if unassigned.
    pub fn canonicalReason(): string {
        const c = _.code;
        // 1xx Informational
        if (c == 100) { return "Continue"; }
        if (c == 101) { return "Switching Protocols"; }
        if (c == 102) { return "Processing"; }
        if (c == 103) { return "Early Hints"; }
        // 2xx Success
        if (c == 200) { return "OK"; }
        if (c == 201) { return "Created"; }
        if (c == 202) { return "Accepted"; }
        if (c == 203) { return "Non Authoritative Information"; }
        if (c == 204) { return "No Content"; }
        if (c == 205) { return "Reset Content"; }
        if (c == 206) { return "Partial Content"; }
        if (c == 207) { return "Multi-Status"; }
        if (c == 208) { return "Already Reported"; }
        if (c == 226) { return "IM Used"; }
        // 3xx Redirection
        if (c == 300) { return "Multiple Choices"; }
        if (c == 301) { return "Moved Permanently"; }
        if (c == 302) { return "Found"; }
        if (c == 303) { return "See Other"; }
        if (c == 304) { return "Not Modified"; }
        if (c == 305) { return "Use Proxy"; }
        if (c == 307) { return "Temporary Redirect"; }
        if (c == 308) { return "Permanent Redirect"; }
        // 4xx Client Error
        if (c == 400) { return "Bad Request"; }
        if (c == 401) { return "Unauthorized"; }
        if (c == 402) { return "Payment Required"; }
        if (c == 403) { return "Forbidden"; }
        if (c == 404) { return "Not Found"; }
        if (c == 405) { return "Method Not Allowed"; }
        if (c == 406) { return "Not Acceptable"; }
        if (c == 407) { return "Proxy Authentication Required"; }
        if (c == 408) { return "Request Timeout"; }
        if (c == 409) { return "Conflict"; }
        if (c == 410) { return "Gone"; }
        if (c == 411) { return "Length Required"; }
        if (c == 412) { return "Precondition Failed"; }
        if (c == 413) { return "Payload Too Large"; }
        if (c == 414) { return "URI Too Long"; }
        if (c == 415) { return "Unsupported Media Type"; }
        if (c == 416) { return "Range Not Satisfiable"; }
        if (c == 417) { return "Expectation Failed"; }
        if (c == 418) { return "I'm a teapot"; }
        if (c == 421) { return "Misdirected Request"; }
        if (c == 422) { return "Unprocessable Entity"; }
        if (c == 423) { return "Locked"; }
        if (c == 424) { return "Failed Dependency"; }
        if (c == 425) { return "Too Early"; }
        if (c == 426) { return "Upgrade Required"; }
        if (c == 428) { return "Precondition Required"; }
        if (c == 429) { return "Too Many Requests"; }
        if (c == 431) { return "Request Header Fields Too Large"; }
        if (c == 451) { return "Unavailable For Legal Reasons"; }
        // 5xx Server Error
        if (c == 500) { return "Internal Server Error"; }
        if (c == 501) { return "Not Implemented"; }
        if (c == 502) { return "Bad Gateway"; }
        if (c == 503) { return "Service Unavailable"; }
        if (c == 504) { return "Gateway Timeout"; }
        if (c == 505) { return "HTTP Version Not Supported"; }
        if (c == 506) { return "Variant Also Negotiates"; }
        if (c == 507) { return "Insufficient Storage"; }
        if (c == 508) { return "Loop Detected"; }
        if (c == 510) { return "Not Extended"; }
        if (c == 511) { return "Network Authentication Required"; }
        return "";
    }

    pub fn isInformational(): bool {
        return _.code >= (100 as u16) && _.code < (200 as u16);
    }

    pub fn isSuccess(): bool {
        return _.code >= (200 as u16) && _.code < (300 as u16);
    }

    pub fn isRedirection(): bool {
        return _.code >= (300 as u16) && _.code < (400 as u16);
    }

    pub fn isClientError(): bool {
        return _.code >= (400 as u16) && _.code < (500 as u16);
    }

    pub fn isServerError(): bool {
        return _.code >= (500 as u16) && _.code < (600 as u16);
    }
}

/// Parse an HTTP version token (ASCII case-insensitive via `@cap/unicase`).
/// Supports `HTTP/0.9`, `HTTP/1.0`, `HTTP/1.1`, `HTTP/2.0` / `HTTP/2`, `HTTP/3.0` / `HTTP/3`.
pub fn parseHttpVersion(s: string): u8 {
    if (eqAscii(s, "HTTP/0.9")) { return HTTP_09; }
    if (eqAscii(s, "HTTP/1.0")) { return HTTP_10; }
    if (eqAscii(s, "HTTP/1.1")) { return HTTP_11; }
    if (eqAscii(s, "HTTP/2.0") || eqAscii(s, "HTTP/2")) { return HTTP_2; }
    if (eqAscii(s, "HTTP/3.0") || eqAscii(s, "HTTP/3")) { return HTTP_3; }
    return HTTP_UNKNOWN;
}

/// Return the canonical HTTP version string for `v`, or `""` if unknown.
pub fn httpVersionAsStr(v: u8): string {
    if (v == HTTP_09) { return "HTTP/0.9"; }
    if (v == HTTP_10) { return "HTTP/1.0"; }
    if (v == HTTP_11) { return "HTTP/1.1"; }
    if (v == HTTP_2) { return "HTTP/2.0"; }
    if (v == HTTP_3) { return "HTTP/3.0"; }
    return "";
}

/// Validate an HTTP header field-name per RFC 9110 (`1*tchar`, non-empty).
pub fn isValidHeaderName(name: string): bool {
    if (name.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < name.len) {
        if (!htpIsTchar(name[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// ASCII case-insensitive HTTP header name equality via `@cap/unicase`.
pub fn headerNameEq(a: string, b: string): bool {
    return eqAscii(a, b);
}

export {
    METHOD_UNKNOWN,
    METHOD_GET,
    METHOD_HEAD,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_TRACE,
    METHOD_PATCH,
    HTTP_UNKNOWN,
    HTTP_09,
    HTTP_10,
    HTTP_11,
    HTTP_2,
    HTTP_3,
    parseMethod,
    methodAsStr,
    isSafeMethod,
    isIdempotentMethod,
    StatusCode,
    parseHttpVersion,
    httpVersionAsStr,
    isValidHeaderName,
    headerNameEq,
};
