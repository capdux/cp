// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/network.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/network.cp — Request, Response, Route & URL Router
// Zero-copy models aligned with Playwright's network interception API.
// ============================================================================

const std = @import("std");

type NetVec16 = @Vector(16, u8);

pub err PlaywrightError {
    NotConnected,
    ConnectionClosed,
    BrowserClosed,
    ContextClosed,
    PageClosed,
    TargetNotFound,
    Timeout,
    InvalidSelector,
    InvalidUrl,
    ExecutableNotFound,
    ProtocolError,
    BufferTooSmall,
    RouteAlreadyHandled,
    OutOfMemory,
}

pub enum RouteAction {
    Pending,
    Continued,
    Fulfilled,
    Aborted,
    Fallback,
}

pub struct NameValue {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
}

pub struct ResourceTiming {
    pub start_time: f64 = 0.0,
    pub domain_lookup_start: f64 = -1.0,
    pub domain_lookup_end: f64 = -1.0,
    pub connect_start: f64 = -1.0,
    pub connect_end: f64 = -1.0,
    pub secure_connection_start: f64 = -1.0,
    pub response_start: f64 = -1.0,
    pub response_end: f64 = -1.0,
}

pub struct RequestSizes {
    pub request_body_size: i64 = 0,
    pub request_headers_size: i64 = 0,
    pub response_body_size: i64 = 0,
    pub response_headers_size: i64 = 0,
}

pub struct RequestFailure {
    pub error_text: []const u8 = "",
}

pub struct SecurityDetails {
    pub issuer: []const u8 = "",
    pub protocol: []const u8 = "",
    pub subject_name: []const u8 = "",
    pub valid_from: i64 = 0,
    pub valid_to: i64 = 0,
}

pub struct RemoteAddr {
    pub ip_address: []const u8 = "",
    pub port: u16 = 0,
}

pub struct FulfillOptions {
    status: i32 = 200,
    content_type: []const u8 = "text/plain",
    body: []const u8 = "",
    headers_json: []const u8 = "{}",
}

pub struct RouteContinueOverrides {
    pub url: []const u8 = "",
    pub method: []const u8 = "",
    pub headers_json: []const u8 = "",
    pub post_data: []const u8 = "",
}

pub struct ScriptTagOptions {
    pub url: []const u8 = "",
    pub path: []const u8 = "",
    pub content: []const u8 = "",
    pub tag_type: []const u8 = "",
}

pub struct StyleTagOptions {
    pub url: []const u8 = "",
    pub path: []const u8 = "",
    pub content: []const u8 = "",
}

pub struct WorkerInfo {
    pub url: []const u8 = "",
    pub target_id: []const u8 = "",
}

pub struct PageEventRecord {
    pub name: []const u8 = "",
    pub payload: []const u8 = "",
}

pub type ExposedBindingFn = fn(ctx: usize, args_json: []const u8): []const u8;

pub struct ExposedBindingEntry {
    pub name: []const u8 = "",
    pub callback: ExposedBindingFn = undefined,
    pub ctx: usize = 0,
}

fn skipWs(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
        i += 1;
    }
    return i;
}

fn parseJsonStringSlice(s: []const u8, quote_pos: usize, next_pos: *usize): ([]const u8)? {
    if (quote_pos >= s.len || s[quote_pos] != '"') return null;
    let i = quote_pos + 1;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == '"') {
            next_pos.* = i + 1;
            return s[quote_pos + 1 .. i];
        }
        i += 1;
    }
    return null;
}

/// Zero-copy case-insensitive lookup of a header key inside a flat JSON object string.
pub fn findHeaderInJson(headers_json: []const u8, target_name: []const u8): ([]const u8)? {
    let i = skipWs(headers_json, 0);
    if (i >= headers_json.len || headers_json[i] != '{') return null;
    i += 1;
    while (i < headers_json.len) {
        i = skipWs(headers_json, i);
        if (i >= headers_json.len || headers_json[i] == '}') return null;
        if (headers_json[i] == ',') {
            i += 1;
            continue;
        }
        let after_key: usize = 0;
        const key = parseJsonStringSlice(headers_json, i, &after_key) orelse return null;
        i = skipWs(headers_json, after_key);
        if (i >= headers_json.len || headers_json[i] != ':') return null;
        i = skipWs(headers_json, i + 1);
        let after_val: usize = 0;
        const val = parseJsonStringSlice(headers_json, i, &after_val) orelse return null;
        i = after_val;
        if (std.ascii.eqlIgnoreCase(key, target_name)) {
            return val;
        }
    }
    return null;
}

/// Parse a flat JSON headers object into a caller-owned slice of NameValue pairs.
pub fn parseHeadersJsonArray(alloc: std.mem.Allocator, headers_json: []const u8): ([]NameValue)!PlaywrightError {
    let out: std.ArrayList(NameValue) = .empty;
    errdefer out.deinit(alloc);

    let i = skipWs(headers_json, 0);
    if (i >= headers_json.len || headers_json[i] != '{') {
        return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }
    i += 1;
    while (i < headers_json.len) {
        i = skipWs(headers_json, i);
        if (i >= headers_json.len || headers_json[i] == '}') break;
        if (headers_json[i] == ',') {
            i += 1;
            continue;
        }
        let after_key: usize = 0;
        const key = parseJsonStringSlice(headers_json, i, &after_key) orelse break;
        i = skipWs(headers_json, after_key);
        if (i >= headers_json.len || headers_json[i] != ':') break;
        i = skipWs(headers_json, i + 1);
        let after_val: usize = 0;
        const val = parseJsonStringSlice(headers_json, i, &after_val) orelse break;
        i = after_val;
        out.append(alloc, NameValue{ name: key, value: val }) catch return PlaywrightError.OutOfMemory;
    }
    return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
}

pub struct Request {
    pub url: []const u8 = "",
    pub method: []const u8 = "GET",
    pub resource_type: []const u8 = "document",
    pub post_data: []const u8 = "",
    pub headers_json: []const u8 = "{}",
    pub is_navigation_request: bool = false,
    pub frame_id: []const u8 = "",
    pub failure_text: []const u8 = "",
    pub redirected_from: (*Request)? = null,
    pub redirected_to: (*Request)? = null,
    pub response_ptr: (*Response)? = null,
    pub resource_timing: ResourceTiming = .{},
    pub request_sizes: RequestSizes = .{},

    pub fn isNavigationRequest(): bool {
        return _.is_navigation_request;
    }

    pub fn resourceType(): []const u8 {
        return _.resource_type;
    }

    pub fn allHeaders(): []const u8 {
        return _.headers_json;
    }

    pub fn headersArray(alloc: std.mem.Allocator): ([]NameValue)!PlaywrightError {
        return parseHeadersJsonArray(alloc, _.headers_json);
    }

    pub fn headerValue(name: []const u8): ([]const u8)? {
        return findHeaderInJson(_.headers_json, name);
    }

    pub fn postData(): ([]const u8)? {
        if (_.post_data.len == 0) return null;
        return _.post_data;
    }

    pub fn postDataJSON(): ([]const u8)? {
        const trimmed_start = skipWs(_.post_data, 0);
        if (trimmed_start >= _.post_data.len) return null;
        const first = _.post_data[trimmed_start];
        if (first != '{' && first != '[') return null;
        return _.post_data[trimmed_start..];
    }

    pub fn redirectedFrom(): (*Request)? {
        return _.redirected_from;
    }

    pub fn redirectedTo(): (*Request)? {
        return _.redirected_to;
    }

    pub fn timing(): ResourceTiming {
        return _.resource_timing;
    }

    pub fn sizes(): RequestSizes {
        let s = _.request_sizes;
        if (s.request_body_size == 0 && _.post_data.len > 0) {
            s.request_body_size = _.post_data.len as i64;
        }
        if (s.request_headers_size == 0 && _.headers_json.len > 2) {
            s.request_headers_size = _.headers_json.len as i64;
        }
        return s;
    }

    pub fn failure(): RequestFailure? {
        if (_.failure_text.len == 0) return null;
        return RequestFailure{ error_text: _.failure_text };
    }

    pub fn response(): (*Response)? {
        return _.response_ptr;
    }

    pub fn frame(): []const u8 {
        return _.frame_id;
    }
}

pub struct Response {
    pub url: []const u8 = "",
    pub status: i32 = 200,
    pub status_text: []const u8 = "OK",
    pub ok: bool = true,
    pub headers_json: []const u8 = "{}",
    pub body_bytes: []const u8 = "",
    pub from_service_worker: bool = false,
    pub finish_error: ([]const u8)? = null,
    pub security_details: SecurityDetails? = null,
    pub server_addr: RemoteAddr? = null,
    pub request_ptr: (*Request)? = null,

    pub fn isOk(): bool {
        return _.status >= 200 && _.status < 300;
    }

    pub fn statusText(): []const u8 {
        return _.status_text;
    }

    pub fn allHeaders(): []const u8 {
        return _.headers_json;
    }

    pub fn headersArray(alloc: std.mem.Allocator): ([]NameValue)!PlaywrightError {
        return parseHeadersJsonArray(alloc, _.headers_json);
    }

    pub fn headerValue(name: []const u8): ([]const u8)? {
        return findHeaderInJson(_.headers_json, name);
    }

    pub fn body(): []const u8 {
        return _.body_bytes;
    }

    pub fn text(): []const u8 {
        return _.body_bytes;
    }

    pub fn json(): ([]const u8)!PlaywrightError {
        const start = skipWs(_.body_bytes, 0);
        if (start >= _.body_bytes.len) return PlaywrightError.ProtocolError;
        const c = _.body_bytes[start];
        if (c != '{' && c != '[' && c != '"') return PlaywrightError.ProtocolError;
        return _.body_bytes[start..];
    }

    pub fn finished(): ([]const u8)? {
        return _.finish_error;
    }

    pub fn fromServiceWorker(): bool {
        return _.from_service_worker;
    }

    pub fn securityDetails(): SecurityDetails? {
        return _.security_details;
    }

    pub fn serverAddr(): RemoteAddr? {
        return _.server_addr;
    }

    pub fn request(): (*Request)? {
        return _.request_ptr;
    }
}

pub struct Route {
    pub request: Request = .{},
    pub action: RouteAction = RouteAction.Pending,
    pub fulfill_status: i32 = 200,
    pub fulfill_content_type: []const u8 = "text/plain",
    pub fulfill_body: []const u8 = "",
    pub fulfill_headers_json: []const u8 = "{}",
    pub abort_error_code: []const u8 = "failed",
    pub override_url: []const u8 = "",
    pub override_method: []const u8 = "",
    pub override_headers_json: []const u8 = "",
    pub override_post_data: []const u8 = "",
    pub fetched: bool = false,

    pub fn isHandled(): bool {
        return _.action != RouteAction.Pending;
    }

    pub fn fulfill(*_, opts: FulfillOptions): void!PlaywrightError {
        if (_.action != RouteAction.Pending) return PlaywrightError.RouteAlreadyHandled;
        _.action = RouteAction.Fulfilled;
        _.fulfill_status = opts.status;
        _.fulfill_content_type = opts.content_type;
        _.fulfill_body = opts.body;
        _.fulfill_headers_json = opts.headers_json;
    }

    pub fn abort(*_, error_code: []const u8): void!PlaywrightError {
        if (_.action != RouteAction.Pending) return PlaywrightError.RouteAlreadyHandled;
        _.action = RouteAction.Aborted;
        _.abort_error_code = if (error_code.len > 0) error_code else "failed";
    }

    pub fn continue_(*_, override_url: []const u8, override_method: []const u8): void!PlaywrightError {
        if (_.action != RouteAction.Pending) return PlaywrightError.RouteAlreadyHandled;
        _.action = RouteAction.Continued;
        _.override_url = override_url;
        _.override_method = override_method;
    }

    pub fn continueWithOverrides(*_, overrides: RouteContinueOverrides): void!PlaywrightError {
        if (_.action != RouteAction.Pending) return PlaywrightError.RouteAlreadyHandled;
        _.action = RouteAction.Continued;
        _.override_url = overrides.url;
        _.override_method = overrides.method;
        _.override_headers_json = overrides.headers_json;
        _.override_post_data = overrides.post_data;
    }

    pub fn fallback(*_): void!PlaywrightError {
        if (_.action != RouteAction.Pending) return PlaywrightError.RouteAlreadyHandled;
        _.action = RouteAction.Fallback;
    }

    pub fn fetch(*_, overrides: RouteContinueOverrides): Response!PlaywrightError {
        const effective_url = if (overrides.url.len > 0) overrides.url else _.request.url;
        if (effective_url.len == 0) return PlaywrightError.InvalidUrl;
        const effective_headers = if (overrides.headers_json.len > 0) overrides.headers_json else _.request.headers_json;
        const effective_body = if (overrides.post_data.len > 0) overrides.post_data else _.fulfill_body;
        _.fetched = true;
        return Response{
            url: effective_url,
            status: 200,
            status_text: "OK",
            ok: true,
            headers_json: effective_headers,
            body_bytes: effective_body,
            request_ptr: &_.request,
        };
    }
}

pub type RouteHandlerFn = fn(ctx: usize, route: *Route): void;

pub struct RouteEntry {
    pub pattern: []const u8 = "",
    pub handler: RouteHandlerFn = undefined,
    pub ctx: usize = 0,
    pub times: i32 = -1,
}

pub struct UrlMatcher {
    /// 16-lane SIMD scan to check if a glob pattern contains `*` or `?` wildcards.
    pub static fn hasWildcardsSimd(pattern: []const u8): bool {
        let i: usize = 0;
        const v_star: NetVec16 = @splat('*');
        const v_qmark: NetVec16 = @splat('?');
        while (i + 16 <= pattern.len) : (i += 16) {
            const chunk: NetVec16 = pattern[i..][0..16].*;
            const mask = (chunk == v_star) | (chunk == v_qmark);
            if (@reduce(.Or, mask)) return true;
        }
        while (i < pattern.len) : (i += 1) {
            if (pattern[i] == '*' || pattern[i] == '?') return true;
        }
        return false;
    }

    /// Glob matcher supporting `*` and `**` wildcards against URLs (with SIMD literal fast-path).
    pub static fn matches(pattern: []const u8, url: []const u8): bool {
        if (pattern.len == 0 || std.mem.eql(u8, pattern, "*") || std.mem.eql(u8, pattern, "**")) {
            return true;
        }
        if (!UrlMatcher.hasWildcardsSimd(pattern)) {
            return std.mem.eql(u8, pattern, url);
        }
        return UrlMatcher.globMatch(pattern, url);
    }

    static fn globMatch(pat: []const u8, text: []const u8): bool {
        let pi: usize = 0;
        let ti: usize = 0;
        let star_p: usize = 0;
        let has_star = false;
        let star_t: usize = 0;

        while (ti < text.len) {
            if (pi < pat.len && (pat[pi] == text[ti] || pat[pi] == '?')) {
                pi += 1;
                ti += 1;
            } else if (pi < pat.len && pat[pi] == '*') {
                while (pi < pat.len && pat[pi] == '*') {
                    pi += 1;
                }
                if (pi == pat.len) return true;
                has_star = true;
                star_p = pi;
                star_t = ti;
            } else if (has_star) {
                star_t += 1;
                ti = star_t;
                pi = star_p;
            } else {
                return false;
            }
        }

        while (pi < pat.len && pat[pi] == '*') {
            pi += 1;
        }
        return pi == pat.len;
    }
}

pub struct WebSocketRoute {
    allocator: std.mem.Allocator = undefined,
    pub url: []const u8 = "",
    pub closed: bool = false,
    pub close_code: u16 = 1000,
    pub close_reason: []const u8 = "",
    pub connected_to_server: bool = false,
    pub sent_to_page: std.ArrayList([]u8) = .empty,
    pub sent_to_server: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator, ws_url: []const u8): WebSocketRoute {
        return WebSocketRoute{
            allocator: alloc,
            url: ws_url,
            closed: false,
            close_code: 1000,
            close_reason: "",
            connected_to_server: false,
            sent_to_page: .empty,
            sent_to_server: .empty,
        };
    }

    pub fn deinit(*_) {
        for (m of _.sent_to_page.items) {
            _.allocator.free(m);
        }
        _.sent_to_page.deinit(_.allocator);
        _.sent_to_page = .empty;

        for (m of _.sent_to_server.items) {
            _.allocator.free(m);
        }
        _.sent_to_server.deinit(_.allocator);
        _.sent_to_server = .empty;
    }

    ~init() {
        _.deinit();
    }

    pub fn connectToServer(*_) {
        _.connected_to_server = true;
    }

    pub fn send(*_, message: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ConnectionClosed;
        const duped = _.allocator.dupe(u8, message) catch return PlaywrightError.OutOfMemory;
        _.sent_to_page.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
    }

    pub fn sendToServer(*_, message: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ConnectionClosed;
        const duped = _.allocator.dupe(u8, message) catch return PlaywrightError.OutOfMemory;
        _.sent_to_server.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
    }

    pub fn close(*_, code: u16, reason: []const u8) {
        _.closed = true;
        _.close_code = code;
        _.close_reason = reason;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }
}

pub type WebSocketRouteHandlerFn = fn(ctx: usize, ws_route: *WebSocketRoute): void;

pub struct WebSocketRouteEntry {
    pub pattern: []const u8 = "",
    pub handler: WebSocketRouteHandlerFn = undefined,
    pub ctx: usize = 0,
}
