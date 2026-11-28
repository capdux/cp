// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/Browser.ts & BrowserContext.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/browser.cp — Browser & BrowserContext
// Manages CdpEndpoint lifecycle, multi-context isolation, Page creation & RAII
// ============================================================================

const std = @import("std");
import {
    CdpConnectionState,
    CdpEndpoint,
    CdpSendTransportFn,
    CdpBrowserLauncher,
    CdpFrameScanner,
} from "cap:cdp";
import {
    PuppeteerError,
    TargetType,
    TargetInfo,
    Cookie,
    CookieParam,
    mapCdpError,
} from "./types";
import { Page } from "./page";

pub struct BrowserContext {
    id_str: []const u8 = "default",
    incognito: bool = false,
    closed_flag: bool = false,
    endpoint: *CdpEndpoint = undefined,

    pub static fn init(endpoint: *CdpEndpoint, ctx_id: []const u8, incognito: bool): BrowserContext {
        return BrowserContext{
            id_str: ctx_id,
            incognito: incognito,
            closed_flag: false,
            endpoint: endpoint,
        };
    }

    pub fn id(): []const u8 {
        return _.id_str;
    }

    pub fn isIncognito(): bool {
        return _.incognito;
    }

    pub fn isClosed(): bool {
        return _.closed_flag;
    }

    pub fn overridePermissions(*_, origin: []const u8, permissions_json: []const u8): void!PuppeteerError {
        if (_.closed_flag) return PuppeteerError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"origin\":\"{s}\",\"permissions\":{s},\"browserContextId\":\"{s}\"}}",
            .{ origin, permissions_json, _.id_str },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_perm = _.endpoint.sendCommand("", "Browser.grantPermissions", params, &fbuf) catch |e| {
            return mapCdpError(e);
        };
        * = _ign_perm;
    }

    pub fn close(*_): void!PuppeteerError {
        if (_.closed_flag) return;
        _.closed_flag = true;
        if (_.incognito && _.endpoint.state == CdpConnectionState.Connected) {
            let pbuf: [256]u8 = undefined;
            let fbuf: [512]u8 = undefined;
            const params = std.fmt.bufPrint(
                &pbuf,
                "{{\"browserContextId\":\"{s}\"}}",
                .{_.id_str},
            ) catch return PuppeteerError.BufferTooSmall;
            const _ign_1 = _.endpoint.sendCommand("", "Target.disposeBrowserContext", params, &fbuf) catch 0;
            * = _ign_1;
        }
    }
}

pub struct Browser {
    allocator: std.mem.Allocator = undefined,
    endpoint: *CdpEndpoint = undefined,
    ws_endpoint_buf: [512]u8 = undefined,
    ws_endpoint_len: usize = 0,
    executable_path_buf: [512]u8 = undefined,
    executable_path_len: usize = 0,
    pages_list: std.ArrayList(*Page) = .empty,
    contexts_list: std.ArrayList(BrowserContext) = .empty,
    targets_list: std.ArrayList(TargetInfo) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    launch_args: ([][]u8)? = null,
    next_seq: u64 = 1,
    closed: bool = false,

    pub static fn init(
        alloc: std.mem.Allocator,
        ws_url: []const u8,
        executable_path: []const u8,
    ): Browser!PuppeteerError {
        let ep = alloc.create(CdpEndpoint) catch return PuppeteerError.OutOfMemory;
        ep.* = CdpEndpoint.init(alloc);

        let b = Browser{
            allocator: alloc,
            endpoint: ep,
            ws_endpoint_len: 0,
            executable_path_len: 0,
            pages_list: .empty,
            contexts_list: .empty,
            targets_list: .empty,
            owned_strings: .empty,
            launch_args: null,
            next_seq: 1,
            closed: false,
        };

        const wn = if (ws_url.len <= 512) ws_url.len else 512;
        if (wn > 0) @memcpy(b.ws_endpoint_buf[0..wn], ws_url[0..wn]);
        b.ws_endpoint_len = wn;

        const en = if (executable_path.len <= 512) executable_path.len else 512;
        if (en > 0) @memcpy(b.executable_path_buf[0..en], executable_path[0..en]);
        b.executable_path_len = en;

        b.contexts_list.append(alloc, BrowserContext.init(ep, "default", false)) catch {
            ep.deinit();
            alloc.destroy(ep);
            return PuppeteerError.OutOfMemory;
        };
        b.targets_list.append(alloc, TargetInfo{
            target_id: "browser",
            target_type: TargetType.Browser,
            title: "Browser",
            url: "",
            attached: true,
            browser_context_id: "default",
        }) catch {};

        return b;
    }

    pub fn deinit(*_) {
        if (_.closed) return;
        _.closed = true;

        for (p of _.pages_list.items) {
            p.deinit();
            _.allocator.destroy(p);
        }
        _.pages_list.deinit(_.allocator);
        _.pages_list = .empty;

        _.contexts_list.deinit(_.allocator);
        _.contexts_list = .empty;

        _.targets_list.deinit(_.allocator);
        _.targets_list = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;

        if (args := _.launch_args) {
            CdpBrowserLauncher.freeLaunchArgs(_.allocator, args);
            _.launch_args = null;
        }

        _.endpoint.close();
        _.endpoint.deinit();
        _.allocator.destroy(_.endpoint);
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PuppeteerError {
        const duped = _.allocator.dupe(u8, s) catch return PuppeteerError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PuppeteerError.OutOfMemory;
        };
        return duped;
    }

    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize) {
        _.endpoint.setTransport(cb, ctx);
        _.endpoint.markConnected();
    }

    pub fn feedIncoming(*_, payload: []const u8): void!PuppeteerError {
        const _ign_in = _.endpoint.handleIncomingFrame(payload) catch |e| {
            return mapCdpError(e);
        };
        * = _ign_in;
    }

    pub fn wsEndpoint(): []const u8 {
        return _.ws_endpoint_buf[0.._.ws_endpoint_len];
    }

    pub fn executablePath(): []const u8 {
        return _.executable_path_buf[0.._.executable_path_len];
    }

    pub fn isConnected(): bool {
        if (_.closed) return false;
        return _.endpoint.state == CdpConnectionState.Connected;
    }

    pub fn defaultBrowserContext(*_): *BrowserContext {
        return &_.contexts_list.items[0];
    }

    pub fn browserContexts(): []const BrowserContext {
        return _.contexts_list.items;
    }

    pub fn createBrowserContext(*_): BrowserContext!PuppeteerError {
        if (!_.isConnected()) return PuppeteerError.NotConnected;
        let pbuf: [128]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const req_id = _.endpoint.sendCommand("", "Target.createBrowserContext", "{}", &fbuf) catch |e| {
            return mapCdpError(e);
        };
        let ctx_id_raw: []const u8 = "";
        if (resp := _.endpoint.takeResponse(req_id)) {
            if (!resp.is_error) {
                ctx_id_raw = CdpFrameScanner.extractTopLevelField(resp.result_json, "browserContextId", true) orelse "";
            }
        }
        let fallback_buf: [64]u8 = undefined;
        if (ctx_id_raw.len == 0) {
            ctx_id_raw = std.fmt.bufPrint(&fallback_buf, "ctx-{d}", .{_.next_seq}) catch "ctx";
            _.next_seq += 1;
        }
        * = pbuf;
        const ctx_id = _.intern(ctx_id_raw)?;
        const bc = BrowserContext.init(_.endpoint, ctx_id, true);
        _.contexts_list.append(_.allocator, bc) catch return PuppeteerError.OutOfMemory;
        return bc;
    }

    pub fn newPage(*_): (*Page)!PuppeteerError {
        if (!_.isConnected()) return PuppeteerError.NotConnected;

        let fbuf: [512]u8 = undefined;
        const create_id = _.endpoint.sendCommand(
            "",
            "Target.createTarget",
            "{\"url\":\"about:blank\"}",
            &fbuf,
        ) catch |e| {
            return mapCdpError(e);
        };

        let tid_raw: []const u8 = "";
        if (resp := _.endpoint.takeResponse(create_id)) {
            if (!resp.is_error) {
                tid_raw = CdpFrameScanner.extractTopLevelField(resp.result_json, "targetId", true) orelse "";
            }
        }
        let tid_fallback: [64]u8 = undefined;
        if (tid_raw.len == 0) {
            tid_raw = std.fmt.bufPrint(&tid_fallback, "target-{d}", .{_.next_seq}) catch "target";
            _.next_seq += 1;
        }
        const tid = _.intern(tid_raw)?;

        let pbuf: [256]u8 = undefined;
        const attach_params = std.fmt.bufPrint(
            &pbuf,
            "{{\"targetId\":\"{s}\",\"flatten\":true}}",
            .{tid},
        ) catch return PuppeteerError.BufferTooSmall;
        const attach_id = _.endpoint.sendCommand("", "Target.attachToTarget", attach_params, &fbuf) catch |e| {
            return mapCdpError(e);
        };

        let sid_raw: []const u8 = "";
        if (ar := _.endpoint.takeResponse(attach_id)) {
            if (!ar.is_error) {
                sid_raw = CdpFrameScanner.extractTopLevelField(ar.result_json, "sessionId", true) orelse "";
            }
        }
        let sid_fallback: [64]u8 = undefined;
        if (sid_raw.len == 0) {
            sid_raw = std.fmt.bufPrint(&sid_fallback, "session-{d}", .{_.next_seq}) catch "session";
            _.next_seq += 1;
        }
        const sid = _.intern(sid_raw)?;

        _.endpoint.createSession(sid, "page", tid, "") catch |e| {
            return mapCdpError(e);
        };

        let page_ptr = _.allocator.create(Page) catch return PuppeteerError.OutOfMemory;
        page_ptr.* = Page.init(_.endpoint, sid, tid);
        page_ptr.bindSelf();
        _.pages_list.append(_.allocator, page_ptr) catch {
            page_ptr.deinit();
            _.allocator.destroy(page_ptr);
            return PuppeteerError.OutOfMemory;
        };

        _.targets_list.append(_.allocator, TargetInfo{
            target_id: tid,
            target_type: TargetType.Page,
            title: "",
            url: "about:blank",
            attached: true,
            browser_context_id: "default",
        }) catch {};

        return page_ptr;
    }

    pub fn pages(): []*Page {
        return _.pages_list.items;
    }

    pub fn targets(): []const TargetInfo {
        return _.targets_list.items;
    }

    pub fn target(): TargetInfo {
        return _.targets_list.items[0];
    }

    pub fn version(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        if (!_.isConnected()) return PuppeteerError.NotConnected;
        let fbuf: [512]u8 = undefined;
        const req_id = _.endpoint.sendCommand("", "Browser.getVersion", "{}", &fbuf) catch |e| {
            return mapCdpError(e);
        };
        if (resp := _.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ProtocolError;
            const prod = CdpFrameScanner.extractTopLevelField(resp.result_json, "product", true) orelse "Chrome/131.0.0.0";
            const n = if (prod.len <= out_buf.len) prod.len else out_buf.len;
            if (n > 0) @memcpy(out_buf[0..n], prod[0..n]);
            return out_buf[0..n];
        }
        const fallback = "Chrome/131.0.0.0";
        const n = if (fallback.len <= out_buf.len) fallback.len else out_buf.len;
        @memcpy(out_buf[0..n], fallback[0..n]);
        return out_buf[0..n];
    }

    pub fn userAgent(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        if (!_.isConnected()) return PuppeteerError.NotConnected;
        let fbuf: [512]u8 = undefined;
        const req_id = _.endpoint.sendCommand("", "Browser.getVersion", "{}", &fbuf) catch |e| {
            return mapCdpError(e);
        };
        if (resp := _.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ProtocolError;
            const ua = CdpFrameScanner.extractTopLevelField(resp.result_json, "userAgent", true)
                orelse "Mozilla/5.0 HeadlessChrome/131.0.0.0";
            const n = if (ua.len <= out_buf.len) ua.len else out_buf.len;
            if (n > 0) @memcpy(out_buf[0..n], ua[0..n]);
            return out_buf[0..n];
        }
        const fallback = "Mozilla/5.0 HeadlessChrome/131.0.0.0";
        const n = if (fallback.len <= out_buf.len) fallback.len else out_buf.len;
        @memcpy(out_buf[0..n], fallback[0..n]);
        return out_buf[0..n];
    }

    pub fn disconnect(*_) {
        _.endpoint.close();
    }

    pub fn close(*_): void!PuppeteerError {
        if (_.closed) return;
        if (_.endpoint.state == CdpConnectionState.Connected) {
            let fbuf: [512]u8 = undefined;
            const _ign_2 = _.endpoint.sendCommand("", "Browser.close", "{}", &fbuf) catch 0;
            * = _ign_2;
        }
        _.deinit();
    }
}
