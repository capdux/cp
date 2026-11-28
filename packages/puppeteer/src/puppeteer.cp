// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/node/PuppeteerNode.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/puppeteer.cp — Top-Level Puppeteer Facade
// launch, connect, defaultExecutablePath, buildLaunchArgs & parseDevToolsWsUrl
// ============================================================================

const std = @import("std");
import {
    CdpSendTransportFn,
    CdpWsEndpoint,
    CdpBrowserLauncher,
    CdpNativeWsTransport,
} from "cap:cdp";
import {
    PuppeteerError,
    LaunchOptions,
    ConnectOptions,
    mapCdpError,
} from "./types";
import { Browser } from "./browser";

pub struct Puppeteer {
    pub static fn defaultExecutablePath(alloc: std.mem.Allocator): ([]u8)? {
        return CdpBrowserLauncher.probeBrowserExecutable(alloc, "");
    }

    pub static fn resolveExecutablePath(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)? {
        return CdpBrowserLauncher.probeBrowserExecutable(alloc, preferred);
    }

    pub static fn buildLaunchArgs(alloc: std.mem.Allocator, options: LaunchOptions): ([]([]u8))!PuppeteerError {
        const cdp_opts = options.toCdpOptions();
        return CdpBrowserLauncher.buildLaunchArgs(alloc, cdp_opts) catch |e| {
            return mapCdpError(e);
        };
    }

    pub static fn freeLaunchArgs(alloc: std.mem.Allocator, args: [][]u8) {
        CdpBrowserLauncher.freeLaunchArgs(alloc, args);
    }

    pub static fn parseDevToolsWsUrl(stderr_chunk: []const u8): CdpWsEndpoint? {
        return CdpBrowserLauncher.parseDevToolsWsUrl(stderr_chunk);
    }

    pub static fn launchWithTransport(
        alloc: std.mem.Allocator,
        options: LaunchOptions,
        ws_url: []const u8,
        transport_fn: CdpSendTransportFn,
        transport_ctx: usize,
    ): Browser!PuppeteerError {
        const args = Puppeteer.buildLaunchArgs(alloc, options)?;
        let browser = Browser.init(alloc, ws_url, options.executable_path) catch |e| {
            CdpBrowserLauncher.freeLaunchArgs(alloc, args);
            return e;
        };
        browser.launch_args = args;
        browser.setTransport(transport_fn, transport_ctx);
        return browser;
    }

    pub static fn launch(alloc: std.mem.Allocator, options: LaunchOptions): Browser!PuppeteerError {
        const exe = CdpBrowserLauncher.probeBrowserExecutable(alloc, options.executable_path) orelse {
            return PuppeteerError.ExecutableNotFound;
        };
        defer alloc.free(exe);

        let wt = alloc.create(CdpNativeWsTransport) catch return PuppeteerError.OutOfMemory;
        wt.* = CdpNativeWsTransport.init(alloc);

        let cdp_opts = options.toCdpOptions();
        cdp_opts.executable_path = exe;
        wt.launchAndConnect(cdp_opts) catch |e| {
            wt.close();
            alloc.destroy(wt);
            return mapCdpError(e);
        };

        let browser = Browser.init(alloc, wt.wsUrl(), exe) catch |e| {
            wt.close();
            alloc.destroy(wt);
            return e;
        };
        browser.attachNativeTransport(wt);
        return browser;
    }

    pub static fn connectWithTransport(
        alloc: std.mem.Allocator,
        options: ConnectOptions,
        transport_fn: CdpSendTransportFn,
        transport_ctx: usize,
    ): Browser!PuppeteerError {
        if (options.browser_ws_url.len == 0) {
            return PuppeteerError.InvalidArgument;
        }
        let browser = Browser.init(alloc, options.browser_ws_url, "")?;
        browser.setTransport(transport_fn, transport_ctx);
        return browser;
    }

    pub static fn connect(alloc: std.mem.Allocator, browser_ws_url: []const u8): Browser!PuppeteerError {
        if (browser_ws_url.len == 0) {
            return PuppeteerError.InvalidArgument;
        }
        let wt = alloc.create(CdpNativeWsTransport) catch return PuppeteerError.OutOfMemory;
        wt.* = CdpNativeWsTransport.init(alloc);

        wt.connectWs(browser_ws_url) catch |e| {
            wt.close();
            alloc.destroy(wt);
            return mapCdpError(e);
        };

        let browser = Browser.init(alloc, browser_ws_url, "") catch |e| {
            wt.close();
            alloc.destroy(wt);
            return e;
        };
        browser.attachNativeTransport(wt);
        return browser;
    }
}

pub fn launch(alloc: std.mem.Allocator, options: LaunchOptions): Browser!PuppeteerError {
    return Puppeteer.launch(alloc, options);
}

pub fn launchWithTransport(
    alloc: std.mem.Allocator,
    options: LaunchOptions,
    ws_url: []const u8,
    transport_fn: CdpSendTransportFn,
    transport_ctx: usize,
): Browser!PuppeteerError {
    return Puppeteer.launchWithTransport(alloc, options, ws_url, transport_fn, transport_ctx);
}

pub fn connect(alloc: std.mem.Allocator, browser_ws_url: []const u8): Browser!PuppeteerError {
    return Puppeteer.connect(alloc, browser_ws_url);
}

pub fn connectWithTransport(
    alloc: std.mem.Allocator,
    options: ConnectOptions,
    transport_fn: CdpSendTransportFn,
    transport_ctx: usize,
): Browser!PuppeteerError {
    return Puppeteer.connectWithTransport(alloc, options, transport_fn, transport_ctx);
}

pub fn defaultExecutablePath(alloc: std.mem.Allocator): ([]u8)? {
    return Puppeteer.defaultExecutablePath(alloc);
}
