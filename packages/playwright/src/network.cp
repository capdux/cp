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

pub struct FulfillOptions {
    status: i32 = 200,
    content_type: []const u8 = "text/plain",
    body: []const u8 = "",
    headers_json: []const u8 = "{}",
}

pub struct Request {
    pub url: []const u8 = "",
    pub method: []const u8 = "GET",
    pub resource_type: []const u8 = "document",
    pub post_data: []const u8 = "",
    pub headers_json: []const u8 = "{}",
    pub is_navigation_request: bool = false,

    pub fn isNavigationRequest(): bool {
        return _.is_navigation_request;
    }
}

pub struct Response {
    pub url: []const u8 = "",
    pub status: i32 = 200,
    pub status_text: []const u8 = "OK",
    pub ok: bool = true,
    pub headers_json: []const u8 = "{}",
    pub body: []const u8 = "",

    pub fn isOk(): bool {
        return _.status >= 200 && _.status < 300;
    }
}

pub struct Route {
    pub request: Request = .{},
    pub action: RouteAction = RouteAction.Pending,
    pub fulfill_status: i32 = 200,
    pub fulfill_content_type: []const u8 = "text/plain",
    pub fulfill_body: []const u8 = "",
    pub abort_error_code: []const u8 = "failed",
    pub override_url: []const u8 = "",
    pub override_method: []const u8 = "",

    pub fn isHandled(): bool {
        return _.action != RouteAction.Pending;
    }

    pub fn fulfill(*_, opts: FulfillOptions): void!PlaywrightError {
        if (_.action != RouteAction.Pending) return PlaywrightError.RouteAlreadyHandled;
        _.action = RouteAction.Fulfilled;
        _.fulfill_status = opts.status;
        _.fulfill_content_type = opts.content_type;
        _.fulfill_body = opts.body;
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

    pub fn fallback(*_): void!PlaywrightError {
        if (_.action != RouteAction.Pending) return PlaywrightError.RouteAlreadyHandled;
        _.action = RouteAction.Fallback;
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
