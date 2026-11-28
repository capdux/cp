// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/browserContext.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/context.cp — Playwright BrowserContext Abstraction
// Manages isolated pages, cookies, storageState, permissions, geolocation,
// offline mode, HTTP credentials, CDP sessions, and context-level routes.
// ============================================================================

const std = @import("std");
import { CdpEndpoint, CdpConnectionState, CdpFrameScanner } from "cap:cdp";
import {
    PlaywrightError,
    Route,
    RouteEntry,
    RouteHandlerFn,
    UrlMatcher,
    WorkerInfo,
    PageEventRecord,
    ExposedBindingFn,
    ExposedBindingEntry,
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

pub struct Geolocation {
    pub latitude: f64 = 0.0,
    pub longitude: f64 = 0.0,
    pub accuracy: f64 = 0.0,
}

pub struct HTTPCredentials {
    pub username: []const u8 = "",
    pub password: []const u8 = "",
    pub origin: []const u8 = "",
}

pub struct HarRouteRule {
    pub har_path: []const u8 = "",
    pub url_pattern: []const u8 = "**",
}

pub struct CDPSession {
    pub endpoint: *CdpEndpoint = undefined,
    pub session_id: []const u8 = "",
    pub target_id: []const u8 = "",
    pub detached: bool = false,

    pub static fn init(endpoint: *CdpEndpoint, session_id: []const u8, target_id: []const u8): CDPSession {
        return CDPSession{
            endpoint: endpoint,
            session_id: session_id,
            target_id: target_id,
            detached: false,
        };
    }

    pub fn sessionId(): []const u8 {
        return _.session_id;
    }

    pub fn send(*_, method: []const u8, params_json: []const u8): ([]const u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;
        let frame_buf: [4096]u8 = undefined;
        const p = if (params_json.len > 0) params_json else "{}";
        const cmd_id = _.endpoint.sendCommand(_.session_id, method, p, &frame_buf) catch |e| {
            return switch (e) {
                error.NotConnected => PlaywrightError.NotConnected,
                error.ConnectionClosed => PlaywrightError.ConnectionClosed,
                error.SessionClosed => PlaywrightError.PageClosed,
                error.SessionNotFound => PlaywrightError.TargetNotFound,
                error.BufferTooSmall => PlaywrightError.BufferTooSmall,
                error.OutOfMemory => PlaywrightError.OutOfMemory,
                else => PlaywrightError.ProtocolError,
            };
        };
        if (resp := _.endpoint.takeResponse(cmd_id)) {
            if (resp.is_error) return PlaywrightError.ProtocolError;
            return resp.result_json;
        }
        return "{}";
    }

    pub fn detach(*_): void!PlaywrightError {
        if (_.detached) return;
        _.detached = true;
        if (_.endpoint.state == CdpConnectionState.Connected) {
            let params_buf: [256]u8 = undefined;
            let frame_buf: [512]u8 = undefined;
            const p = std.fmt.bufPrint(&params_buf, "{{\"sessionId\":\"{s}\"}}", .{_.session_id}) catch "{}";
            * = _.endpoint.sendCommand("", "Target.detachFromTarget", p, &frame_buf) catch 0;
        }
    }
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
    pub browser_handle: usize = 0,
    pub default_navigation_timeout_ms: u32 = 30000,
    pub is_offline: bool = false,
    pub geolocation: Geolocation? = null,
    pub http_credentials: HTTPCredentials? = null,
    pub extra_http_headers_json: []const u8 = "{}",
    next_page_num: u64 = 1,

    pages_list: std.ArrayList(*Page) = .empty,
    background_pages_list: std.ArrayList(*Page) = .empty,
    service_workers_list: std.ArrayList(WorkerInfo) = .empty,
    cookies_list: std.ArrayList(Cookie) = .empty,
    routes: std.ArrayList(RouteEntry) = .empty,
    har_routes: std.ArrayList(HarRouteRule) = .empty,
    init_scripts: std.ArrayList([]const u8) = .empty,
    exposed_bindings: std.ArrayList(ExposedBindingEntry) = .empty,
    granted_permissions: std.ArrayList([]const u8) = .empty,
    pending_events: std.ArrayList(PageEventRecord) = .empty,
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
            browser_handle: 0,
            default_navigation_timeout_ms: opts.default_timeout_ms,
            is_offline: false,
            geolocation: null,
            http_credentials: null,
            extra_http_headers_json: "{}",
            next_page_num: 1,
            pages_list: .empty,
            background_pages_list: .empty,
            service_workers_list: .empty,
            cookies_list: .empty,
            routes: .empty,
            har_routes: .empty,
            init_scripts: .empty,
            exposed_bindings: .empty,
            granted_permissions: .empty,
            pending_events: .empty,
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
        for (bp of _.background_pages_list.items) {
            bp.deinit();
            _.allocator.destroy(bp);
        }
        _.background_pages_list.deinit(_.allocator);
        _.background_pages_list = .empty;
        _.service_workers_list.deinit(_.allocator);
        _.service_workers_list = .empty;
        _.cookies_list.deinit(_.allocator);
        _.cookies_list = .empty;
        _.routes.deinit(_.allocator);
        _.routes = .empty;
        _.har_routes.deinit(_.allocator);
        _.har_routes = .empty;
        _.init_scripts.deinit(_.allocator);
        _.init_scripts = .empty;
        _.exposed_bindings.deinit(_.allocator);
        _.exposed_bindings = .empty;
        _.granted_permissions.deinit(_.allocator);
        _.granted_permissions = .empty;
        _.pending_events.deinit(_.allocator);
        _.pending_events = .empty;
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

    fn sendRootCdp(*_, method: []const u8, params_json: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        if (_.endpoint.state != CdpConnectionState.Connected) return;
        let frame_buf: [2048]u8 = undefined;
        const cmd_id = _.endpoint.sendCommand("", method, params_json, &frame_buf) catch |e| {
            return switch (e) {
                error.NotConnected => PlaywrightError.NotConnected,
                error.ConnectionClosed => PlaywrightError.ConnectionClosed,
                error.BufferTooSmall => PlaywrightError.BufferTooSmall,
                error.OutOfMemory => PlaywrightError.OutOfMemory,
                else => PlaywrightError.ProtocolError,
            };
        };
        * = _.endpoint.takeResponse(cmd_id);
    }

    pub fn browser(): usize {
        return _.browser_handle;
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

        let actual_tid: []const u8 = tid_raw;
        let actual_sid: []const u8 = sid_raw;

        if (_.endpoint.state == CdpConnectionState.Connected) {
            let params_buf: [256]u8 = undefined;
            let frame_buf: [512]u8 = undefined;
            const p = std.fmt.bufPrint(&params_buf, "{{\"url\":\"about:blank\"}}", .{}) catch "{}";
            const cmd_id = _.endpoint.sendCommand("", "Target.createTarget", p, &frame_buf) catch 0;
            if (cmd_id > 0) {
                if (resp := _.endpoint.takeResponse(cmd_id)) {
                    if (tid := CdpFrameScanner.extractTopLevelField(resp.result_json, "targetId", true)) {
                        if (tid.len > 0) {
                            actual_tid = _.intern(tid) catch tid_raw;
                        }
                    }
                    let ap_buf: [256]u8 = undefined;
                    let af_buf: [512]u8 = undefined;
                    const ap = std.fmt.bufPrint(&ap_buf, "{{\"targetId\":\"{s}\",\"flatten\":true}}", .{actual_tid}) catch "{}";
                    const attach_id = _.endpoint.sendCommand("", "Target.attachToTarget", ap, &af_buf) catch 0;
                    if (attach_id > 0) {
                        if (aresp := _.endpoint.takeResponse(attach_id)) {
                            if (sid := CdpFrameScanner.extractTopLevelField(aresp.result_json, "sessionId", true)) {
                                if (sid.len > 0) {
                                    actual_sid = _.intern(sid) catch sid_raw;
                                }
                            }
                        }
                    }
                }
            }
        }

        _.endpoint.createSession(actual_sid, "page", actual_tid, "") catch return PlaywrightError.OutOfMemory;

        let page_ptr = _.allocator.create(Page) catch return PlaywrightError.OutOfMemory;
        page_ptr.* = Page.init(_.allocator, _.endpoint, actual_sid, actual_tid);
        page_ptr.default_timeout_ms = _.options.default_timeout_ms;
        page_ptr.default_navigation_timeout_ms = _.default_navigation_timeout_ms;
        page_ptr.viewport_width = _.options.viewport_width;
        page_ptr.viewport_height = _.options.viewport_height;

        _.pages_list.append(_.allocator, page_ptr) catch {
            page_ptr.deinit();
            _.allocator.destroy(page_ptr);
            return PlaywrightError.OutOfMemory;
        };

        for (s of _.init_scripts.items) {
            page_ptr.addInitScript(s) catch {};
        }
        for (b of _.exposed_bindings.items) {
            page_ptr.exposeBinding(b.name, b.callback, b.ctx) catch {};
        }

        return page_ptr;
    }

    pub fn pages(): []*Page {
        return _.pages_list.items;
    }

    pub fn backgroundPages(): []*Page {
        return _.background_pages_list.items;
    }

    pub fn serviceWorkers(): []const WorkerInfo {
        return _.service_workers_list.items;
    }

    pub fn registerServiceWorker(*_, sw_url: []const u8, sw_target_id: []const u8): void!PlaywrightError {
        const su = _.intern(sw_url)?;
        const st = _.intern(sw_target_id)?;
        _.service_workers_list.append(_.allocator, WorkerInfo{
            url: su,
            target_id: st,
        }) catch return PlaywrightError.OutOfMemory;
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

    pub fn addInitScript(*_, script: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        const stored = _.intern(script)?;
        _.init_scripts.append(_.allocator, stored) catch return PlaywrightError.OutOfMemory;
        for (p of _.pages_list.items) {
            if (!p.isClosed()) {
                p.addInitScript(stored)?;
            }
        }
    }

    pub fn exposeBinding(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        const stored = _.intern(name)?;
        _.exposed_bindings.append(_.allocator, ExposedBindingEntry{
            name: stored,
            callback: callback,
            ctx: ctx,
        }) catch return PlaywrightError.OutOfMemory;
        for (p of _.pages_list.items) {
            if (!p.isClosed()) {
                p.exposeBinding(stored, callback, ctx)?;
            }
        }
    }

    pub fn exposeFunction(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError {
        return _.exposeBinding(name, callback, ctx);
    }

    pub fn grantPermissions(*_, permissions: []const []const u8, origin: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        let perms_buf: std.ArrayList(u8) = .empty;
        defer perms_buf.deinit(_.allocator);
        perms_buf.appendSlice(_.allocator, "{\"permissions\":[") catch return PlaywrightError.OutOfMemory;
        let i: usize = 0;
        while (i < permissions.len) {
            const perm = _.intern(permissions[i])?;
            _.granted_permissions.append(_.allocator, perm) catch return PlaywrightError.OutOfMemory;
            if (i > 0) perms_buf.append(_.allocator, ',') catch return PlaywrightError.OutOfMemory;
            perms_buf.append(_.allocator, '"') catch return PlaywrightError.OutOfMemory;
            perms_buf.appendSlice(_.allocator, perm) catch return PlaywrightError.OutOfMemory;
            perms_buf.append(_.allocator, '"') catch return PlaywrightError.OutOfMemory;
            i += 1;
        }
        if (origin.len > 0) {
            perms_buf.appendSlice(_.allocator, "],\"origin\":\"") catch return PlaywrightError.OutOfMemory;
            perms_buf.appendSlice(_.allocator, origin) catch return PlaywrightError.OutOfMemory;
            perms_buf.appendSlice(_.allocator, "\"}") catch return PlaywrightError.OutOfMemory;
        } else {
            perms_buf.appendSlice(_.allocator, "]}") catch return PlaywrightError.OutOfMemory;
        }
        _.sendRootCdp("Browser.grantPermissions", perms_buf.items)?;
    }

    pub fn clearPermissions(*_): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        _.granted_permissions.clearRetainingCapacity();
        _.sendRootCdp("Browser.resetPermissions", "{}")?;
    }

    pub fn setGeolocation(*_, geo: Geolocation?): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        _.geolocation = geo;
        for (p of _.pages_list.items) {
            if (p.isClosed() || _.endpoint.state != CdpConnectionState.Connected) continue;
            let frame_buf: [512]u8 = undefined;
            if (g := geo) {
                let params_buf: [256]u8 = undefined;
                const params = std.fmt.bufPrint(
                    &params_buf,
                    "{{\"latitude\":{d},\"longitude\":{d},\"accuracy\":{d}}}",
                    .{ g.latitude, g.longitude, g.accuracy },
                ) catch return PlaywrightError.BufferTooSmall;
                const cmd_id = _.endpoint.sendCommand(p.session_id, "Emulation.setGeolocationOverride", params, &frame_buf) catch return PlaywrightError.ProtocolError;
                const r_geo = _.endpoint.takeResponse(cmd_id);
                * = r_geo;
            } else {
                const cmd_id = _.endpoint.sendCommand(p.session_id, "Emulation.clearGeolocationOverride", "{}", &frame_buf) catch return PlaywrightError.ProtocolError;
                const r_clr = _.endpoint.takeResponse(cmd_id);
                * = r_clr;
            }
        }
    }

    pub fn setExtraHTTPHeaders(*_, headers_json: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        const stored = _.intern(if (headers_json.len > 0) headers_json else "{}")?;
        _.extra_http_headers_json = stored;
        for (p of _.pages_list.items) {
            if (!p.isClosed()) {
                p.setExtraHTTPHeaders(stored)?;
            }
        }
    }

    pub fn setOffline(*_, offline: bool): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        _.is_offline = offline;
        const offline_str: []const u8 = if (offline) "true" else "false";
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"offline\":{s},\"latency\":0,\"downloadThroughput\":-1,\"uploadThroughput\":-1}}",
            .{offline_str},
        ) catch return PlaywrightError.BufferTooSmall;
        for (p of _.pages_list.items) {
            if (p.isClosed() || _.endpoint.state != CdpConnectionState.Connected) continue;
            let frame_buf: [512]u8 = undefined;
            const cmd_id = _.endpoint.sendCommand(p.session_id, "Network.emulateNetworkConditions", params, &frame_buf) catch return PlaywrightError.ProtocolError;
            const r_net = _.endpoint.takeResponse(cmd_id);
            * = r_net;
        }
    }

    pub fn setHTTPCredentials(*_, creds: HTTPCredentials?): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        if (c := creds) {
            const u = _.intern(c.username)?;
            const pw = _.intern(c.password)?;
            const orig = _.intern(c.origin)?;
            _.http_credentials = HTTPCredentials{
                username: u,
                password: pw,
                origin: orig,
            };
        } else {
            _.http_credentials = null;
        }
    }

    pub fn setDefaultTimeout(*_, timeout_ms: u32) {
        _.options.default_timeout_ms = timeout_ms;
        for (p of _.pages_list.items) {
            p.default_timeout_ms = timeout_ms;
        }
    }

    pub fn setDefaultNavigationTimeout(*_, timeout_ms: u32) {
        _.default_navigation_timeout_ms = timeout_ms;
        for (p of _.pages_list.items) {
            p.default_navigation_timeout_ms = timeout_ms;
        }
    }

    pub fn routeFromHAR(*_, har_path: []const u8, url_pattern: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        if (har_path.len == 0) return PlaywrightError.InvalidUrl;
        const hp = _.intern(har_path)?;
        const pat = _.intern(if (url_pattern.len > 0) url_pattern else "**")?;
        _.har_routes.append(_.allocator, HarRouteRule{
            har_path: hp,
            url_pattern: pat,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn emitEvent(*_, event_name: []const u8, payload: []const u8): void!PlaywrightError {
        const n = _.intern(event_name)?;
        const p = _.intern(payload)?;
        _.pending_events.append(_.allocator, PageEventRecord{
            name: n,
            payload: p,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn waitForEvent(*_, event_name: []const u8, timeout_ms: u32): ([]const u8)!PlaywrightError {
        * = timeout_ms;
        if (_.closed) return PlaywrightError.ContextClosed;
        let i: usize = 0;
        while (i < _.pending_events.items.len) {
            if (std.mem.eql(u8, _.pending_events.items[i].name, event_name)) {
                const payload = _.pending_events.items[i].payload;
                * = _.pending_events.orderedRemove(i);
                return payload;
            }
            i += 1;
        }
        return PlaywrightError.Timeout;
    }

    pub fn newCDPSession(*_, page: *Page): CDPSession!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        if (page.isClosed()) return PlaywrightError.PageClosed;
        if (_.endpoint.state == CdpConnectionState.Connected) {
            let params_buf: [256]u8 = undefined;
            let frame_buf: [512]u8 = undefined;
            const p = std.fmt.bufPrint(&params_buf, "{{\"targetId\":\"{s}\",\"flatten\":true}}", .{page.target_id}) catch "{}";
            const cmd_id = _.endpoint.sendCommand("", "Target.attachToTarget", p, &frame_buf) catch return PlaywrightError.ProtocolError;
            const r_att = _.endpoint.takeResponse(cmd_id);
            * = r_att;
        }
        return CDPSession.init(_.endpoint, page.session_id, page.target_id);
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
