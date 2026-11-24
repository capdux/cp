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

pub const APP_MAX_ROUTES: usize = 128;
pub const APP_MAX_MIDDLEWARES: usize = 16;
pub const APP_MAX_SEGMENTS: usize = 16;
pub const APP_MAX_PARAMS: usize = 16;
pub const APP_MAX_HEADERS: usize = 32;
pub const APP_MAX_PATH_LEN: usize = 256;

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
    headers: [APP_MAX_HEADERS]HeaderKV = [HeaderKV{}; APP_MAX_HEADERS];
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
            headers: [HeaderKV{}; APP_MAX_HEADERS],
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
    headers: [APP_MAX_HEADERS]HeaderKV = [HeaderKV{}; APP_MAX_HEADERS];
    header_len: usize = 0;

    pub static fn json(status_code: u16, payload: string): Response {
        return Response{
            status: status_code,
            content_type: "application/json; charset=utf-8",
            body: payload,
            headers: [HeaderKV{}; APP_MAX_HEADERS],
            header_len: 0,
        };
    }

    pub static fn text(status_code: u16, payload: string): Response {
        return Response{
            status: status_code,
            content_type: "text/plain; charset=utf-8",
            body: payload,
            headers: [HeaderKV{}; APP_MAX_HEADERS],
            header_len: 0,
        };
    }

    pub static fn noContent(): Response {
        return Response{
            status: 204,
            content_type: "",
            body: "",
            headers: [HeaderKV{}; APP_MAX_HEADERS],
            header_len: 0,
        };
    }

    pub static fn statusCode(code: u16): Response {
        return Response{
            status: code,
            content_type: "text/plain; charset=utf-8",
            body: "",
            headers: [HeaderKV{}; APP_MAX_HEADERS],
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
        let i: usize = 0;
        while (i < _.header_len) : (i += 1) {
            if (asciiEqlIgnoreCase(_.headers[i].name, name)) {
                return _.headers[i].value;
            }
        }
        return null;
    }
}

// ============================================================================
// 2. Axum-Style Zero-Reflection Context & Extractors
// ============================================================================

pub struct Context {
    method: AppMethod = AppMethod.GET;
    path: string = "/";
    raw_query: string = "";
    body: string = "";
    headers: [APP_MAX_HEADERS]HeaderKV = [HeaderKV{}; APP_MAX_HEADERS];
    header_len: usize = 0;
    params: [APP_MAX_PARAMS]ParamKV = [ParamKV{}; APP_MAX_PARAMS];
    param_len: usize = 0;
    state_ptr: usize = 0;
    response_headers: [APP_MAX_HEADERS]HeaderKV = [HeaderKV{}; APP_MAX_HEADERS];
    response_header_len: usize = 0;
    aborted: bool = false;
    abort_response: Response = Response{};

    /// Extract a dynamic path parameter (`:id` or `*wildcard`).
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

    /// Access the application state pointer address injected via `app.state(ptr)`.
    pub fn stateAddr(): usize {
        return _.state_ptr;
    }

    /// Attach a response header (e.g., CORS / X-Request-Id) merged into the final response.
    pub fn setHeader(*_, name: string, value: string): *Context {
        let i: usize = 0;
        while (i < _.response_header_len) : (i += 1) {
            if (asciiEqlIgnoreCase(_.response_headers[i].name, name)) {
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

    /// Build a JSON response (`application/json; charset=utf-8`).
    pub fn json(status_code: u16, payload: string): Response {
        return Response.json(status_code, payload);
    }

    /// Build a plain-text response (`text/plain; charset=utf-8`).
    pub fn text(status_code: u16, payload: string): Response {
        return Response.text(status_code, payload);
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
// 3. Handler, Middleware & Pre-Compiled Route Table
// ============================================================================

pub type HandlerFn = *const fn (*Context): Response;
pub type MiddlewareFn = *const fn (*Context): bool;

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

    pub fn get(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefix(AppMethod.GET, _.prefix_buf[0.._.prefix_len], path, "", handler);
        return _;
    }

    pub fn post(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefix(AppMethod.POST, _.prefix_buf[0.._.prefix_len], path, "", handler);
        return _;
    }

    pub fn put(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefix(AppMethod.PUT, _.prefix_buf[0.._.prefix_len], path, "", handler);
        return _;
    }

    pub fn delete(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefix(AppMethod.DELETE, _.prefix_buf[0.._.prefix_len], path, "", handler);
        return _;
    }

    pub fn patch(*_, path: string, handler: HandlerFn): *RouteGroup {
        _.app.registerWithPrefix(AppMethod.PATCH, _.prefix_buf[0.._.prefix_len], path, "", handler);
        return _;
    }

    pub fn group(*_, sub_prefix: string): RouteGroup {
        let g = RouteGroup{
            app: _.app,
            prefix_buf: [0; APP_MAX_PATH_LEN],
            prefix_len: 0,
        };
        g.prefix_len = normalizeAndJoinPath(_.prefix_buf[0.._.prefix_len], sub_prefix, *g.prefix_buf);
        return g;
    }
}

// ============================================================================
// 5. Flagship App Kernel (Chainable Registration + Dispatch + OpenAPI 3.1)
// ============================================================================

pub struct App {
    routes: [APP_MAX_ROUTES]RouteEntry = [RouteEntry{}; APP_MAX_ROUTES];
    route_len: usize = 0;
    middlewares: [APP_MAX_MIDDLEWARES]MiddlewareFn? = [null; APP_MAX_MIDDLEWARES];
    middleware_len: usize = 0;
    state_ptr: usize = 0;

    pub static fn init(): App {
        return App{
            routes: [RouteEntry{}; APP_MAX_ROUTES],
            route_len: 0,
            middlewares: [null; APP_MAX_MIDDLEWARES],
            middleware_len: 0,
            state_ptr: 0,
        };
    }

    pub fn registerWithPrefix(
        *_,
        method: AppMethod,
        prefix: string,
        path: string,
        summary: string,
        handler: HandlerFn,
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
        };
        entry.path_len = normalizeAndJoinPath(prefix, path, *entry.path_buf);
        compileRouteSegments(*entry);
        _.routes[_.route_len] = entry;
        _.route_len += 1;
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

    pub fn use(*_, middleware: MiddlewareFn): *App {
        if (_.middleware_len < APP_MAX_MIDDLEWARES) {
            _.middlewares[_.middleware_len] = middleware;
            _.middleware_len += 1;
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

    pub fn routeCount(): usize {
        return _.route_len;
    }

    pub fn middlewareCount(): usize {
        return _.middleware_len;
    }

    /// Pure zero-I/O HTTP request dispatcher: executes middleware chain, matches route,
    /// extracts params, runs handler, and merges response headers.
    pub fn dispatch(*_, req: Request): Response {
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

        let ctx = Context{
            method: req.method,
            path: if (clean_path.len == 0) "/" else clean_path,
            raw_query: query_str,
            body: req.body,
            headers: req.headers,
            header_len: req.header_len,
            params: [ParamKV{}; APP_MAX_PARAMS],
            param_len: 0,
            state_ptr: _.state_ptr,
            response_headers: [HeaderKV{}; APP_MAX_HEADERS],
            response_header_len: 0,
            aborted: false,
            abort_response: Response{},
        };

        // 1. Run pre-dispatch middleware chain
        let mw_idx: usize = 0;
        while (mw_idx < _.middleware_len) : (mw_idx += 1) {
            if (mw := _.middlewares[mw_idx]) {
                const proceed = mw(*ctx);
                if (!proceed || ctx.aborted) {
                    let resp = if (ctx.aborted)
                        ctx.abort_response
                    else
                        Response.json(403, "{\"error\":\"Forbidden\"}");
                    mergeContextHeaders(*ctx, *resp);
                    return resp;
                }
            }
        }

        // 2. Match route (highest priority wins; track allowed methods for 405)
        let best_idx: usize? = null;
        let best_priority: u16 = 0;
        let best_params: [APP_MAX_PARAMS]ParamKV = [ParamKV{}; APP_MAX_PARAMS];
        let best_param_len: usize = 0;
        let allowed_mask: u8 = 0;

        let tmp_params: [APP_MAX_PARAMS]ParamKV = [ParamKV{}; APP_MAX_PARAMS];
        let tmp_param_len: usize = 0;

        let i: usize = 0;
        while (i < _.route_len) : (i += 1) {
            if (matchRouteEntry(*_.routes[i], ctx.path, *tmp_params, *tmp_param_len)) {
                const entry_method = _.routes[i].method;
                allowed_mask = allowed_mask | entry_method.bitMask();
                if (entry_method == ctx.method) {
                    const entry_prio = _.routes[i].priority;
                    if (best_idx == null || entry_prio > best_priority) {
                        best_idx = i;
                        best_priority = entry_prio;
                        best_param_len = tmp_param_len;
                        let p_i: usize = 0;
                        while (p_i < tmp_param_len) : (p_i += 1) {
                            best_params[p_i] = tmp_params[p_i];
                        }
                    }
                }
            }
        }

        if (matched := best_idx) {
            ctx.params = best_params;
            ctx.param_len = best_param_len;
            if (h := _.routes[matched].handler) {
                let resp = h(*ctx);
                mergeContextHeaders(*ctx, *resp);
                return resp;
            }
        }

        // 3. Distinguish 405 Method Not Allowed vs 404 Not Found
        if (allowed_mask != 0) {
            let resp = Response.json(405, "{\"error\":\"Method Not Allowed\"}");
            * = resp.setHeader("Allow", formatAllowHeaderValue(allowed_mask));
            mergeContextHeaders(*ctx, *resp);
            return resp;
        }

        let not_found = Response.json(404, "{\"error\":\"Not Found\"}");
        mergeContextHeaders(*ctx, *not_found);
        return not_found;
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
