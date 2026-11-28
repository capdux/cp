// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/fetch.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/request_context.cp — APIRequestContext & Cookie Jar Sync
// Built on cap:http (Method, HTTPClient) & cap:json with BrowserContext cookie sync,
// max_redirects automatic redirect chain (301/302/303/307/308 + redirect history),
// MultipartFormData encoder, ProxySettings, and ignore_https_errors support.
// ============================================================================

const std = @import("std");
import { Method } from "cap:http/method";
import { HTTPClient } from "cap:http/client";
import { JSON, JsonValue } from "cap:json";
import { PlaywrightError, UrlMatcher, findHeaderInJson } from "./network";
import { Cookie, BrowserContext } from "./context";

type FetchVec16 = @Vector(16, u8);

/// 16-lane SIMD scan to check if a multipart field name/value contains CR/LF or quotes.
pub fn simdHasHeaderUnsafeBytes(data: []const u8): bool {
    let i: usize = 0;
    const v_cr: FetchVec16 = @splat('\r');
    const v_lf: FetchVec16 = @splat('\n');
    const v_qt: FetchVec16 = @splat('"');
    while (i + 16 <= data.len) : (i += 16) {
        const chunk: FetchVec16 = data[i..][0..16].*;
        if (@reduce(.Or, (chunk == v_cr) | (chunk == v_lf) | (chunk == v_qt))) return true;
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '\r' || b == '\n' || b == '"') return true;
    }
    return false;
}

pub struct ProxySettings {
    pub server: []const u8 = "",
    pub bypass: []const u8 = "",
    pub username: []const u8 = "",
    pub password: []const u8 = "",

    /// Check if `host` matches any comma-separated bypass rule (e.g. `localhost,.internal.local`).
    pub fn shouldBypass(host: []const u8): bool {
        if (_.bypass.len == 0 || host.len == 0) return false;
        let start: usize = 0;
        while (start <= _.bypass.len) {
            let end = start;
            while (end < _.bypass.len && _.bypass[end] != ',') : (end += 1) {}
            const rule = std.mem.trim(u8, _.bypass[start..end], " \t");
            if (rule.len > 0) {
                if (std.mem.eql(u8, rule, "*") || std.ascii.eqlIgnoreCase(rule, host)) {
                    return true;
                }
                if (rule[0] == '.' && host.len >= rule.len) {
                    if (std.ascii.eqlIgnoreCase(host[host.len - rule.len ..], rule)) return true;
                }
                if (UrlMatcher.matches(rule, host)) return true;
            }
            if (end >= _.bypass.len) break;
            start = end + 1;
        }
        return false;
    }

    /// Build `Basic <base64(username:password)>` into a stack buffer.
    pub fn buildProxyAuthHeader(out_buf: []u8): ([]const u8)!PlaywrightError {
        if (_.username.len == 0 && _.password.len == 0) return "";
        let raw_buf: [512]u8 = undefined;
        const raw = std.fmt.bufPrint(&raw_buf, "{s}:{s}", .{ _.username, _.password }) catch return PlaywrightError.BufferTooSmall;
        const b64_len = std.base64.standard.Encoder.calcSize(raw.len);
        if (6 + b64_len > out_buf.len) return PlaywrightError.BufferTooSmall;
        @memcpy(out_buf[0..6], "Basic ");
        const encoded = std.base64.standard.Encoder.encode(out_buf[6 .. 6 + b64_len], raw);
        return out_buf[0 .. 6 + encoded.len];
    }
}

pub struct MultipartField {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
    pub filename: []const u8 = "",
    pub content_type: []const u8 = "",
}

pub struct MultipartFormData {
    allocator: std.mem.Allocator = undefined,
    deinitialized: bool = false,
    pub boundary: []const u8 = "----PlaywrightCPFormBoundary7MA4YWxkTrZu0gW",
    pub fields: std.ArrayList(MultipartField) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator): MultipartFormData {
        return MultipartFormData{
            allocator: alloc,
            deinitialized: false,
            boundary: "----PlaywrightCPFormBoundary7MA4YWxkTrZu0gW",
            fields: .empty,
            owned_strings: .empty,
        };
    }

    pub static fn initWithBoundary(alloc: std.mem.Allocator, seed: u64): MultipartFormData!PlaywrightError {
        let form = MultipartFormData.init(alloc);
        errdefer form.deinit();
        let b_buf: [64]u8 = undefined;
        const b = std.fmt.bufPrint(&b_buf, "----PlaywrightCPBoundary{x:0>16}", .{seed}) catch return PlaywrightError.BufferTooSmall;
        form.boundary = form.intern(b)?;
        return form;
    }

    pub fn deinit(*_) {
        if (_.deinitialized) return;
        _.deinitialized = true;
        _.fields.deinit(_.allocator);
        _.fields = .empty;
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        return duped;
    }

    pub fn addField(*_, name: []const u8, value: []const u8): void!PlaywrightError {
        const n = _.intern(name)?;
        const v = _.intern(value)?;
        _.fields.append(_.allocator, MultipartField{
            name: n,
            value: v,
            filename: "",
            content_type: "",
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn addFile(
        *_,
        name: []const u8,
        filename: []const u8,
        content_type: []const u8,
        data: []const u8,
    ): void!PlaywrightError {
        const n = _.intern(name)?;
        const fn_s = _.intern(filename)?;
        const ct = _.intern(if (content_type.len > 0) content_type else "application/octet-stream")?;
        const d = _.intern(data)?;
        _.fields.append(_.allocator, MultipartField{
            name: n,
            value: d,
            filename: fn_s,
            content_type: ct,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn contentTypeHeader(out_buf: []u8): ([]const u8)!PlaywrightError {
        return std.fmt.bufPrint(out_buf, "multipart/form-data; boundary={s}", .{_.boundary}) catch return PlaywrightError.BufferTooSmall;
    }

    /// Encode all text and file fields into RFC 7578 multipart/form-data bytes.
    pub fn encode(alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        for (f of _.fields.items) {
            out.appendSlice(alloc, "--") catch return PlaywrightError.OutOfMemory;
            out.appendSlice(alloc, _.boundary) catch return PlaywrightError.OutOfMemory;
            out.appendSlice(alloc, "\r\nContent-Disposition: form-data; name=\"") catch return PlaywrightError.OutOfMemory;
            out.appendSlice(alloc, f.name) catch return PlaywrightError.OutOfMemory;
            out.append(alloc, '"') catch return PlaywrightError.OutOfMemory;
            if (f.filename.len > 0) {
                out.appendSlice(alloc, "; filename=\"") catch return PlaywrightError.OutOfMemory;
                out.appendSlice(alloc, f.filename) catch return PlaywrightError.OutOfMemory;
                out.append(alloc, '"') catch return PlaywrightError.OutOfMemory;
            }
            out.appendSlice(alloc, "\r\n") catch return PlaywrightError.OutOfMemory;
            if (f.content_type.len > 0) {
                out.appendSlice(alloc, "Content-Type: ") catch return PlaywrightError.OutOfMemory;
                out.appendSlice(alloc, f.content_type) catch return PlaywrightError.OutOfMemory;
                out.appendSlice(alloc, "\r\n") catch return PlaywrightError.OutOfMemory;
            }
            out.appendSlice(alloc, "\r\n") catch return PlaywrightError.OutOfMemory;
            out.appendSlice(alloc, f.value) catch return PlaywrightError.OutOfMemory;
            out.appendSlice(alloc, "\r\n") catch return PlaywrightError.OutOfMemory;
        }

        out.appendSlice(alloc, "--") catch return PlaywrightError.OutOfMemory;
        out.appendSlice(alloc, _.boundary) catch return PlaywrightError.OutOfMemory;
        out.appendSlice(alloc, "--\r\n") catch return PlaywrightError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }
}

pub struct RedirectHop {
    pub from_url: []const u8 = "",
    pub to_url: []const u8 = "",
    pub status: i32 = 302,
    pub method: []const u8 = "GET",
}

pub struct FetchTransportResult {
    pub status: i32 = 200,
    pub status_text: []const u8 = "OK",
    pub headers_json: []const u8 = "{}",
    pub body: []const u8 = "",
}

pub type FetchTransportFn = fn(
    ctx: usize,
    method: []const u8,
    url: []const u8,
    headers_json: []const u8,
    body: []const u8,
    proxy_server: []const u8,
    ignore_https_errors: bool,
): FetchTransportResult;

pub struct APIRequestOptions {
    pub base_url: []const u8 = "",
    pub extra_http_headers_json: []const u8 = "{}",
    pub timeout_ms: u32 = 30000,
    pub ignore_https_errors: bool = false,
    pub max_redirects: u32 = 20,
    pub proxy: ProxySettings? = null,
}

pub struct APIRequestFetchOptions {
    pub method: []const u8 = "GET",
    pub data: []const u8 = "",
    pub headers_json: []const u8 = "{}",
    pub timeout_ms: u32 = 30000,
    pub max_redirects: i32 = -1,
    pub ignore_https_errors: bool? = null,
    pub multipart: (*const MultipartFormData)? = null,
    pub fail_on_status_code: bool = false,
}

pub struct APIResponse {
    pub url: []const u8 = "",
    pub status: i32 = 200,
    pub status_text: []const u8 = "OK",
    pub headers_json: []const u8 = "{}",
    pub body: []const u8 = "",
    pub ok: bool = true,
    pub raw_request: []const u8 = "",
    pub redirect_count: u32 = 0,
    pub redirect_history: []const RedirectHop = &.{},
    pub proxy_used: []const u8 = "",
    pub tls_verify_disabled: bool = false,

    pub fn isOk(): bool {
        return _.status >= 200 && _.status < 300;
    }

    pub fn statusCode(): i32 {
        return _.status;
    }

    pub fn text(): []const u8 {
        return _.body;
    }

    pub fn json(alloc: std.mem.Allocator): JsonValue!PlaywrightError {
        if (_.body.len == 0) return PlaywrightError.ProtocolError;
        return JSON.parse(alloc, _.body) catch return PlaywrightError.ProtocolError;
    }
}

pub struct APIRequestContext {
    allocator: std.mem.Allocator = undefined,
    deinitialized: bool = false,
    pub options: APIRequestOptions = .{},
    pub disposed: bool = false,

    transport_fn: FetchTransportFn? = null,
    transport_ctx: usize = 0,

    cookies_list: std.ArrayList(Cookie) = .empty,
    redirect_log: std.ArrayList(RedirectHop) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator, opts: APIRequestOptions): APIRequestContext {
        return APIRequestContext{
            allocator: alloc,
            deinitialized: false,
            options: opts,
            disposed: false,
            transport_fn: null,
            transport_ctx: 0,
            cookies_list: .empty,
            redirect_log: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        if (_.deinitialized) return;
        _.deinitialized = true;
        _.disposed = true;
        _.cookies_list.deinit(_.allocator);
        _.cookies_list = .empty;
        _.redirect_log.deinit(_.allocator);
        _.redirect_log = .empty;
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    pub fn dispose(*_) {
        _.deinit();
    }

    pub fn setTransport(*_, cb: FetchTransportFn, ctx: usize) {
        _.transport_fn = cb;
        _.transport_ctx = ctx;
    }

    fn intern(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        return duped;
    }

    pub fn addCookie(*_, c: Cookie): void!PlaywrightError {
        if (_.disposed) return PlaywrightError.ContextClosed;
        const name_s = _.intern(c.name)?;
        const val_s = _.intern(c.value)?;
        const dom_s = _.intern(c.domain)?;
        const path_s = _.intern(c.path)?;
        const ss_s = _.intern(c.same_site)?;
        _.cookies_list.append(_.allocator, Cookie{
            name: name_s,
            value: val_s,
            domain: dom_s,
            path: path_s,
            expires: c.expires,
            http_only: c.http_only,
            secure: c.secure,
            same_site: ss_s,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn cookies(): []const Cookie {
        return _.cookies_list.items;
    }

    pub fn syncCookiesFromContext(*_, ctx: *BrowserContext): void!PlaywrightError {
        if (_.disposed) return PlaywrightError.ContextClosed;
        _.cookies_list.clearRetainingCapacity();
        for (c of ctx.cookies()) {
            _.addCookie(c)?;
        }
    }

    pub fn syncCookiesToContext(*_, ctx: *BrowserContext): void!PlaywrightError {
        if (_.disposed) return PlaywrightError.ContextClosed;
        ctx.addCookies(_.cookies_list.items)?;
    }

    pub static fn isRedirectStatus(status: i32): bool {
        return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
    }

    /// RFC 7231 / Playwright redirect method rewrite rules:
    /// 303 -> GET (unless HEAD); 301/302 POST -> GET; 307/308 preserve method.
    pub static fn resolveRedirectMethod(status: i32, current_method: []const u8): []const u8 {
        if (status == 303) {
            if (std.ascii.eqlIgnoreCase(current_method, "HEAD")) return "HEAD";
            return "GET";
        }
        if ((status == 301 || status == 302) && std.ascii.eqlIgnoreCase(current_method, "POST")) {
            return "GET";
        }
        return current_method;
    }

    static fn extractHost(url: []const u8): []const u8 {
        let rest = url;
        if (pos := std.mem.indexOf(u8, rest, "://")) {
            rest = rest[pos + 3 ..];
        }
        let end = rest.len;
        if (slash := std.mem.indexOfScalar(u8, rest, '/')) {
            end = slash;
        }
        const authority = rest[0..end];
        if (colon := std.mem.indexOfScalar(u8, authority, ':')) {
            return authority[0..colon];
        }
        return authority;
    }

    static fn extractOrigin(url: []const u8): []const u8 {
        if (pos := std.mem.indexOf(u8, url, "://")) {
            const after = pos + 3;
            if (slash := std.mem.indexOfScalarPos(u8, url, after, '/')) {
                return url[0..slash];
            }
            return url;
        }
        return "";
    }

    pub fn resolveRedirectUrl(*_, current_url: []const u8, location: []const u8): ([]const u8)!PlaywrightError {
        if (location.len == 0) return PlaywrightError.InvalidUrl;
        if (std.mem.startsWith(u8, location, "http://") || std.mem.startsWith(u8, location, "https://")) {
            return _.intern(location);
        }
        if (std.mem.startsWith(u8, location, "//")) {
            const scheme: []const u8 = if (std.mem.startsWith(u8, current_url, "https://")) "https:" else "http:";
            const joined = std.fmt.allocPrint(_.allocator, "{s}{s}", .{ scheme, location }) catch return PlaywrightError.OutOfMemory;
            _.owned_strings.append(_.allocator, joined) catch {
                _.allocator.free(joined);
                return PlaywrightError.OutOfMemory;
            };
            return joined;
        }
        const origin = APIRequestContext.extractOrigin(current_url);
        if (origin.len == 0) return PlaywrightError.InvalidUrl;
        if (location[0] == '/') {
            const joined = std.fmt.allocPrint(_.allocator, "{s}{s}", .{ origin, location }) catch return PlaywrightError.OutOfMemory;
            _.owned_strings.append(_.allocator, joined) catch {
                _.allocator.free(joined);
                return PlaywrightError.OutOfMemory;
            };
            return joined;
        }
        const joined = std.fmt.allocPrint(_.allocator, "{s}/{s}", .{ origin, location }) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, joined) catch {
            _.allocator.free(joined);
            return PlaywrightError.OutOfMemory;
        };
        return joined;
    }

    pub fn resolveUrl(*_, url_or_path: []const u8): ([]const u8)!PlaywrightError {
        if (url_or_path.len == 0) return PlaywrightError.InvalidUrl;
        if (std.mem.startsWith(u8, url_or_path, "http://") || std.mem.startsWith(u8, url_or_path, "https://")) {
            return _.intern(url_or_path);
        }
        if (_.options.base_url.len == 0) return PlaywrightError.InvalidUrl;
        const base = std.mem.trimEnd(u8, _.options.base_url, "/");
        const path = std.mem.trimStart(u8, url_or_path, "/");
        const joined = std.fmt.allocPrint(_.allocator, "{s}/{s}", .{ base, path }) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, joined) catch {
            _.allocator.free(joined);
            return PlaywrightError.OutOfMemory;
        };
        return joined;
    }

    static fn parseMethod(m: []const u8): Method {
        if (std.mem.eql(u8, m, "POST")) return Method.POST;
        if (std.mem.eql(u8, m, "PUT")) return Method.PUT;
        if (std.mem.eql(u8, m, "DELETE")) return Method.DELETE;
        if (std.mem.eql(u8, m, "PATCH")) return Method.PATCH;
        if (std.mem.eql(u8, m, "HEAD")) return Method.HEAD;
        return Method.GET;
    }

    pub fn fetch(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        if (_.disposed) return PlaywrightError.ContextClosed;

        let current_url = _.resolveUrl(url_or_path)?;
        let current_method = if (opts.method.len > 0) opts.method else "GET";
        let current_body = opts.data;
        let current_headers = if (opts.headers_json.len > 0) opts.headers_json else "{}";

        // Encode MultipartFormData when provided
        if (mp := opts.multipart) {
            const encoded = mp.encode(_.allocator)?;
            _.owned_strings.append(_.allocator, encoded) catch {
                _.allocator.free(encoded);
                return PlaywrightError.OutOfMemory;
            };
            current_body = encoded;
            let ct_buf: [160]u8 = undefined;
            const ct = mp.contentTypeHeader(&ct_buf)?;
            const hdr = std.fmt.allocPrint(_.allocator, "{{\"Content-Type\":\"{s}\"}}", .{ct}) catch return PlaywrightError.OutOfMemory;
            _.owned_strings.append(_.allocator, hdr) catch {
                _.allocator.free(hdr);
                return PlaywrightError.OutOfMemory;
            };
            current_headers = hdr;
        }

        const effective_ignore_https = opts.ignore_https_errors orelse _.options.ignore_https_errors;
        const max_redirs: u32 = if (opts.max_redirects >= 0) opts.max_redirects as u32 else _.options.max_redirects;

        let effective_proxy: []const u8 = "";
        if (px := _.options.proxy) {
            const host = APIRequestContext.extractHost(current_url);
            if (px.server.len > 0 && !px.shouldBypass(host)) {
                effective_proxy = px.server;
            }
        }

        // Format initial HTTP/1.1 request wire frame via cap:http
        let client = HTTPClient.init(APIRequestContext.parseMethod(current_method), current_url);
        client.body = current_body;
        client.timeout_ms = opts.timeout_ms;

        let req_buf: [8192]u8 = undefined;
        const formatted = client.formatRequest(&req_buf) catch return PlaywrightError.BufferTooSmall;
        const raw_req = _.intern(formatted)?;

        _.redirect_log.clearRetainingCapacity();

        // When a transport hook is installed, execute full redirect chain tracking (301/302/303/307/308).
        if (tfn := _.transport_fn) {
            let redir_count: u32 = 0;
            while (true) {
                const res = tfn(
                    _.transport_ctx,
                    current_method,
                    current_url,
                    current_headers,
                    current_body,
                    effective_proxy,
                    effective_ignore_https,
                );

                if (APIRequestContext.isRedirectStatus(res.status)) {
                    if (loc := findHeaderInJson(res.headers_json, "location")) {
                        if (redir_count >= max_redirs) {
                            return PlaywrightError.ProtocolError;
                        }
                        const next_url = _.resolveRedirectUrl(current_url, loc)?;
                        const next_method = APIRequestContext.resolveRedirectMethod(res.status, current_method);
                        _.redirect_log.append(_.allocator, RedirectHop{
                            from_url: current_url,
                            to_url: next_url,
                            status: res.status,
                            method: next_method,
                        }) catch return PlaywrightError.OutOfMemory;

                        if (!std.ascii.eqlIgnoreCase(next_method, current_method) && std.ascii.eqlIgnoreCase(next_method, "GET")) {
                            current_body = "";
                        }
                        current_url = next_url;
                        current_method = next_method;
                        redir_count += 1;

                        // Re-evaluate proxy bypass for redirected host
                        effective_proxy = "";
                        if (px := _.options.proxy) {
                            const host = APIRequestContext.extractHost(current_url);
                            if (px.server.len > 0 && !px.shouldBypass(host)) {
                                effective_proxy = px.server;
                            }
                        }
                        continue;
                    }
                }

                const ok = res.status >= 200 && res.status < 300;
                if (opts.fail_on_status_code && !ok) {
                    return PlaywrightError.ProtocolError;
                }
                return APIResponse{
                    url: current_url,
                    status: res.status,
                    status_text: _.intern(res.status_text)?,
                    headers_json: _.intern(res.headers_json)?,
                    body: _.intern(res.body)?,
                    ok: ok,
                    raw_request: raw_req,
                    redirect_count: redir_count,
                    redirect_history: _.redirect_log.items,
                    proxy_used: effective_proxy,
                    tls_verify_disabled: effective_ignore_https,
                };
            }
        }

        return APIResponse{
            url: current_url,
            status: 200,
            status_text: "OK",
            headers_json: current_headers,
            body: current_body,
            ok: true,
            raw_request: raw_req,
            redirect_count: 0,
            redirect_history: _.redirect_log.items,
            proxy_used: effective_proxy,
            tls_verify_disabled: effective_ignore_https,
        };
    }

    static fn withMethod(opts: APIRequestFetchOptions, method: []const u8): APIRequestFetchOptions {
        return APIRequestFetchOptions{
            method: method,
            data: opts.data,
            headers_json: opts.headers_json,
            timeout_ms: opts.timeout_ms,
            max_redirects: opts.max_redirects,
            ignore_https_errors: opts.ignore_https_errors,
            multipart: opts.multipart,
            fail_on_status_code: opts.fail_on_status_code,
        };
    }

    pub fn get(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestContext.withMethod(opts, "GET"));
    }

    pub fn post(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestContext.withMethod(opts, "POST"));
    }

    pub fn put(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestContext.withMethod(opts, "PUT"));
    }

    pub fn delete(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestContext.withMethod(opts, "DELETE"));
    }
}
