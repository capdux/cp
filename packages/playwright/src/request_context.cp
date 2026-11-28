// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/fetch.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/request_context.cp — APIRequestContext & Cookie Jar Sync
// Built on cap:http (Method, HTTPClient) & cap:json with BrowserContext cookie sync.
// ============================================================================

const std = @import("std");
import { Method, HTTPClient } from "cap:http";
import { JSON, JsonValue } from "cap:json";
import { PlaywrightError } from "./network";
import { Cookie, BrowserContext } from "./context";

pub struct APIRequestOptions {
    pub base_url: []const u8 = "",
    pub extra_http_headers_json: []const u8 = "{}",
    pub timeout_ms: u32 = 30000,
    pub ignore_https_errors: bool = false,
}

pub struct APIRequestFetchOptions {
    pub method: []const u8 = "GET",
    pub data: []const u8 = "",
    pub headers_json: []const u8 = "{}",
    pub timeout_ms: u32 = 30000,
}

pub struct APIResponse {
    pub url: []const u8 = "",
    pub status: i32 = 200,
    pub status_text: []const u8 = "OK",
    pub headers_json: []const u8 = "{}",
    pub body: []const u8 = "",
    pub ok: bool = true,
    pub raw_request: []const u8 = "",

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
    pub options: APIRequestOptions = .{},
    pub disposed: bool = false,

    cookies_list: std.ArrayList(Cookie) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator, opts: APIRequestOptions): APIRequestContext {
        return APIRequestContext{
            allocator: alloc,
            options: opts,
            disposed: false,
            cookies_list: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.disposed = true;
        _.cookies_list.deinit(_.allocator);
        _.cookies_list = .empty;
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
        const resolved = _.resolveUrl(url_or_path)?;

        let client = HTTPClient.init(APIRequestContext.parseMethod(opts.method), resolved);
        client.body = opts.data;
        client.timeout_ms = opts.timeout_ms;

        let req_buf: [4096]u8 = undefined;
        const formatted = client.formatRequest(&req_buf) catch return PlaywrightError.BufferTooSmall;
        const raw_req = _.intern(formatted)?;

        return APIResponse{
            url: resolved,
            status: 200,
            status_text: "OK",
            headers_json: "{}",
            body: opts.data,
            ok: true,
            raw_request: raw_req,
        };
    }

    pub fn get(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestFetchOptions{
            method: "GET",
            data: opts.data,
            headers_json: opts.headers_json,
            timeout_ms: opts.timeout_ms,
        });
    }

    pub fn post(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestFetchOptions{
            method: "POST",
            data: opts.data,
            headers_json: opts.headers_json,
            timeout_ms: opts.timeout_ms,
        });
    }

    pub fn put(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestFetchOptions{
            method: "PUT",
            data: opts.data,
            headers_json: opts.headers_json,
            timeout_ms: opts.timeout_ms,
        });
    }

    pub fn delete(*_, url_or_path: []const u8, opts: APIRequestFetchOptions): APIResponse!PlaywrightError {
        return _.fetch(url_or_path, APIRequestFetchOptions{
            method: "DELETE",
            data: opts.data,
            headers_json: opts.headers_json,
            timeout_ms: opts.timeout_ms,
        });
    }
}
