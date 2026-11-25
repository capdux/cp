// Origin: Pure-CP Stdlib (Official Flagship REST API Framework: ElysiaJS Chainable API + Axum Zero-Reflection Kernel) | Created: 2026-11-24
// ============================================================================
// Pure-CP Zero-Alloc Flagship REST API Framework (`cap:web`)
//   - ElysiaJS-inspired fluent chainable route & middleware registration
//   - Axum-inspired zero-reflection compile-time Context extractors
//   - Pre-compiled segment router: static, `:param`, `*wildcard`, 404 vs 405
//   - Onion / short-circuit middleware pipeline with automatic response header merge
//   - Automatic OpenAPI 3.1.0 JSON specification generator
//   - Pure zero-I/O `app.dispatch(req)` kernel ready for unit tests or TCP/HTTP servers
// ============================================================================

const std = @import("std");
import { RadixRouter, RadixMatchResult } from "./radixRouter.cp";

pub const APP_MAX_ROUTES: usize = 128;
pub const APP_MAX_MIDDLEWARES: usize = 16;
pub const APP_MAX_HOOKS: usize = 8;
pub const APP_MAX_SEGMENTS: usize = 16;
pub const APP_MAX_PARAMS: usize = 16;
pub const APP_MAX_HEADERS: usize = 32;
pub const APP_MAX_PATH_LEN: usize = 256;
pub const APP_MAX_STATE_KV: usize = 16;
pub const APP_MAX_DERIVED_KV: usize = 16;

// ============================================================================
// Internal Zero-Alloc String, ASCII, Numeric & JSON Helpers
// ============================================================================

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn bytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn asciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (asciiLower(a[i]) != asciiLower(b[i])) return false;
    }
    return true;
}

fn parseDecimalU64(s: string): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const digit: u64 = (c - '0') as u64;
        if (acc > (std.math.maxInt(u64) - digit) / 10) return null;
        acc = acc * 10 + digit;
    }
    return acc;
}

fn parseDecimalI64(s: string): i64? {
    if (s.len == 0) return null;
    let neg: bool = false;
    let start: usize = 0;
    if (s[0] == '-') {
        neg = true;
        start = 1;
    } else if (s[0] == '+') {
        start = 1;
    }
    if (start >= s.len) return null;
    if (mag := parseDecimalU64(s[start..])) {
        const min_abs: u64 = (1 as u64) << 63;
        if (neg) {
            if (mag > min_abs) return null;
            if (mag == min_abs) return std.math.minInt(i64);
            return -(mag as i64);
        } else {
            if (mag > 9223372036854775807) return null;
            return mag as i64;
        }
    }
    return null;
}

fn writeI64Decimal(out_buf: []u8, pos: *usize, val: i64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = 0;
    if (val < 0) {
        if (!writeByte(out_buf, pos, '-')) return false;
        v = if (val == std.math.minInt(i64)) ((1 as u64) << 63) else ((-val) as u64);
    } else {
        v = val as u64;
    }
    if (v == 0) {
        return writeByte(out_buf, pos, '0');
    }
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        if (!writeByte(out_buf, pos, tmp[n])) return false;
    }
    return true;
}

fn trimAsciiSpaces(s: string): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

pub enum CookieSameSite {
    None,
    Lax,
    Strict,
}

pub struct CookieOptions {
    path: string = "/";
    domain: string = "";
    max_age: i64 = -1; // < 0 means omit Max-Age
    http_only: bool = true;
    secure: bool = false;
    same_site: CookieSameSite = CookieSameSite.Lax;
}

/// Format an RFC 6265 `Set-Cookie` header value into `out_buf` (zero heap).
pub fn formatSetCookie(name: string, value: string, opts: CookieOptions, out_buf: []u8): usize? {
    if (name.len == 0) return null;
    let pos: usize = 0;
    if (!writeRaw(out_buf, *pos, name)) return null;
    if (!writeByte(out_buf, *pos, '=')) return null;
    if (!writeRaw(out_buf, *pos, value)) return null;
    if (opts.path.len > 0) {
        if (!writeRaw(out_buf, *pos, "; Path=")) return null;
        if (!writeRaw(out_buf, *pos, opts.path)) return null;
    }
    if (opts.domain.len > 0) {
        if (!writeRaw(out_buf, *pos, "; Domain=")) return null;
        if (!writeRaw(out_buf, *pos, opts.domain)) return null;
    }
    if (opts.max_age >= 0) {
        if (!writeRaw(out_buf, *pos, "; Max-Age=")) return null;
        if (!writeI64Decimal(out_buf, *pos, opts.max_age)) return null;
    }
    if (opts.http_only) {
        if (!writeRaw(out_buf, *pos, "; HttpOnly")) return null;
    }
    if (opts.secure) {
        if (!writeRaw(out_buf, *pos, "; Secure")) return null;
    }
    match (opts.same_site) {
        .Strict => {
            if (!writeRaw(out_buf, *pos, "; SameSite=Strict")) return null;
        },
        .Lax => {
            if (!writeRaw(out_buf, *pos, "; SameSite=Lax")) return null;
        },
        .None => {
            if (!writeRaw(out_buf, *pos, "; SameSite=None")) return null;
        },
    }
    return pos;
}

pub enum AppErrorCode {
    None,
    NotFound,
    MethodNotAllowed,
    Validation,
    Parse,
    Unauthorized,
    Forbidden,
    InternalServerError,
    Custom,

    pub fn asString(): string {
        match (_) {
            .None => return "NONE",
            .NotFound => return "NOT_FOUND",
            .MethodNotAllowed => return "METHOD_NOT_ALLOWED",
            .Validation => return "VALIDATION",
            .Parse => return "PARSE",
            .Unauthorized => return "UNAUTHORIZED",
            .Forbidden => return "FORBIDDEN",
            .InternalServerError => return "INTERNAL_SERVER_ERROR",
            .Custom => return "CUSTOM",
        }
    }

    pub fn defaultStatus(): u16 {
        match (_) {
            .None => return 200,
            .NotFound => return 404,
            .MethodNotAllowed => return 405,
            .Validation => return 422,
            .Parse => return 400,
            .Unauthorized => return 401,
            .Forbidden => return 403,
            .InternalServerError => return 500,
            .Custom => return 500,
        }
    }
}

pub enum PluginScope {
    Local,
    Scoped,
    Global,
}

fn findByte(s: string, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return null;
}

fn writeByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn hexDigitLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

fn writeJsonEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    if (!writeByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!writeRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!writeRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!writeRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!writeRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!writeRaw(out_buf, pos, "\\t")) return false;
        } else if (b < 0x20) {
            if (!writeRaw(out_buf, pos, "\\u00")) return false;
            if (!writeByte(out_buf, pos, hexDigitLower((b >> 4) & 0x0F))) return false;
            if (!writeByte(out_buf, pos, hexDigitLower(b & 0x0F))) return false;
        } else {
            if (!writeByte(out_buf, pos, b)) return false;
        }
    }
    return writeByte(out_buf, pos, '"');
}

// ============================================================================
// 1. Core HTTP Primitives: AppMethod, HeaderKV, ParamKV, Request, Response
// ============================================================================

pub enum AppMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,

    pub fn asString(): string {
        match (_) {
            .GET => return "GET",
            .POST => return "POST",
            .PUT => return "PUT",
            .DELETE => return "DELETE",
            .PATCH => return "PATCH",
            .HEAD => return "HEAD",
            .OPTIONS => return "OPTIONS",
        }
    }

    pub fn asLowerString(): string {
        match (_) {
            .GET => return "get",
            .POST => return "post",
            .PUT => return "put",
            .DELETE => return "delete",
            .PATCH => return "patch",
            .HEAD => return "head",
            .OPTIONS => return "options",
        }
    }

    pub fn bitMask(): u8 {
        match (_) {
            .GET => return 1,
            .POST => return 2,
            .PUT => return 4,
            .DELETE => return 8,
            .PATCH => return 16,
            .HEAD => return 32,
            .OPTIONS => return 64,
        }
    }

    pub fn methodIndex(): usize {
        match (_) {
            .GET => return 0,
            .POST => return 1,
            .PUT => return 2,
            .DELETE => return 3,
            .PATCH => return 4,
            .HEAD => return 5,
            .OPTIONS => return 6,
        }
    }

    pub static fn fromString(s: string): AppMethod? {
        if (asciiEqlIgnoreCase(s, "GET")) return AppMethod.GET;
        if (asciiEqlIgnoreCase(s, "POST")) return AppMethod.POST;
        if (asciiEqlIgnoreCase(s, "PUT")) return AppMethod.PUT;
        if (asciiEqlIgnoreCase(s, "DELETE")) return AppMethod.DELETE;
        if (asciiEqlIgnoreCase(s, "PATCH")) return AppMethod.PATCH;
        if (asciiEqlIgnoreCase(s, "HEAD")) return AppMethod.HEAD;
        if (asciiEqlIgnoreCase(s, "OPTIONS")) return AppMethod.OPTIONS;
        return null;
    }
}

pub struct HeaderKV {
    name: string = "";
    value: string = "";
}

pub struct ParamKV {
    name: string = "";
    value: string = "";
}

pub struct Request {
    method: AppMethod = AppMethod.GET;
    path: string = "/";
    raw_query: string = "";
    body: string = "";
    headers: [APP_MAX_HEADERS]HeaderKV = undefined;
    header_len: usize = 0;

    /// Construct a request from method and target (automatically splits `?query`).
    pub static fn init(method: AppMethod, target: string): Request {
        let path = target;
        let query: string = "";
        if (q_idx := findByte(target, '?')) {
            path = target[0..q_idx];
            if (q_idx + 1 < target.len) {
                query = target[q_idx + 1 ..];
            }
        }
        return Request{
            method: method,
            path: if (path.len == 0) "/" else path,
            raw_query: query,
            body: "",
            headers: undefined,
            header_len: 0,
        };
    }

    pub fn withHeader(*_, name: string, value: string): *Request {
        if (_.header_len < APP_MAX_HEADERS) {
            _.headers[_.header_len] = HeaderKV{ name: name, value: value };
            _.header_len += 1;
        }
        return _;
    }

    pub fn withBody(*_, body: string): *Request {
        _.body = body;
        return _;
    }

    pub fn header(name: string): string? {
        let i: usize = 0;
        while (i < _.header_len) : (i += 1) {
            if (asciiEqlIgnoreCase(_.headers[i].name, name)) {
                return _.headers[i].value;
            }
        }
        return null;
    }
}

pub struct Response {
    status: u16 = 200;
    content_type: string = "text/plain; charset=utf-8";
    body: string = "";
    body_buf: [128]u8 = undefined;
    body_buf_len: usize = 0;
    trace_buf: [128]u8 = undefined;
    trace_buf_len: usize = 0;
    headers: [APP_MAX_HEADERS]HeaderKV = undefined;
    header_len: usize = 0;

    pub static fn textConcat3(status_code: u16, a: string, sep: string, b: string): Response {
        let r = Response{
            status: status_code,
            content_type: "text/plain; charset=utf-8",
            body: "",
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
        let pos: usize = 0;
        * = writeRaw(r.body_buf[0..], *pos, a);
        * = writeRaw(r.body_buf[0..], *pos, sep);
        * = writeRaw(r.body_buf[0..], *pos, b);
        r.body_buf_len = pos;
        return r;
    }

    pub static fn json(status_code: u16, payload: string): Response {
        return Response{
            status: status_code,
            content_type: "application/json; charset=utf-8",
            body: payload,
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
    }

    pub static fn text(status_code: u16, payload: string): Response {
        return Response{
            status: status_code,
            content_type: "text/plain; charset=utf-8",
            body: payload,
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
    }

    pub static fn noContent(): Response {
        return Response{
            status: 204,
            content_type: "",
            body: "",
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
    }

    pub static fn statusCode(code: u16): Response {
        return Response{
            status: code,
            content_type: "text/plain; charset=utf-8",
            body: "",
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
    }

    pub static fn redirect(status_code: u16, location: string): Response {
        let r = Response{
            status: status_code,
            content_type: "",
            body: "",
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
        * = r.setHeader("Location", location);
        return r;
    }

    pub static fn redirectTemporary(location: string): Response {
        return Response.redirect(302, location);
    }

    pub static fn redirectPermanent(location: string): Response {
        return Response.redirect(308, location);
    }

    pub static fn html(status_code: u16, body: string): Response {
        return Response{
            status: status_code,
            content_type: "text/html; charset=utf-8",
            body: body,
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
    }

    pub static fn bytes(status_code: u16, payload: string, content_type: string): Response {
        return Response{
            status: status_code,
            content_type: content_type,
            body: payload,
            body_buf: undefined,
            body_buf_len: 0,
            headers: undefined,
            header_len: 0,
        };
    }

    pub fn setHeader(*_, name: string, value: string): *Response {
        let i: usize = 0;
        while (i < _.header_len) : (i += 1) {
            if (asciiEqlIgnoreCase(_.headers[i].name, name)) {
                _.headers[i].value = value;
                return _;
            }
        }
        if (_.header_len < APP_MAX_HEADERS) {
            _.headers[_.header_len] = HeaderKV{ name: name, value: value };
            _.header_len += 1;
        }
        return _;
    }

    pub fn header(name: string): string? {
        if (_.trace_buf_len > 0 && asciiEqlIgnoreCase(name, "Server-Timing")) {
            return _.trace_buf[0.._.trace_buf_len];
        }
        let i: usize = 0;
        while (i < _.header_len) : (i += 1) {
            if (asciiEqlIgnoreCase(_.headers[i].name, name)) {
                return _.headers[i].value;
            }
        }
        return null;
    }
}

/// Monotonic nanosecond clock (zero-alloc vDSO on Linux, POSIX clock_gettime elsewhere).
fn appMonotonicNowNs(): u64 {
    const app_builtin = @import("builtin");
    if (comptime app_builtin.os.tag == .linux) {
        let ts: std.os.linux.timespec = undefined;
        * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        const sec: u64 = if (ts.sec > 0) @intCast(ts.sec) else 0;
        const nsec: u64 = if (ts.nsec > 0) @intCast(ts.nsec) else 0;
        return sec * std.time.ns_per_s + nsec;
    }
    let ts: std.c.timespec = undefined;
    * = std.c.clock_gettime(.MONOTONIC, &ts);
    const sec: u64 = if (ts.sec > 0) @intCast(ts.sec) else 0;
    const nsec: u64 = if (ts.nsec > 0) @intCast(ts.nsec) else 0;
    return sec * std.time.ns_per_s + nsec;
}

fn writeU64Decimal(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        return writeByte(out_buf, pos, '0');
    }
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        if (!writeByte(out_buf, pos, tmp[n])) return false;
    }
    return true;
}

fn writeDurationMs(out_buf: []u8, pos: *usize, dur_ns: u64): bool {
    const us: u64 = dur_ns / 1000;
    const ms_int: u64 = us / 1000;
    const ms_frac: u64 = us % 1000;
    if (!writeU64Decimal(out_buf, pos, ms_int)) return false;
    if (!writeByte(out_buf, pos, '.')) return false;
    if (!writeByte(out_buf, pos, ('0' + ((ms_frac / 100) as u8)) as u8)) return false;
    if (!writeByte(out_buf, pos, ('0' + (((ms_frac / 10) % 10) as u8)) as u8)) return false;
    return writeByte(out_buf, pos, ('0' + ((ms_frac % 10) as u8)) as u8);
}

fn attachServerTiming(out_resp: *Response, routing_ns: u64, handler_ns: u64, hooks_ns: u64) {
    let pos: usize = 0;
    * = writeRaw(out_resp.trace_buf[0..], *pos, "routing;dur=");
    * = writeDurationMs(out_resp.trace_buf[0..], *pos, routing_ns);
    * = writeRaw(out_resp.trace_buf[0..], *pos, ", handler;dur=");
    * = writeDurationMs(out_resp.trace_buf[0..], *pos, handler_ns);
    if (hooks_ns > 0) {
        * = writeRaw(out_resp.trace_buf[0..], *pos, ", hooks;dur=");
        * = writeDurationMs(out_resp.trace_buf[0..], *pos, hooks_ns);
    }
    out_resp.trace_buf_len = pos;
    * = out_resp.setHeader("Server-Timing", out_resp.trace_buf[0..pos]);
}

// ============================================================================
// 2. Axum-Style Zero-Reflection Context & Extractors
// ============================================================================

pub struct Context {
    method: AppMethod = AppMethod.GET;
    path: string = "/";
    original_uri: string = "/";
    matched_path: string = "";
    raw_query: string = "";
    body: string = "";
    headers: [APP_MAX_HEADERS]HeaderKV = undefined;
    header_len: usize = 0;
    params: [APP_MAX_PARAMS]ParamKV = undefined;
    param_len: usize = 0;
    state_ptr: usize = 0;
    state_kv: [APP_MAX_STATE_KV]ParamKV = undefined;
    state_kv_len: usize = 0;
    derived_kv: [APP_MAX_DERIVED_KV]ParamKV = undefined;
    derived_kv_len: usize = 0;
    response_headers: [APP_MAX_HEADERS]HeaderKV = undefined;
    response_header_len: usize = 0;
    aborted: bool = false;
    abort_response: Response = undefined;
    error_code: AppErrorCode = AppErrorCode.None;
    error_message: string = "";

    /// Extract a dynamic path parameter (`:id`, `{id}`, or `*wildcard`).
    pub fn param(name: string): string? {
        let i: usize = 0;
        while (i < _.param_len) : (i += 1) {
            if (bytesEql(_.params[i].name, name)) {
                return _.params[i].value;
            }
        }
        return null;
    }

    /// Extract and parse an unsigned 64-bit integer path parameter.
    pub fn paramU64(name: string): u64? {
        if (raw := _.param(name)) {
            return parseDecimalU64(raw);
        }
        return null;
    }

    /// Extract and parse a signed 64-bit integer path parameter (Axum `Path<i64>`).
    pub fn paramI64(name: string): i64? {
        if (raw := _.param(name)) {
            return parseDecimalI64(raw);
        }
        return null;
    }

    /// Axum `MatchedPath` extractor: returns the matched route pattern (e.g. `/users/:id`).
    pub fn matchedPath(): string {
        return _.matched_path;
    }

    /// Axum `OriginalUri` extractor: returns the original unsplit request target.
    pub fn originalUri(): string {
        return _.original_uri;
    }

    /// Extract a query parameter from `raw_query` (zero-allocation linear scan).
    pub fn query(key: string): string? {
        if (key.len == 0 || _.raw_query.len == 0) return null;
        let pos: usize = 0;
        const q = _.raw_query;
        while (pos <= q.len) {
            let end: usize = pos;
            while (end < q.len && q[end] != '&') : (end += 1) {}
            const pair = q[pos..end];
            if (pair.len > 0) {
                if (eq_idx := findByte(pair, '=')) {
                    const k = pair[0..eq_idx];
                    if (bytesEql(k, key)) {
                        return if (eq_idx + 1 <= pair.len) pair[eq_idx + 1 ..] else "";
                    }
                } else if (bytesEql(pair, key)) {
                    return "";
                }
            }
            if (end >= q.len) break;
            pos = end + 1;
        }
        return null;
    }

    /// Extract and parse an unsigned 64-bit integer query parameter.
    pub fn queryU64(key: string): u64? {
        if (raw := _.query(key)) {
            return parseDecimalU64(raw);
        }
        return null;
    }

    /// Extract and parse a signed 64-bit integer query parameter.
    pub fn queryI64(key: string): i64? {
        if (raw := _.query(key)) {
            return parseDecimalI64(raw);
        }
        return null;
    }

    /// Extract and parse a boolean query parameter (`true`/`1`/`yes` vs `false`/`0`/`no`).
    pub fn queryBool(key: string): bool? {
        if (raw := _.query(key)) {
            if (asciiEqlIgnoreCase(raw, "true") || bytesEql(raw, "1") || asciiEqlIgnoreCase(raw, "yes")) {
                return true;
            }
            if (asciiEqlIgnoreCase(raw, "false") || bytesEql(raw, "0") || asciiEqlIgnoreCase(raw, "no")) {
                return false;
            }
        }
        return null;
    }

    /// Extract an HTTP request header (ASCII case-insensitive).
    pub fn header(name: string): string? {
        let i: usize = 0;
        while (i < _.header_len) : (i += 1) {
            if (asciiEqlIgnoreCase(_.headers[i].name, name)) {
                return _.headers[i].value;
            }
        }
        return null;
    }

    /// Extract Bearer token from `Authorization: Bearer <token>`.
    pub fn bearerToken(): string? {
        if (auth := _.header("Authorization")) {
            if (auth.len > 7 && asciiEqlIgnoreCase(auth[0..7], "Bearer ")) {
                return trimAsciiSpaces(auth[7..]);
            }
        }
        return null;
    }

    /// Extract an RFC 6265 Cookie value from the `Cookie` request header (zero heap).
    pub fn cookie(name: string): string? {
        if (name.len == 0) return null;
        if (raw := _.header("Cookie")) {
            let pos: usize = 0;
            while (pos < raw.len) {
                let end: usize = pos;
                while (end < raw.len && raw[end] != ';') : (end += 1) {}
                const pair = trimAsciiSpaces(raw[pos..end]);
                if (pair.len > 0) {
                    if (eq_idx := findByte(pair, '=')) {
                        const k = trimAsciiSpaces(pair[0..eq_idx]);
                        if (bytesEql(k, name)) {
                            let val = trimAsciiSpaces(pair[eq_idx + 1 ..]);
                            if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                                val = val[1 .. val.len - 1];
                            }
                            return val;
                        }
                    }
                }
                if (end >= raw.len) break;
                pos = end + 1;
            }
        }
        return null;
    }

    /// Access the application state pointer address injected via `app.state(ptr)`.
    pub fn stateAddr(): usize {
        return _.state_ptr;
    }

    /// Access named application state value injected via `app.setState(key, value)`.
    pub fn stateVal(key: string): string? {
        let i: usize = 0;
        while (i < _.state_kv_len) : (i += 1) {
            if (bytesEql(_.state_kv[i].name, key)) {
                return _.state_kv[i].value;
            }
        }
        return null;
    }

    /// Inject a per-request derived property (used inside `app.derive(...)` hooks).
    pub fn setDerived(*_, key: string, value: string): *Context {
        let i: usize = 0;
        while (i < _.derived_kv_len) : (i += 1) {
            if (bytesEql(_.derived_kv[i].name, key)) {
                _.derived_kv[i].value = value;
                return _;
            }
        }
        if (_.derived_kv_len < APP_MAX_DERIVED_KV) {
            _.derived_kv[_.derived_kv_len] = ParamKV{ name: key, value: value };
            _.derived_kv_len += 1;
        }
        return _;
    }

    /// Read a per-request derived property set by `app.derive(...)`.
    pub fn derived(key: string): string? {
        let i: usize = 0;
        while (i < _.derived_kv_len) : (i += 1) {
            if (bytesEql(_.derived_kv[i].name, key)) {
                return _.derived_kv[i].value;
            }
        }
        return null;
    }

    /// Attach a response header (e.g., CORS / X-Request-Id / Set-Cookie) merged into the final response.
    pub fn setHeader(*_, name: string, value: string): *Context {
        let i: usize = 0;
        while (i < _.response_header_len) : (i += 1) {
            if (asciiEqlIgnoreCase(_.response_headers[i].name, name)
                && !asciiEqlIgnoreCase(name, "Set-Cookie"))
            {
                _.response_headers[i].value = value;
                return _;
            }
        }
        if (_.response_header_len < APP_MAX_HEADERS) {
            _.response_headers[_.response_header_len] = HeaderKV{ name: name, value: value };
            _.response_header_len += 1;
        }
        return _;
    }

    /// Short-circuit middleware execution and return `resp` immediately.
    pub fn abort(*_, resp: Response) {
        _.aborted = true;
        _.abort_response = resp;
    }

    /// Trigger the unified Elysia `onError` pipeline with a structured `AppErrorCode`.
    pub fn fail(*_, code: AppErrorCode, message: string) {
        _.error_code = code;
        _.error_message = message;
        _.aborted = true;
    }

    /// Extract a form field from `application/x-www-form-urlencoded` body (`k1=v1&k2=v2`, zero-alloc slice).
    pub fn form(key: string): string? {
        if (key.len == 0 || _.body.len == 0) return null;
        let pos: usize = 0;
        const b = _.body;
        while (pos <= b.len) {
            let end: usize = pos;
            while (end < b.len && b[end] != '&') : (end += 1) {}
            const pair = b[pos..end];
            if (pair.len > 0) {
                if (eq_idx := findByte(pair, '=')) {
                    const k = pair[0..eq_idx];
                    if (bytesEql(k, key)) {
                        return if (eq_idx + 1 <= pair.len) pair[eq_idx + 1 ..] else "";
                    }
                } else if (bytesEql(pair, key)) {
                    return "";
                }
            }
            if (end >= b.len) break;
            pos = end + 1;
        }
        return null;
    }

    /// Extract the boundary parameter from a `Content-Type: multipart/...; boundary=...` header.
    pub fn multipartBoundary(): string? {
        if (ct := _.header("Content-Type")) {
            if (semi_idx := findByte(ct, ';')) {
                const media = trimAsciiSpaces(ct[0..semi_idx]);
                if (media.len < 10 || !asciiEqlIgnoreCase(media[0..10], "multipart/")) {
                    return null;
                }
                let pos: usize = semi_idx + 1;
                while (pos < ct.len) {
                    let end: usize = pos;
                    while (end < ct.len && ct[end] != ';') : (end += 1) {}
                    const part = trimAsciiSpaces(ct[pos..end]);
                    if (part.len > 0) {
                        if (eq_idx := findByte(part, '=')) {
                            const k = trimAsciiSpaces(part[0..eq_idx]);
                            if (asciiEqlIgnoreCase(k, "boundary")) {
                                let val = trimAsciiSpaces(part[eq_idx + 1 ..]);
                                if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                                    val = val[1 .. val.len - 1];
                                }
                                return if (val.len > 0) val else null;
                            }
                        }
                    }
                    if (end >= ct.len) break;
                    pos = end + 1;
                }
            }
        }
        return null;
    }

    /// Check whether this request has a valid `multipart/*` Content-Type with a boundary.
    pub fn isMultipart(): bool {
        return _.multipartBoundary() != null;
    }

    /// Build a JSON response (`application/json; charset=utf-8`).
    pub fn json(status_code: u16, payload: string): Response {
        return Response.json(status_code, payload);
    }

    /// Build a plain-text response (`text/plain; charset=utf-8`).
    pub fn text(status_code: u16, payload: string): Response {
        return Response.text(status_code, payload);
    }

    /// Build an HTML response (`text/html; charset=utf-8`).
    pub fn html(status_code: u16, body: string): Response {
        * = _;
        return Response.html(status_code, body);
    }

    /// Build a binary/custom content-type response.
    pub fn bytes(status_code: u16, payload: string, content_type: string): Response {
        * = _;
        return Response.bytes(status_code, payload, content_type);
    }

    /// Build a redirect response with `Location` header.
    pub fn redirect(status_code: u16, location: string): Response {
        * = _;
        return Response.redirect(status_code, location);
    }

    /// Build a `302 Found` temporary redirect response.
    pub fn redirectTemporary(location: string): Response {
        * = _;
        return Response.redirectTemporary(location);
    }

    /// Build a `308 Permanent Redirect` response.
    pub fn redirectPermanent(location: string): Response {
        * = _;
        return Response.redirectPermanent(location);
    }

    /// Build a `204 No Content` response.
    pub fn noContent(): Response {
        return Response.noContent();
    }

    /// Build an empty response with the specified HTTP status code.
    pub fn status(code: u16): Response {
        return Response.statusCode(code);
    }
}

// ============================================================================
// 3. Handler, Lifecycle Hooks, Plugin & Pre-Compiled Route Table
// ============================================================================

pub type HandlerFn = *const fn (*Context): Response;
pub type MiddlewareFn = *const fn (*Context): bool;
pub type LifecycleHookFn = *const fn (*Context): bool;
pub type ResponseHookFn = *const fn (*Context, *Response): void;
pub type BeforeHookFn = LifecycleHookFn;
pub type AfterHookFn = ResponseHookFn;
pub type ErrorHookFn = *const fn (*Context, AppErrorCode, string): Response;

pub const HOOK_MASK_MIDDLEWARE: u16 = 1 << 0;
pub const HOOK_MASK_ON_REQUEST: u16 = 1 << 1;
pub const HOOK_MASK_ON_PARSE: u16 = 1 << 2;
pub const HOOK_MASK_ON_TRANSFORM: u16 = 1 << 3;
pub const HOOK_MASK_DERIVE: u16 = 1 << 4;
pub const HOOK_MASK_ON_BEFORE: u16 = 1 << 5;
pub const HOOK_MASK_ON_AFTER: u16 = 1 << 6;
pub const HOOK_MASK_MAP_RESPONSE: u16 = 1 << 7;
pub const HOOK_MASK_ON_AFTER_RESPONSE: u16 = 1 << 8;
pub const HOOK_MASK_ON_ERROR: u16 = 1 << 9;
pub const HOOK_MASK_GLOBAL_HEADERS: u16 = 1 << 10;

pub enum RouteSegKind {
    Static,
    Param,
    Wildcard,
}

pub struct RouteSeg {
    kind: RouteSegKind = RouteSegKind.Static;
    start: u16 = 0;
    len: u16 = 0;
}

pub struct RouteEntry {
    method: AppMethod = AppMethod.GET;
    path_buf: [APP_MAX_PATH_LEN]u8 = [0; APP_MAX_PATH_LEN];
    path_len: usize = 0;
    summary: string = "";
    segs: [APP_MAX_SEGMENTS]RouteSeg = [RouteSeg{}; APP_MAX_SEGMENTS];
    seg_len: usize = 0;
    has_wildcard: bool = false;
    priority: u16 = 0;
    handler: HandlerFn? = null;
    before_hook: BeforeHookFn? = null;
    after_hook: AfterHookFn? = null;

    pub fn pathSlice(): string {
        return _.path_buf[0.._.path_len];
    }
}

fn normalizeAndJoinPath(prefix: string, path: string, out_buf: *[APP_MAX_PATH_LEN]u8): usize {
    let pos: usize = 0;

    // Append normalized prefix (skip trailing '/')
    if (prefix.len > 0) {
        let p_end: usize = prefix.len;
        while (p_end > 0 && prefix[p_end - 1] == '/') : (p_end -= 1) {}
        if (p_end > 0) {
            if (prefix[0] != '/' && pos < APP_MAX_PATH_LEN) {
                out_buf[pos] = '/';
                pos += 1;
            }
            let i: usize = 0;
            while (i < p_end && pos < APP_MAX_PATH_LEN) : (i += 1) {
                out_buf[pos] = prefix[i];
                pos += 1;
            }
        }
    }

    // Append path (ensure leading '/' and strip trailing '/' except root)
    let s_start: usize = 0;
    while (s_start < path.len && path[s_start] == '/') : (s_start += 1) {}
    let s_end: usize = path.len;
    while (s_end > s_start && path[s_end - 1] == '/') : (s_end -= 1) {}

    if (s_end > s_start) {
        if (pos < APP_MAX_PATH_LEN) {
            out_buf[pos] = '/';
            pos += 1;
        }
        let j: usize = s_start;
        while (j < s_end && pos < APP_MAX_PATH_LEN) : (j += 1) {
            out_buf[pos] = path[j];
            pos += 1;
        }
    } else if (pos == 0) {
        out_buf[0] = '/';
        pos = 1;
    }

    return pos;
}

fn compileRouteSegments(entry: *RouteEntry) {
    entry.seg_len = 0;
    entry.has_wildcard = false;
    entry.priority = 0;

    const p = entry.path_buf[0..entry.path_len];
    let i: usize = 0;
    while (i < p.len && entry.seg_len < APP_MAX_SEGMENTS) {
        while (i < p.len && p[i] == '/') : (i += 1) {}
        if (i >= p.len) break;

        let seg_start: usize = i;
        while (i < p.len && p[i] != '/') : (i += 1) {}
        const seg_end: usize = i;
        const first = p[seg_start];

        if (first == '*') {
            const name_start: usize = if (seg_end > seg_start + 1) seg_start + 1 else seg_start;
            const name_len: usize = seg_end - name_start;
            entry.segs[entry.seg_len] = RouteSeg{
                kind: RouteSegKind.Wildcard,
                start: name_start as u16,
                len: name_len as u16,
            };
            entry.seg_len += 1;
            entry.has_wildcard = true;
            entry.priority += 1;
            break;
        } else if (first == ':' && seg_end > seg_start + 1) {
            entry.segs[entry.seg_len] = RouteSeg{
                kind: RouteSegKind.Param,
                start: (seg_start + 1) as u16,
                len: (seg_end - seg_start - 1) as u16,
            };
            entry.seg_len += 1;
            entry.priority += 10;
        } else {
            entry.segs[entry.seg_len] = RouteSeg{
                kind: RouteSegKind.Static,
                start: seg_start as u16,
                len: (seg_end - seg_start) as u16,
            };
            entry.seg_len += 1;
            entry.priority += 100;
        }
    }
}

fn matchRouteEntry(
    entry: *RouteEntry,
    req_path: string,
    out_params: *[APP_MAX_PARAMS]ParamKV,
    out_param_len: *usize,
): bool {
    out_param_len.* = 0;
    const pat = entry.path_buf[0..entry.path_len];

    // Fast path: root `/`
    if (entry.seg_len == 0) {
        let k: usize = 0;
        while (k < req_path.len) : (k += 1) {
            if (req_path[k] != '/') return false;
        }
        return true;
    }

    let pos: usize = 0;
    let seg_idx: usize = 0;

    while (seg_idx < entry.seg_len) : (seg_idx += 1) {
        const seg = entry.segs[seg_idx];
        const seg_name = pat[(seg.start as usize) .. (seg.start as usize) + (seg.len as usize)];

        while (pos < req_path.len && req_path[pos] == '/') : (pos += 1) {}

        match (seg.kind) {
            .Wildcard => {
                // Capture remaining path (strip trailing slashes)
                let rem_end: usize = req_path.len;
                while (rem_end > pos && req_path[rem_end - 1] == '/') : (rem_end -= 1) {}
                if (out_param_len.* < APP_MAX_PARAMS) {
                    out_params[out_param_len.*] = ParamKV{
                        name: seg_name,
                        value: req_path[pos..rem_end],
                    };
                    out_param_len.* += 1;
                }
                return true;
            },
            .Static => {
                if (pos >= req_path.len) return false;
                let end: usize = pos;
                while (end < req_path.len && req_path[end] != '/') : (end += 1) {}
                if (!bytesEql(req_path[pos..end], seg_name)) return false;
                pos = end;
            },
            .Param => {
                if (pos >= req_path.len) return false;
                let end: usize = pos;
                while (end < req_path.len && req_path[end] != '/') : (end += 1) {}
                if (end == pos) return false;
                if (out_param_len.* < APP_MAX_PARAMS) {
                    out_params[out_param_len.*] = ParamKV{
                        name: seg_name,
                        value: req_path[pos..end],
                    };
                    out_param_len.* += 1;
                }
                pos = end;
            },
        }
    }

    // Ensure no trailing non-slash segments remain
    while (pos < req_path.len && req_path[pos] == '/') : (pos += 1) {}
    return pos == req_path.len;
}

fn mergeContextHeaders(ctx: *Context, resp: *Response) {
    let i: usize = 0;
    while (i < ctx.response_header_len) : (i += 1) {
        const hdr = ctx.response_headers[i];
        if (resp.header(hdr.name) == null) {
            * = resp.setHeader(hdr.name, hdr.value);
        }
    }
}

fn formatAllowHeaderValue(mask: u8): string {
    // Fast-path common single-method masks (zero allocation)
    if (mask == 1) return "GET";
    if (mask == 2) return "POST";
    if (mask == 4) return "PUT";
    if (mask == 8) return "DELETE";
    if (mask == 16) return "PATCH";
    if (mask == 3) return "GET, POST";
    if (mask == 5) return "GET, PUT";
    if (mask == 9) return "GET, DELETE";
    if (mask == 15) return "GET, POST, PUT, DELETE";
    if (mask == 31) return "GET, POST, PUT, DELETE, PATCH";
    return "GET, POST, PUT, DELETE, PATCH";
}

// ============================================================================
// 4. RouteGroup (Elysia-Style Scoped Prefix Builder)
// ============================================================================

pub struct RouteGroup {
    app: *App;
    prefix_buf: [APP_MAX_PATH_LEN]u8 = [0; APP_MAX_PATH_LEN];
    prefix_len: usize = 0;
    before_hook: BeforeHookFn? = null;
    after_hook: AfterHookFn? = null;

    /// Attach Elysia-style group-level guard hooks applied to all routes registered on this group.
    pub fn guard(*_, before_hook: BeforeHookFn?, after_hook: AfterHookFn?): *RouteGroup {
        _.before_hook = before_hook;
        _.after_hook = after_hook;
        return _;
    }

    pub fn get(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefixGuarded(AppMethod.GET, _.prefix_buf[0.._.prefix_len], path, "", handler, _.before_hook, _.after_hook);
        return _;
    }

    pub fn post(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefixGuarded(AppMethod.POST, _.prefix_buf[0.._.prefix_len], path, "", handler, _.before_hook, _.after_hook);
        return _;
    }

    pub fn put(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefixGuarded(AppMethod.PUT, _.prefix_buf[0.._.prefix_len], path, "", handler, _.before_hook, _.after_hook);
        return _;
    }

    pub fn delete(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefixGuarded(AppMethod.DELETE, _.prefix_buf[0.._.prefix_len], path, "", handler, _.before_hook, _.after_hook);
        return _;
    }

    pub fn patch(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefixGuarded(AppMethod.PATCH, _.prefix_buf[0.._.prefix_len], path, "", handler, _.before_hook, _.after_hook);
        return _;
    }

    pub fn group(*_, sub_prefix: string): RouteGroup {
        let g = RouteGroup{
            app: _.app,
            prefix_buf: [0; APP_MAX_PATH_LEN],
            prefix_len: 0,
            before_hook: _.before_hook,
            after_hook: _.after_hook,
        };
        g.prefix_len = normalizeAndJoinPath(_.prefix_buf[0.._.prefix_len], sub_prefix, *g.prefix_buf);
        return g;
    }
}

// ============================================================================
// 4b. Elysia-Style Composable Plugin Container
// ============================================================================

pub const PLUGIN_MAX_ROUTES: usize = 32;

pub struct PluginRouteDef {
    method: AppMethod = AppMethod.GET;
    path: string = "/";
    summary: string = "";
    handler: HandlerFn? = null;
}

pub struct Plugin {
    name: string = "";
    prefix: string = "";
    scope: PluginScope = PluginScope.Scoped;
    routes: [PLUGIN_MAX_ROUTES]PluginRouteDef = [PluginRouteDef{}; PLUGIN_MAX_ROUTES];
    route_len: usize = 0;
    on_request_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    on_request_len: usize = 0;
    derive_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    derive_len: usize = 0;
    on_before_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    on_before_len: usize = 0;
    on_after_hooks: [APP_MAX_HOOKS]ResponseHookFn? = [null; APP_MAX_HOOKS];
    on_after_len: usize = 0;
    state_kv: [APP_MAX_STATE_KV]ParamKV = [ParamKV{}; APP_MAX_STATE_KV];
    state_kv_len: usize = 0;

    pub static fn init(name: string): Plugin {
        return Plugin{ name: name };
    }

    pub fn withPrefix(*_, prefix: string): *Plugin {
        _.prefix = prefix;
        return _;
    }

    pub fn withScope(*_, scope: PluginScope): *Plugin {
        _.scope = scope;
        return _;
    }

    pub fn route(*_, method: AppMethod, path: string, summary: string, handler: HandlerFn): *Plugin {
        if (_.route_len < PLUGIN_MAX_ROUTES) {
            _.routes[_.route_len] = PluginRouteDef{
                method: method,
                path: path,
                summary: summary,
                handler: handler,
            };
            _.route_len += 1;
        }
        return _;
    }

    pub fn get(*_, path: string, handler: HandlerFn): *Plugin {
        return _.route(AppMethod.GET, path, "", handler);
    }

    pub fn post(*_, path: string, handler: HandlerFn): *Plugin {
        return _.route(AppMethod.POST, path, "", handler);
    }

    pub fn put(*_, path: string, handler: HandlerFn): *Plugin {
        return _.route(AppMethod.PUT, path, "", handler);
    }

    pub fn delete(*_, path: string, handler: HandlerFn): *Plugin {
        return _.route(AppMethod.DELETE, path, "", handler);
    }

    pub fn patch(*_, path: string, handler: HandlerFn): *Plugin {
        return _.route(AppMethod.PATCH, path, "", handler);
    }

    pub fn onRequest(*_, hook: LifecycleHookFn): *Plugin {
        if (_.on_request_len < APP_MAX_HOOKS) {
            _.on_request_hooks[_.on_request_len] = hook;
            _.on_request_len += 1;
        }
        return _;
    }

    pub fn derive(*_, hook: LifecycleHookFn): *Plugin {
        if (_.derive_len < APP_MAX_HOOKS) {
            _.derive_hooks[_.derive_len] = hook;
            _.derive_len += 1;
        }
        return _;
    }

    pub fn onBeforeHandle(*_, hook: LifecycleHookFn): *Plugin {
        if (_.on_before_len < APP_MAX_HOOKS) {
            _.on_before_hooks[_.on_before_len] = hook;
            _.on_before_len += 1;
        }
        return _;
    }

    pub fn onAfterHandle(*_, hook: ResponseHookFn): *Plugin {
        if (_.on_after_len < APP_MAX_HOOKS) {
            _.on_after_hooks[_.on_after_len] = hook;
            _.on_after_len += 1;
        }
        return _;
    }

    pub fn setState(*_, key: string, value: string): *Plugin {
        if (_.state_kv_len < APP_MAX_STATE_KV) {
            _.state_kv[_.state_kv_len] = ParamKV{ name: key, value: value };
            _.state_kv_len += 1;
        }
        return _;
    }
}

// ============================================================================
// 5. Flagship App Kernel (Radix Router + 8-Stage Lifecycle + Plugins + OpenAPI 3.1)
// ============================================================================

pub const HOOK_MASK_TRACE: u16 = 1 << 11;

pub struct App {
    routes: [APP_MAX_ROUTES]RouteEntry = [RouteEntry{}; APP_MAX_ROUTES];
    route_len: usize = 0;
    radix: RadixRouter = RadixRouter.init();
    middlewares: [APP_MAX_MIDDLEWARES]MiddlewareFn? = [null; APP_MAX_MIDDLEWARES];
    middleware_len: usize = 0;

    // Elysia 8-Stage Lifecycle Hooks
    on_request_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    on_request_len: usize = 0;
    on_parse_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    on_parse_len: usize = 0;
    on_transform_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    on_transform_len: usize = 0;
    derive_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    derive_len: usize = 0;
    on_before_hooks: [APP_MAX_HOOKS]LifecycleHookFn? = [null; APP_MAX_HOOKS];
    on_before_len: usize = 0;
    on_after_hooks: [APP_MAX_HOOKS]ResponseHookFn? = [null; APP_MAX_HOOKS];
    on_after_len: usize = 0;
    map_response_hooks: [APP_MAX_HOOKS]ResponseHookFn? = [null; APP_MAX_HOOKS];
    map_response_len: usize = 0;
    on_after_response_hooks: [APP_MAX_HOOKS]ResponseHookFn? = [null; APP_MAX_HOOKS];
    on_after_response_len: usize = 0;
    on_error_hook: ErrorHookFn? = null;
    fallback_handler: HandlerFn? = null;
    hook_mask: u16 = 0;
    trace_enabled: bool = false;

    state_ptr: usize = 0;
    state_kv: [APP_MAX_STATE_KV]ParamKV = [ParamKV{}; APP_MAX_STATE_KV];
    state_kv_len: usize = 0;

    pub static fn init(): App {
        return App{};
    }

    pub fn hookMask(*_): u16 {
        return _.hook_mask;
    }

    /// Enable or disable 8-stage `Server-Timing` tracing (zero overhead on fast-path when disabled).
    pub fn enableTrace(*_, enabled: bool): *App {
        _.trace_enabled = enabled;
        if (enabled) {
            _.hook_mask |= HOOK_MASK_TRACE;
        } else {
            _.hook_mask &= (0xFFFF - HOOK_MASK_TRACE);
        }
        return _;
    }

    pub fn registerWithPrefixGuarded(
        *_,
        method: AppMethod,
        prefix: string,
        path: string,
        summary: string,
        handler: HandlerFn,
        before_hook: BeforeHookFn?,
        after_hook: AfterHookFn?,
    ) {
        if (_.route_len >= APP_MAX_ROUTES) return;
        let entry = RouteEntry{
            method: method,
            path_buf: [0; APP_MAX_PATH_LEN],
            path_len: 0,
            summary: summary,
            segs: [RouteSeg{}; APP_MAX_SEGMENTS],
            seg_len: 0,
            has_wildcard: false,
            priority: 0,
            handler: handler,
            before_hook: before_hook,
            after_hook: after_hook,
        };
        entry.path_len = normalizeAndJoinPath(prefix, path, *entry.path_buf);
        compileRouteSegments(*entry);
        const route_idx = _.route_len;
        _.routes[route_idx] = entry;
        _.route_len += 1;
        * = _.radix.insert(method.methodIndex(), _.routes[route_idx].pathSlice(), route_idx as i32);
    }

    pub fn registerWithPrefix(
        *_,
        method: AppMethod,
        prefix: string,
        path: string,
        summary: string,
        handler: HandlerFn,
    ) {
        _.registerWithPrefixGuarded(method, prefix, path, summary, handler, null, null);
    }

    pub fn routeWithGuard(
        *_,
        method: AppMethod,
        path: string,
        handler: HandlerFn,
        before_hook: BeforeHookFn?,
        after_hook: AfterHookFn?,
    ): *App {
        _.registerWithPrefixGuarded(method, "", path, "", handler, before_hook, after_hook);
        return _;
    }

    pub fn getWithGuard(*_, path: string, handler: HandlerFn, before_hook: BeforeHookFn?, after_hook: AfterHookFn?): *App {
        _.registerWithPrefixGuarded(AppMethod.GET, "", path, "", handler, before_hook, after_hook);
        return _;
    }

    pub fn postWithGuard(*_, path: string, handler: HandlerFn, before_hook: BeforeHookFn?, after_hook: AfterHookFn?): *App {
        _.registerWithPrefixGuarded(AppMethod.POST, "", path, "", handler, before_hook, after_hook);
        return _;
    }

    pub fn putWithGuard(*_, path: string, handler: HandlerFn, before_hook: BeforeHookFn?, after_hook: AfterHookFn?): *App {
        _.registerWithPrefixGuarded(AppMethod.PUT, "", path, "", handler, before_hook, after_hook);
        return _;
    }

    pub fn deleteWithGuard(*_, path: string, handler: HandlerFn, before_hook: BeforeHookFn?, after_hook: AfterHookFn?): *App {
        _.registerWithPrefixGuarded(AppMethod.DELETE, "", path, "", handler, before_hook, after_hook);
        return _;
    }

    pub fn patchWithGuard(*_, path: string, handler: HandlerFn, before_hook: BeforeHookFn?, after_hook: AfterHookFn?): *App {
        _.registerWithPrefixGuarded(AppMethod.PATCH, "", path, "", handler, before_hook, after_hook);
        return _;
    }

    /// Axum `Router::route_layer`: attach route-level before/after hooks to the most recently registered route.
    pub fn routeLayer(*_, before_hook: BeforeHookFn?, after_hook: AfterHookFn?): *App {
        if (_.route_len > 0) {
            const idx = _.route_len - 1;
            if (before_hook != null) {
                _.routes[idx].before_hook = before_hook;
            }
            if (after_hook != null) {
                _.routes[idx].after_hook = after_hook;
            }
        }
        return _;
    }

    pub fn route(*_, method: AppMethod, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(method, "", path, "", handler);
        return _;
    }

    pub fn routeDoc(*_, method: AppMethod, path: string, summary: string, handler: HandlerFn): *App {
        _.registerWithPrefix(method, "", path, summary, handler);
        return _;
    }

    pub fn get(*_, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(AppMethod.GET, "", path, "", handler);
        return _;
    }

    pub fn post(*_, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(AppMethod.POST, "", path, "", handler);
        return _;
    }

    pub fn put(*_, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(AppMethod.PUT, "", path, "", handler);
        return _;
    }

    pub fn delete(*_, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(AppMethod.DELETE, "", path, "", handler);
        return _;
    }

    pub fn patch(*_, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(AppMethod.PATCH, "", path, "", handler);
        return _;
    }

    pub fn head(*_, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(AppMethod.HEAD, "", path, "", handler);
        return _;
    }

    pub fn options(*_, path: string, handler: HandlerFn): *App {
        _.registerWithPrefix(AppMethod.OPTIONS, "", path, "", handler);
        return _;
    }

    pub fn use(*_, middleware: MiddlewareFn): *App {
        if (_.middleware_len < APP_MAX_MIDDLEWARES) {
            _.middlewares[_.middleware_len] = middleware;
            _.middleware_len += 1;
            _.hook_mask |= HOOK_MASK_MIDDLEWARE;
        }
        return _;
    }

    // --- Elysia 8-Stage Lifecycle Hook Registration ---

    pub fn onRequest(*_, hook: LifecycleHookFn): *App {
        if (_.on_request_len < APP_MAX_HOOKS) {
            _.on_request_hooks[_.on_request_len] = hook;
            _.on_request_len += 1;
            _.hook_mask |= HOOK_MASK_ON_REQUEST;
        }
        return _;
    }

    pub fn onParse(*_, hook: LifecycleHookFn): *App {
        if (_.on_parse_len < APP_MAX_HOOKS) {
            _.on_parse_hooks[_.on_parse_len] = hook;
            _.on_parse_len += 1;
            _.hook_mask |= HOOK_MASK_ON_PARSE;
        }
        return _;
    }

    pub fn onTransform(*_, hook: LifecycleHookFn): *App {
        if (_.on_transform_len < APP_MAX_HOOKS) {
            _.on_transform_hooks[_.on_transform_len] = hook;
            _.on_transform_len += 1;
            _.hook_mask |= HOOK_MASK_ON_TRANSFORM;
        }
        return _;
    }

    pub fn derive(*_, hook: LifecycleHookFn): *App {
        if (_.derive_len < APP_MAX_HOOKS) {
            _.derive_hooks[_.derive_len] = hook;
            _.derive_len += 1;
            _.hook_mask |= HOOK_MASK_DERIVE;
        }
        return _;
    }

    pub fn onBeforeHandle(*_, hook: LifecycleHookFn): *App {
        if (_.on_before_len < APP_MAX_HOOKS) {
            _.on_before_hooks[_.on_before_len] = hook;
            _.on_before_len += 1;
            _.hook_mask |= HOOK_MASK_ON_BEFORE;
        }
        return _;
    }

    pub fn onAfterHandle(*_, hook: ResponseHookFn): *App {
        if (_.on_after_len < APP_MAX_HOOKS) {
            _.on_after_hooks[_.on_after_len] = hook;
            _.on_after_len += 1;
            _.hook_mask |= HOOK_MASK_ON_AFTER;
        }
        return _;
    }

    pub fn mapResponse(*_, hook: ResponseHookFn): *App {
        if (_.map_response_len < APP_MAX_HOOKS) {
            _.map_response_hooks[_.map_response_len] = hook;
            _.map_response_len += 1;
            _.hook_mask |= HOOK_MASK_MAP_RESPONSE;
        }
        return _;
    }

    pub fn onAfterResponse(*_, hook: ResponseHookFn): *App {
        if (_.on_after_response_len < APP_MAX_HOOKS) {
            _.on_after_response_hooks[_.on_after_response_len] = hook;
            _.on_after_response_len += 1;
            _.hook_mask |= HOOK_MASK_ON_AFTER_RESPONSE;
        }
        return _;
    }

    pub fn onError(*_, hook: ErrorHookFn): *App {
        _.on_error_hook = hook;
        _.hook_mask |= HOOK_MASK_ON_ERROR;
        return _;
    }

    /// Axum `Router::fallback`: custom handler when no route matches.
    pub fn fallback(*_, handler: HandlerFn): *App {
        _.fallback_handler = handler;
        return _;
    }

    /// Mount an Elysia-style `Plugin` (routes, prefix, lifecycle hooks, derived props & state).
    pub fn usePlugin(*_, plugin: *const Plugin): *App {
        let i: usize = 0;
        while (i < plugin.route_len) : (i += 1) {
            const rd = plugin.routes[i];
            if (h := rd.handler) {
                _.registerWithPrefix(rd.method, plugin.prefix, rd.path, rd.summary, h);
            }
        }
        i = 0;
        while (i < plugin.on_request_len) : (i += 1) {
            if (h := plugin.on_request_hooks[i]) {
                * = _.onRequest(h);
            }
        }
        i = 0;
        while (i < plugin.derive_len) : (i += 1) {
            if (h := plugin.derive_hooks[i]) {
                * = _.derive(h);
            }
        }
        i = 0;
        while (i < plugin.on_before_len) : (i += 1) {
            if (h := plugin.on_before_hooks[i]) {
                * = _.onBeforeHandle(h);
            }
        }
        i = 0;
        while (i < plugin.on_after_len) : (i += 1) {
            if (h := plugin.on_after_hooks[i]) {
                * = _.onAfterHandle(h);
            }
        }
        i = 0;
        while (i < plugin.state_kv_len) : (i += 1) {
            * = _.setState(plugin.state_kv[i].name, plugin.state_kv[i].value);
        }
        return _;
    }

    /// Axum `Router::nest`: mount all routes of `sub` under `prefix`.
    pub fn nest(*_, prefix: string, sub: *const App): *App {
        let i: usize = 0;
        while (i < sub.route_len) : (i += 1) {
            const r = sub.routes[i];
            if (h := r.handler) {
                const sub_path = r.path_buf[0..r.path_len];
                _.registerWithPrefixGuarded(r.method, prefix, sub_path, r.summary, h, r.before_hook, r.after_hook);
            }
        }
        return _;
    }

    /// Axum `Router::merge`: merge all routes and named state from `other` into this App.
    pub fn merge(*_, other: *const App): *App {
        let i: usize = 0;
        while (i < other.route_len) : (i += 1) {
            const r = other.routes[i];
            if (h := r.handler) {
                const other_path = r.path_buf[0..r.path_len];
                _.registerWithPrefixGuarded(r.method, "", other_path, r.summary, h, r.before_hook, r.after_hook);
            }
        }
        i = 0;
        while (i < other.state_kv_len) : (i += 1) {
            * = _.setState(other.state_kv[i].name, other.state_kv[i].value);
        }
        return _;
    }

    pub fn group(*_, prefix: string): RouteGroup {
        let g = RouteGroup{
            app: _,
            prefix_buf: [0; APP_MAX_PATH_LEN],
            prefix_len: 0,
        };
        g.prefix_len = normalizeAndJoinPath("", prefix, *g.prefix_buf);
        return g;
    }

    pub fn state(*_, ptr: usize): *App {
        _.state_ptr = ptr;
        return _;
    }

    pub fn setState(*_, key: string, value: string): *App {
        let i: usize = 0;
        while (i < _.state_kv_len) : (i += 1) {
            if (bytesEql(_.state_kv[i].name, key)) {
                _.state_kv[i].value = value;
                return _;
            }
        }
        if (_.state_kv_len < APP_MAX_STATE_KV) {
            _.state_kv[_.state_kv_len] = ParamKV{ name: key, value: value };
            _.state_kv_len += 1;
        }
        return _;
    }

    pub fn routeCount(): usize {
        return _.route_len;
    }

    pub fn middlewareCount(): usize {
        return _.middleware_len;
    }

    fn finalizeResponse(*_, ctx: *Context, resp: *Response) {
        let i: usize = 0;
        while (i < _.on_after_len) : (i += 1) {
            if (h := _.on_after_hooks[i]) {
                h(ctx, resp);
            }
        }
        mergeContextHeaders(ctx, resp);
        i = 0;
        while (i < _.map_response_len) : (i += 1) {
            if (h := _.map_response_hooks[i]) {
                h(ctx, resp);
            }
        }
        i = 0;
        while (i < _.on_after_response_len) : (i += 1) {
            if (h := _.on_after_response_hooks[i]) {
                h(ctx, resp);
            }
        }
    }

    fn buildErrorResponse(*_, ctx: *Context, code: AppErrorCode, msg: string): Response {
        if (eh := _.on_error_hook) {
            let resp = eh(ctx, code, msg);
            _.finalizeResponse(ctx, *resp);
            return resp;
        }
        let resp = Response.json(code.defaultStatus(), msg);
        _.finalizeResponse(ctx, *resp);
        return resp;
    }

    fn runHookChain(
        *_,
        hooks: [APP_MAX_HOOKS]LifecycleHookFn?,
        hook_len: usize,
        ctx: *Context,
    ): Response? {
        let i: usize = 0;
        while (i < hook_len) : (i += 1) {
            if (h := hooks[i]) {
                const ok = h(ctx);
                if (!ok || ctx.aborted) {
                    if (ctx.error_code != AppErrorCode.None) {
                        return _.buildErrorResponse(ctx, ctx.error_code, ctx.error_message);
                    }
                    let resp = if (ctx.aborted)
                        ctx.abort_response
                    else
                        Response.json(403, "{\"error\":\"Forbidden\"}");
                    _.finalizeResponse(ctx, *resp);
                    return resp;
                }
            }
        }
        return null;
    }

    fn initContextForRequest(*_, req: *const Request, lookup_path: string, query_str: string, ctx: *Context) {
        ctx.method = req.method;
        ctx.path = lookup_path;
        ctx.original_uri = req.path;
        ctx.matched_path = "";
        ctx.raw_query = query_str;
        ctx.body = req.body;
        if (req.header_len > 0) {
            let hi: usize = 0;
            while (hi < req.header_len) : (hi += 1) {
                ctx.headers[hi] = req.headers[hi];
            }
        }
        ctx.header_len = req.header_len;
        ctx.param_len = 0;
        ctx.state_ptr = _.state_ptr;
        if (_.state_kv_len > 0) {
            let si: usize = 0;
            while (si < _.state_kv_len) : (si += 1) {
                ctx.state_kv[si] = _.state_kv[si];
            }
        }
        ctx.state_kv_len = _.state_kv_len;
        ctx.derived_kv_len = 0;
        ctx.response_header_len = 0;
        ctx.aborted = false;
        ctx.error_code = AppErrorCode.None;
        ctx.error_message = "";
    }

    /// Zero-copy in-place dispatcher for high-throughput servers (reuses caller's `ctx` and `out_resp`).
    pub fn dispatchInto(*_, req: *const Request, ctx: *Context, out_resp: *Response) {
        let clean_path = req.path;
        let query_str = req.raw_query;
        if (query_str.len == 0) {
            if (q_idx := findByte(req.path, '?')) {
                clean_path = req.path[0..q_idx];
                if (q_idx + 1 < req.path.len) {
                    query_str = req.path[q_idx + 1 ..];
                }
            }
        }
        const lookup_path = if (clean_path.len == 0) "/" else clean_path;

        // Zero-Hook Fast-Path: skip all 8-stage lifecycle checks when no global hooks are registered
        if (_.hook_mask == 0) {
            const rmatch = _.radix.lookup(req.method.methodIndex(), lookup_path);
            _.initContextForRequest(req, lookup_path, query_str, ctx);
            if (rmatch.matched && rmatch.handler_id >= 0) {
                const matched = rmatch.handler_id as usize;
                ctx.matched_path = _.routes[matched].pathSlice();
                if (rmatch.param_len > 0) {
                    let p_i: usize = 0;
                    while (p_i < rmatch.param_len && p_i < APP_MAX_PARAMS) : (p_i += 1) {
                        ctx.params[p_i] = ParamKV{
                            name: rmatch.params[p_i].name,
                            value: rmatch.params[p_i].value,
                        };
                    }
                    ctx.param_len = if (rmatch.param_len < APP_MAX_PARAMS) rmatch.param_len else APP_MAX_PARAMS;
                }

                // Route-level guard / route_layer before_hook (only if set on this route)
                if (bh := _.routes[matched].before_hook) {
                    const ok = bh(ctx);
                    if (!ok || ctx.aborted) {
                        if (ctx.error_code != AppErrorCode.None) {
                            out_resp.* = Response.json(ctx.error_code.defaultStatus(), ctx.error_message);
                        } else {
                            out_resp.* = if (ctx.aborted)
                                ctx.abort_response
                            else
                                Response.json(403, "{\"error\":\"Forbidden\"}");
                        }
                        if (ctx.response_header_len > 0) {
                            mergeContextHeaders(ctx, out_resp);
                        }
                        return;
                    }
                }

                if (h := _.routes[matched].handler) {
                    out_resp.* = h(ctx);
                    if (ctx.error_code != AppErrorCode.None) {
                        out_resp.* = Response.json(ctx.error_code.defaultStatus(), ctx.error_message);
                        if (ctx.response_header_len > 0) {
                            mergeContextHeaders(ctx, out_resp);
                        }
                        return;
                    }
                    if (rmatch.head_fallback_to_get) {
                        out_resp.body = "";
                        out_resp.body_buf_len = 0;
                    }
                    if (ah := _.routes[matched].after_hook) {
                        ah(ctx, out_resp);
                    }
                    if (ctx.response_header_len > 0) {
                        mergeContextHeaders(ctx, out_resp);
                    }
                    return;
                }
            }

            if (rmatch.allowed_mask != 0) {
                out_resp.* = Response.json(405, "{\"error\":\"Method Not Allowed\"}");
                * = out_resp.setHeader("Allow", formatAllowHeaderValue(rmatch.allowed_mask));
                return;
            }
            if (fb := _.fallback_handler) {
                out_resp.* = fb(ctx);
                if (ctx.response_header_len > 0) {
                    mergeContextHeaders(ctx, out_resp);
                }
                return;
            }
            out_resp.* = Response.json(404, "{\"error\":\"Not Found\"}");
            return;
        }

        // Full 8-Stage Lifecycle Path (when global hooks, middlewares, or trace are enabled)
        _.initContextForRequest(req, lookup_path, query_str, ctx);
        const trace_on = _.trace_enabled;
        let t_hooks_ns: u64 = 0;
        let t_routing_ns: u64 = 0;
        let t_handler_ns: u64 = 0;

        // Stage 1: onRequest hooks (zero-overhead skip when empty)
        if (_.on_request_len > 0) {
            const t_s1: u64 = if (trace_on) appMonotonicNowNs() else 0;
            if (early := _.runHookChain(_.on_request_hooks, _.on_request_len, ctx)) {
                if (trace_on) {
                    const t_e1 = appMonotonicNowNs();
                    t_hooks_ns += if (t_e1 >= t_s1) (t_e1 - t_s1) else 0;
                }
                out_resp.* = early;
                if (trace_on) attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                return;
            }
            if (trace_on) {
                const t_e1 = appMonotonicNowNs();
                t_hooks_ns += if (t_e1 >= t_s1) (t_e1 - t_s1) else 0;
            }
        }

        // Legacy & Onion middleware chain
        if (_.middleware_len > 0) {
            const t_mw0: u64 = if (trace_on) appMonotonicNowNs() else 0;
            let mw_idx: usize = 0;
            while (mw_idx < _.middleware_len) : (mw_idx += 1) {
                if (mw := _.middlewares[mw_idx]) {
                    const proceed = mw(ctx);
                    if (!proceed || ctx.aborted) {
                        if (ctx.error_code != AppErrorCode.None) {
                            out_resp.* = _.buildErrorResponse(ctx, ctx.error_code, ctx.error_message);
                        } else {
                            out_resp.* = if (ctx.aborted)
                                ctx.abort_response
                            else
                                Response.json(403, "{\"error\":\"Forbidden\"}");
                            _.finalizeResponse(ctx, out_resp);
                        }
                        if (trace_on) {
                            const t_mw1 = appMonotonicNowNs();
                            t_hooks_ns += if (t_mw1 >= t_mw0) (t_mw1 - t_mw0) else 0;
                            attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                        }
                        return;
                    }
                }
            }
            if (trace_on) {
                const t_mw1 = appMonotonicNowNs();
                t_hooks_ns += if (t_mw1 >= t_mw0) (t_mw1 - t_mw0) else 0;
            }
        }

        // Stage 2: O(1) Static Hash + Compressed Radix Trie Route Lookup
        const t_r0: u64 = if (trace_on) appMonotonicNowNs() else 0;
        const rmatch = _.radix.lookup(ctx.method.methodIndex(), ctx.path);
        if (trace_on) {
            const t_r1 = appMonotonicNowNs();
            t_routing_ns = if (t_r1 >= t_r0) (t_r1 - t_r0) else 0;
        }
        if (rmatch.matched && rmatch.handler_id >= 0) {
            const matched = rmatch.handler_id as usize;
            ctx.matched_path = _.routes[matched].pathSlice();
            let p_i: usize = 0;
            while (p_i < rmatch.param_len && p_i < APP_MAX_PARAMS) : (p_i += 1) {
                ctx.params[p_i] = ParamKV{
                    name: rmatch.params[p_i].name,
                    value: rmatch.params[p_i].value,
                };
            }
            ctx.param_len = if (rmatch.param_len < APP_MAX_PARAMS) rmatch.param_len else APP_MAX_PARAMS;

            // Stage 3: derive -> onParse -> onTransform -> onBeforeHandle -> route before_hook
            const t_s3_0: u64 = if (trace_on) appMonotonicNowNs() else 0;
            if (_.derive_len > 0) {
                if (early := _.runHookChain(_.derive_hooks, _.derive_len, ctx)) {
                    out_resp.* = early;
                    if (trace_on) {
                        const t_s3_1 = appMonotonicNowNs();
                        t_hooks_ns += if (t_s3_1 >= t_s3_0) (t_s3_1 - t_s3_0) else 0;
                        attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                    }
                    return;
                }
            }
            if (_.on_parse_len > 0) {
                if (early := _.runHookChain(_.on_parse_hooks, _.on_parse_len, ctx)) {
                    out_resp.* = early;
                    if (trace_on) {
                        const t_s3_1 = appMonotonicNowNs();
                        t_hooks_ns += if (t_s3_1 >= t_s3_0) (t_s3_1 - t_s3_0) else 0;
                        attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                    }
                    return;
                }
            }
            if (_.on_transform_len > 0) {
                if (early := _.runHookChain(_.on_transform_hooks, _.on_transform_len, ctx)) {
                    out_resp.* = early;
                    if (trace_on) {
                        const t_s3_1 = appMonotonicNowNs();
                        t_hooks_ns += if (t_s3_1 >= t_s3_0) (t_s3_1 - t_s3_0) else 0;
                        attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                    }
                    return;
                }
            }
            if (_.on_before_len > 0) {
                if (early := _.runHookChain(_.on_before_hooks, _.on_before_len, ctx)) {
                    out_resp.* = early;
                    if (trace_on) {
                        const t_s3_1 = appMonotonicNowNs();
                        t_hooks_ns += if (t_s3_1 >= t_s3_0) (t_s3_1 - t_s3_0) else 0;
                        attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                    }
                    return;
                }
            }
            if (bh := _.routes[matched].before_hook) {
                const ok = bh(ctx);
                if (!ok || ctx.aborted) {
                    if (ctx.error_code != AppErrorCode.None) {
                        out_resp.* = _.buildErrorResponse(ctx, ctx.error_code, ctx.error_message);
                    } else {
                        out_resp.* = if (ctx.aborted)
                            ctx.abort_response
                        else
                            Response.json(403, "{\"error\":\"Forbidden\"}");
                        _.finalizeResponse(ctx, out_resp);
                    }
                    if (trace_on) {
                        const t_s3_1 = appMonotonicNowNs();
                        t_hooks_ns += if (t_s3_1 >= t_s3_0) (t_s3_1 - t_s3_0) else 0;
                        attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                    }
                    return;
                }
            }
            if (trace_on && (_.derive_len > 0 || _.on_parse_len > 0 || _.on_transform_len > 0 || _.on_before_len > 0 || _.routes[matched].before_hook != null)) {
                const t_s3_1 = appMonotonicNowNs();
                t_hooks_ns += if (t_s3_1 >= t_s3_0) (t_s3_1 - t_s3_0) else 0;
            }

            // Stage 4: Handler execution
            if (h := _.routes[matched].handler) {
                const t_h0: u64 = if (trace_on) appMonotonicNowNs() else 0;
                out_resp.* = h(ctx);
                if (trace_on) {
                    const t_h1 = appMonotonicNowNs();
                    t_handler_ns = if (t_h1 >= t_h0) (t_h1 - t_h0) else 0;
                }
                if (ctx.error_code != AppErrorCode.None) {
                    out_resp.* = _.buildErrorResponse(ctx, ctx.error_code, ctx.error_message);
                    if (trace_on) attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                    return;
                }
                if (rmatch.head_fallback_to_get) {
                    out_resp.body = "";
                    out_resp.body_buf_len = 0;
                }
                const t_af0: u64 = if (trace_on) appMonotonicNowNs() else 0;
                if (ah := _.routes[matched].after_hook) {
                    ah(ctx, out_resp);
                }
                if (_.on_after_len > 0 || ctx.response_header_len > 0 || _.map_response_len > 0 || _.on_after_response_len > 0) {
                    _.finalizeResponse(ctx, out_resp);
                }
                if (trace_on) {
                    if (_.routes[matched].after_hook != null || _.on_after_len > 0 || _.map_response_len > 0 || _.on_after_response_len > 0) {
                        const t_af1 = appMonotonicNowNs();
                        t_hooks_ns += if (t_af1 >= t_af0) (t_af1 - t_af0) else 0;
                    }
                    attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
                }
                return;
            }
        }

        // Stage 5: 405 Method Not Allowed vs Fallback / 404 Not Found
        if (rmatch.allowed_mask != 0) {
            out_resp.* = Response.json(405, "{\"error\":\"Method Not Allowed\"}");
            if (eh := _.on_error_hook) {
                out_resp.* = eh(ctx, AppErrorCode.MethodNotAllowed, "{\"error\":\"Method Not Allowed\"}");
            }
            * = out_resp.setHeader("Allow", formatAllowHeaderValue(rmatch.allowed_mask));
            _.finalizeResponse(ctx, out_resp);
            if (trace_on) attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
            return;
        }

        if (fb := _.fallback_handler) {
            const t_fb0: u64 = if (trace_on) appMonotonicNowNs() else 0;
            out_resp.* = fb(ctx);
            if (trace_on) {
                const t_fb1 = appMonotonicNowNs();
                t_handler_ns = if (t_fb1 >= t_fb0) (t_fb1 - t_fb0) else 0;
            }
            _.finalizeResponse(ctx, out_resp);
            if (trace_on) attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
            return;
        }

        out_resp.* = _.buildErrorResponse(ctx, AppErrorCode.NotFound, "{\"error\":\"Not Found\"}");
        if (trace_on) attachServerTiming(out_resp, t_routing_ns, t_handler_ns, t_hooks_ns);
    }

    /// Pure zero-I/O HTTP request dispatcher (convenience wrapper over `dispatchInto`).
    pub fn dispatch(*_, req: Request): Response {
        let ctx: Context = undefined;
        let resp: Response = undefined;
        _.dispatchInto(&req, *ctx, *resp);
        return resp;
    }

    /// Generate an OpenAPI 3.1.0 JSON specification document for all registered routes.
    /// Returns the number of bytes written to `out_buf`, or `null` if the buffer is too small.
    pub fn toOpenApiJson(*_, title: string, version: string, out_buf: []u8): usize? {
        let pos: usize = 0;

        if (!writeRaw(out_buf, *pos, "{\"openapi\":\"3.1.0\",\"info\":{\"title\":")) return null;
        if (!writeJsonEscaped(out_buf, *pos, title)) return null;
        if (!writeRaw(out_buf, *pos, ",\"version\":")) return null;
        if (!writeJsonEscaped(out_buf, *pos, version)) return null;
        if (!writeRaw(out_buf, *pos, "},\"paths\":{")) return null;

        let emitted: [APP_MAX_ROUTES]bool = [false; APP_MAX_ROUTES];
        let first_path: bool = true;

        let i: usize = 0;
        while (i < _.route_len) : (i += 1) {
            if (emitted[i]) continue;

            const base_path = _.routes[i].pathSlice();
            if (!first_path) {
                if (!writeByte(out_buf, *pos, ',')) return null;
            }
            first_path = false;

            // Convert `:param` / `*wildcard` to OpenAPI `{param}` path key
            if (!writeByte(out_buf, *pos, '"')) return null;
            if (!writeOpenApiPath(out_buf, *pos, base_path)) return null;
            if (!writeRaw(out_buf, *pos, "\":{")) return null;

            let first_method: bool = true;
            let j: usize = i;
            while (j < _.route_len) : (j += 1) {
                if (emitted[j]) continue;
                if (!bytesEql(_.routes[j].pathSlice(), base_path)) continue;
                emitted[j] = true;

                if (!first_method) {
                    if (!writeByte(out_buf, *pos, ',')) return null;
                }
                first_method = false;

                const r = _.routes[j];
                if (!writeJsonEscaped(out_buf, *pos, r.method.asLowerString())) return null;
                if (!writeRaw(out_buf, *pos, ":{")) return null;

                if (r.summary.len > 0) {
                    if (!writeRaw(out_buf, *pos, "\"summary\":")) return null;
                    if (!writeJsonEscaped(out_buf, *pos, r.summary)) return null;
                    if (!writeByte(out_buf, *pos, ',')) return null;
                }

                // Emit path parameters if present
                let has_params: bool = false;
                let s_idx: usize = 0;
                while (s_idx < r.seg_len) : (s_idx += 1) {
                    if (r.segs[s_idx].kind != RouteSegKind.Static) {
                        has_params = true;
                        break;
                    }
                }
                if (has_params) {
                    if (!writeRaw(out_buf, *pos, "\"parameters\":[")) return null;
                    let first_param: bool = true;
                    let p_idx: usize = 0;
                    const pat = r.pathSlice();
                    while (p_idx < r.seg_len) : (p_idx += 1) {
                        const seg = r.segs[p_idx];
                        if (seg.kind == RouteSegKind.Static) continue;
                        if (!first_param) {
                            if (!writeByte(out_buf, *pos, ',')) return null;
                        }
                        first_param = false;
                        const pname = pat[(seg.start as usize) .. (seg.start as usize) + (seg.len as usize)];
                        if (!writeRaw(out_buf, *pos, "{\"name\":")) return null;
                        if (!writeJsonEscaped(out_buf, *pos, pname)) return null;
                        if (!writeRaw(out_buf, *pos, ",\"in\":\"path\",\"required\":true,\"schema\":{\"type\":\"string\"}}")) return null;
                    }
                    if (!writeRaw(out_buf, *pos, "],")) return null;
                }

                if (!writeRaw(out_buf, *pos, "\"responses\":{\"200\":{\"description\":\"OK\"}}}")) return null;
            }

            if (!writeByte(out_buf, *pos, '}')) return null;
        }

        if (!writeRaw(out_buf, *pos, "}}")) return null;
        return pos;
    }
}

fn writeOpenApiPath(out_buf: []u8, pos: *usize, path: string): bool {
    if (path.len == 0 || bytesEql(path, "/")) {
        return writeByte(out_buf, pos, '/');
    }
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == '/') {
            if (!writeByte(out_buf, pos, '/')) return false;
            i += 1;
            continue;
        }
        let seg_start: usize = i;
        while (i < path.len && path[i] != '/') : (i += 1) {}
        const seg_end: usize = i;
        if (path[seg_start] == ':' && seg_end > seg_start + 1) {
            if (!writeByte(out_buf, pos, '{')) return false;
            if (!writeRaw(out_buf, pos, path[seg_start + 1 .. seg_end])) return false;
            if (!writeByte(out_buf, pos, '}')) return false;
        } else if (path[seg_start] == '*') {
            const pname = if (seg_end > seg_start + 1) path[seg_start + 1 .. seg_end] else "wildcard";
            if (!writeByte(out_buf, pos, '{')) return false;
            if (!writeRaw(out_buf, pos, pname)) return false;
            if (!writeByte(out_buf, pos, '}')) return false;
        } else {
            if (!writeRaw(out_buf, pos, path[seg_start..seg_end])) return false;
        }
    }
    return true;
}
