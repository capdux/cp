// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/browserContext.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/context.cp — Playwright BrowserContext Abstraction
// Manages isolated pages, cookies, storageState, and context-level routes.
// ============================================================================

const std = @import("std");
import { CdpEndpoint, CdpConnectionState } from "cap:cdp";
import {
    PlaywrightError,
    Route,
    RouteEntry,
    RouteHandlerFn,
    UrlMatcher,
} from "./network";
import { Page } from "./page";
import { Tracing } from "./tracing";

pub struct Cookie {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
    pub domain: []const u8 = "",
    pub path: []const u8 = "/",
    pub expires: f64 = -1.0,
    pub http_only: bool = false,
    pub secure: bool = false,
    pub same_site: []const u8 = "Lax",
}

pub struct StorageOrigin {
    pub origin: []const u8 = "",
    pub local_storage_json: []const u8 = "[]",
}

pub struct StorageState {
    allocator: std.mem.Allocator = undefined,
    pub cookies: std.ArrayList(Cookie) = .empty,
    pub origins: std.ArrayList(StorageOrigin) = .empty,

    pub static fn init(alloc: std.mem.Allocator): StorageState {
        return StorageState{
            allocator: alloc,
            cookies: .empty,
            origins: .empty,
        };
    }

    pub fn deinit(*_) {
        _.cookies.deinit(_.allocator);
        _.cookies = .empty;
        _.origins.deinit(_.allocator);
        _.origins = .empty;
    }

    ~init() {
        _.deinit();
    }

    pub fn toJson(alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        buf.appendSlice(alloc, "{\"cookies\":[") catch return PlaywrightError.OutOfMemory;
        let i: usize = 0;
        while (i < _.cookies.items.len) {
            if (i > 0) buf.append(alloc, ',') catch return PlaywrightError.OutOfMemory;
            const c = _.cookies.items[i];
            const item = std.fmt.allocPrint(
                alloc,
                "{{\"name\":\"{s}\",\"value\":\"{s}\",\"domain\":\"{s}\",\"path\":\"{s}\"}}",
                .{ c.name, c.value, c.domain, c.path },
            ) catch return PlaywrightError.OutOfMemory;
            defer alloc.free(item);
            buf.appendSlice(alloc, item) catch return PlaywrightError.OutOfMemory;
            i += 1;
        }
        buf.appendSlice(alloc, "],\"origins\":[]}") catch return PlaywrightError.OutOfMemory;
        return buf.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }
}

pub struct BrowserContextOptions {
    viewport_width: u32 = 1280,
    viewport_height: u32 = 720,
    ignore_https_errors: bool = false,
    user_agent: []const u8 = "",
    locale: []const u8 = "en-US",
    default_timeout_ms: u32 = 30000,
}

pub struct BrowserContext {
    allocator: std.mem.Allocator = undefined,
    pub endpoint: *CdpEndpoint = undefined,
    pub context_id: []const u8 = "",
    pub options: BrowserContextOptions = .{},
    pub closed: bool = false,
    next_page_num: u64 = 1,

    pages_list: std.ArrayList(*Page) = .empty,
    cookies_list: std.ArrayList(Cookie) = .empty,
    routes: std.ArrayList(RouteEntry) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    tracing_state: Tracing = undefined,

    pub static fn init(
        alloc: std.mem.Allocator,
        endpoint: *CdpEndpoint,
        context_id: []const u8,
        opts: BrowserContextOptions,
    ): BrowserContext {
        return BrowserContext{
            allocator: alloc,
            endpoint: endpoint,
            context_id: context_id,
            options: opts,
            closed: false,
            next_page_num: 1,
            pages_list: .empty,
            cookies_list: .empty,
            routes: .empty,
            owned_strings: .empty,
            tracing_state: Tracing.init(alloc, endpoint, context_id),
        };
    }

    pub fn deinit(*_) {
        _.close();
        _.tracing_state.deinit();
        for (p of _.pages_list.items) {
            p.deinit();
            _.allocator.destroy(p);
        }
        _.pages_list.deinit(_.allocator);
        _.pages_list = .empty;
        _.cookies_list.deinit(_.allocator);
        _.cookies_list = .empty;
        _.routes.deinit(_.allocator);
        _.routes = .empty;
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

    pub fn newPage(*_): (*Page)!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;

        const page_num = _.next_page_num;
        _.next_page_num += 1;

        const sid_raw = std.fmt.allocPrint(_.allocator, "{s}-page-{d}", .{ _.context_id, page_num }) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, sid_raw) catch {
            _.allocator.free(sid_raw);
            return PlaywrightError.OutOfMemory;
        };
        const tid_raw = std.fmt.allocPrint(_.allocator, "target-{s}-{d}", .{ _.context_id, page_num }) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, tid_raw) catch {
            _.allocator.free(tid_raw);
            return PlaywrightError.OutOfMemory;
        };

        _.endpoint.createSession(sid_raw, "page", tid_raw, "") catch return PlaywrightError.OutOfMemory;

        if (_.endpoint.state == CdpConnectionState.Connected) {
            let params_buf: [256]u8 = undefined;
            let frame_buf: [512]u8 = undefined;
            const p = std.fmt.bufPrint(&params_buf, "{{\"url\":\"about:blank\"}}", .{}) catch "{}";
            const cmd_id = _.endpoint.sendCommand("", "Target.createTarget", p, &frame_buf) catch 0;
            if (cmd_id > 0) {
                * = _.endpoint.takeResponse(cmd_id);
            }
        }

        let page_ptr = _.allocator.create(Page) catch return PlaywrightError.OutOfMemory;
        page_ptr.* = Page.init(_.allocator, _.endpoint, sid_raw, tid_raw);
        page_ptr.default_timeout_ms = _.options.default_timeout_ms;

        _.pages_list.append(_.allocator, page_ptr) catch {
            page_ptr.deinit();
            _.allocator.destroy(page_ptr);
            return PlaywrightError.OutOfMemory;
        };
        return page_ptr;
    }

    pub fn pages(): []*Page {
        return _.pages_list.items;
    }

    pub fn tracing(*_): *Tracing {
        return &_.tracing_state;
    }

    pub fn cookies(): []const Cookie {
        return _.cookies_list.items;
    }

    pub fn addCookies(*_, new_cookies: []const Cookie): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        for (c of new_cookies) {
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
    }

    pub fn clearCookies(*_) {
        _.cookies_list.clearRetainingCapacity();
    }

    pub fn storageState(alloc: std.mem.Allocator): StorageState!PlaywrightError {
        let state = StorageState.init(alloc);
        for (c of _.cookies_list.items) {
            state.cookies.append(alloc, c) catch {
                state.deinit();
                return PlaywrightError.OutOfMemory;
            };
        }
        return state;
    }

    pub fn route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        const pat = _.intern(pattern)?;
        _.routes.append(_.allocator, RouteEntry{
            pattern: pat,
            handler: handler,
            ctx: ctx,
            times: -1,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn unroute(*_, pattern: []const u8) {
        let i: usize = _.routes.items.len;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.routes.items[i].pattern, pattern)) {
                * = _.routes.orderedRemove(i);
            }
        }
    }

    pub fn dispatchRoute(*_, r: *Route): bool {
        let i: usize = _.routes.items.len;
        while (i > 0) {
            i -= 1;
            const entry = _.routes.items[i];
            if (UrlMatcher.matches(entry.pattern, r.request.url)) {
                entry.handler(entry.ctx, r);
                if (r.isHandled()) return true;
            }
        }
        return false;
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        for (p of _.pages_list.items) {
            p.close();
        }
    }
}
