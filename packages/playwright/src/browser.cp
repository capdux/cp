// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/browser.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/browser.cp — Playwright Browser Abstraction
// Owns CdpEndpoint & BrowserContext lifecycle with zero-leak RAII cleanup.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpBrowserLauncher,
    CdpSendTransportFn,
    CdpNativeWsTransport,
} from "cap:cdp";
import { PlaywrightError } from "./network";
import { Page } from "./page";
import { BrowserContext, BrowserContextOptions } from "./context";

pub struct Browser {
    allocator: std.mem.Allocator = undefined,
    pub endpoint: CdpEndpoint = undefined,
    pub version_str: []const u8 = "130.0.6723.0",
    pub ws_endpoint: []const u8 = "",
    pub executable_path: []const u8 = "",
    pub launch_args: [][]u8 = &.{},
    pub closed: bool = false,
    next_ctx_num: u64 = 1,

    contexts_list: std.ArrayList(*BrowserContext) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    native_transport: (*CdpNativeWsTransport)? = null,

    pub static fn init(alloc: std.mem.Allocator): Browser {
        return Browser{
            allocator: alloc,
            endpoint: CdpEndpoint.init(alloc),
            version_str: "130.0.6723.0",
            ws_endpoint: "",
            executable_path: "",
            launch_args: &.{},
            closed: false,
            next_ctx_num: 1,
            contexts_list: .empty,
            owned_strings: .empty,
            native_transport: null,
        };
    }

    pub fn deinit(*_) {
        _.close();
        if (tr := _.native_transport) {
            tr.deinit();
            _.allocator.destroy(tr);
            _.native_transport = null;
        }
        for (ctx of _.contexts_list.items) {
            ctx.deinit();
            _.allocator.destroy(ctx);
        }
        _.contexts_list.deinit(_.allocator);
        _.contexts_list = .empty;

        if (_.launch_args.len > 0) {
            CdpBrowserLauncher.freeLaunchArgs(_.allocator, _.launch_args);
            _.launch_args = &.{};
        }

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;

        _.endpoint.deinit();
    }

    ~init() {
        _.deinit();
    }

    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize) {
        _.endpoint.setTransport(cb, ctx);
        _.endpoint.markConnected();
    }

    pub fn attachNativeTransport(*_, tr: *CdpNativeWsTransport) {
        _.native_transport = tr;
        tr.attachEndpoint(&_.endpoint);
    }

    pub fn setWsEndpoint(*_, url: []const u8): void!PlaywrightError {
        if (url.len == 0) return;
        const duped = _.allocator.dupe(u8, url) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        _.ws_endpoint = duped;
    }

    pub fn setExecutablePath(*_, path: []const u8): void!PlaywrightError {
        if (path.len == 0) return;
        const duped = _.allocator.dupe(u8, path) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        _.executable_path = duped;
    }

    pub fn newContext(*_, opts: BrowserContextOptions): (*BrowserContext)!PlaywrightError {
        if (_.closed) return PlaywrightError.BrowserClosed;

        const n = _.next_ctx_num;
        _.next_ctx_num += 1;

        const cid = std.fmt.allocPrint(_.allocator, "ctx-{d}", .{n}) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, cid) catch {
            _.allocator.free(cid);
            return PlaywrightError.OutOfMemory;
        };

        let ctx_ptr = _.allocator.create(BrowserContext) catch return PlaywrightError.OutOfMemory;
        ctx_ptr.* = BrowserContext.init(_.allocator, &_.endpoint, cid, opts);

        _.contexts_list.append(_.allocator, ctx_ptr) catch {
            ctx_ptr.deinit();
            _.allocator.destroy(ctx_ptr);
            return PlaywrightError.OutOfMemory;
        };
        return ctx_ptr;
    }

    pub fn newPage(*_): (*Page)!PlaywrightError {
        if (_.closed) return PlaywrightError.BrowserClosed;
        const ctx = _.newContext(.{})?;
        return ctx.newPage();
    }

    pub fn contexts(): []*BrowserContext {
        return _.contexts_list.items;
    }

    pub fn version(): []const u8 {
        return _.version_str;
    }

    pub fn isConnected(): bool {
        return !_.closed && _.endpoint.state == CdpConnectionState.Connected;
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        for (ctx of _.contexts_list.items) {
            ctx.close();
        }
        _.endpoint.close();
        if (tr := _.native_transport) {
            tr.close();
        }
    }
}
