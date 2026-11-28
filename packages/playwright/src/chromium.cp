// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/chromium.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/chromium.cp — Chromium BrowserType Launcher & CDP Connect
// Zero-copy reuses cap:cdp CdpBrowserLauncher (chromium/google-chrome/
// chromium-browser/msedge/capsurf) and CdpWsEndpoint URL parser.
// ============================================================================

const std = @import("std");
import {
    CdpBrowserLaunchOptions,
    CdpBrowserLauncher,
    CdpWsEndpoint,
    CdpSendTransportFn,
    CdpNativeWsTransport,
} from "cap:cdp";
import { PlaywrightError } from "./network";
import { Browser } from "./browser";

pub struct LaunchOptions {
    headless: bool = true,
    executable_path: []const u8 = "",
    user_data_dir: []const u8 = "",
    args: []const []const u8 = &.{},
    viewport_width: u32 = 1280,
    viewport_height: u32 = 720,
    timeout_ms: u32 = 30000,
    remote_debugging_port: u16 = 0,
    ignore_https_errors: bool = false,
    require_executable: bool = false,
    transport_fn: CdpSendTransportFn? = null,
    transport_ctx: usize = 0,
}

pub struct ConnectOverCdpOptions {
    timeout_ms: u32 = 30000,
    transport_fn: CdpSendTransportFn? = null,
    transport_ctx: usize = 0,
}

pub struct ChromiumBrowserType {
    pub static fn name(): []const u8 {
        return "chromium";
    }

    /// Probe system browser executable via cap:cdp CdpBrowserLauncher.
    pub static fn executablePath(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)? {
        return CdpBrowserLauncher.probeBrowserExecutable(alloc, preferred);
    }

    /// Launch a Chromium/Capsurf browser instance (builds CDP flags via CdpBrowserLauncher).
    pub static fn launch(alloc: std.mem.Allocator, opts: LaunchOptions): Browser!PlaywrightError {
        let browser = Browser.init(alloc);
        errdefer browser.deinit();

        const cdp_opts = CdpBrowserLaunchOptions{
            headless: opts.headless,
            executable_path: opts.executable_path,
            user_data_dir: opts.user_data_dir,
            args: opts.args,
            viewport_width: opts.viewport_width,
            viewport_height: opts.viewport_height,
            timeout_ms: opts.timeout_ms,
            remote_debugging_port: opts.remote_debugging_port,
            ignore_https_errors: opts.ignore_https_errors,
        };

        const built_args = CdpBrowserLauncher.buildLaunchArgs(alloc, cdp_opts) catch {
            return PlaywrightError.OutOfMemory;
        };
        browser.launch_args = built_args;

        if (exe := CdpBrowserLauncher.probeBrowserExecutable(alloc, opts.executable_path)) {
            defer alloc.free(exe);
            browser.setExecutablePath(exe)?;
        } else if (opts.require_executable) {
            return PlaywrightError.ExecutableNotFound;
        }

        if (cb := opts.transport_fn) {
            browser.setTransport(cb, opts.transport_ctx);
        } else {
            let tr = alloc.create(CdpNativeWsTransport) catch return PlaywrightError.OutOfMemory;
            tr.* = CdpNativeWsTransport.init(alloc);
            tr.launchAndConnect(cdp_opts) catch |e| {
                tr.deinit();
                alloc.destroy(tr);
                return switch (e) {
                    error.OutOfMemory => PlaywrightError.OutOfMemory,
                    error.Timeout => PlaywrightError.Timeout,
                    error.InvalidFrame => PlaywrightError.InvalidUrl,
                    error.NotConnected => PlaywrightError.NotConnected,
                    else => PlaywrightError.ExecutableNotFound,
                };
            };
            browser.attachNativeTransport(tr);
            if (tr.wsUrl().len > 0) {
                browser.setWsEndpoint(tr.wsUrl())?;
            }
        }

        return browser;
    }

    /// Launch with an explicit custom transport callback (for Mock unit tests).
    pub static fn launchWithTransport(
        alloc: std.mem.Allocator,
        opts: LaunchOptions,
        cb: CdpSendTransportFn,
        ctx: usize,
    ): Browser!PlaywrightError {
        return ChromiumBrowserType.launch(alloc, LaunchOptions{
            headless: opts.headless,
            executable_path: opts.executable_path,
            user_data_dir: opts.user_data_dir,
            args: opts.args,
            viewport_width: opts.viewport_width,
            viewport_height: opts.viewport_height,
            timeout_ms: opts.timeout_ms,
            remote_debugging_port: opts.remote_debugging_port,
            ignore_https_errors: opts.ignore_https_errors,
            require_executable: opts.require_executable,
            transport_fn: cb,
            transport_ctx: ctx,
        });
    }

    /// Connect to an existing browser over Chrome DevTools Protocol (`ws://...` or `wss://...`).
    pub static fn connectOverCDP(
        alloc: std.mem.Allocator,
        endpoint_url: []const u8,
        opts: ConnectOverCdpOptions,
    ): Browser!PlaywrightError {
        * = opts.timeout_ms;
        const parsed: CdpWsEndpoint = CdpBrowserLauncher.parseWsUrl(endpoint_url) orelse return PlaywrightError.InvalidUrl;

        let browser = Browser.init(alloc);
        errdefer browser.deinit();

        browser.setWsEndpoint(parsed.url)?;
        if (cb := opts.transport_fn) {
            browser.setTransport(cb, opts.transport_ctx);
        } else {
            let tr = alloc.create(CdpNativeWsTransport) catch return PlaywrightError.OutOfMemory;
            tr.* = CdpNativeWsTransport.init(alloc);
            tr.connectWs(parsed.url) catch |e| {
                tr.deinit();
                alloc.destroy(tr);
                return switch (e) {
                    error.OutOfMemory => PlaywrightError.OutOfMemory,
                    error.InvalidFrame => PlaywrightError.InvalidUrl,
                    error.NotConnected => PlaywrightError.NotConnected,
                    else => PlaywrightError.ProtocolError,
                };
            };
            browser.attachNativeTransport(tr);
        }
        return browser;
    }

    /// Connect over CDP with an explicit custom transport callback (for Mock unit tests).
    pub static fn connectOverCDPWithTransport(
        alloc: std.mem.Allocator,
        endpoint_url: []const u8,
        opts: ConnectOverCdpOptions,
        cb: CdpSendTransportFn,
        ctx: usize,
    ): Browser!PlaywrightError {
        return ChromiumBrowserType.connectOverCDP(alloc, endpoint_url, ConnectOverCdpOptions{
            timeout_ms: opts.timeout_ms,
            transport_fn: cb,
            transport_ctx: ctx,
        });
    }
}

pub const chromium = ChromiumBrowserType;
