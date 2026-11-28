// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/page.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/page.cp — Playwright Page Abstraction
// Wraps a CDP target session via cap:cdp Domains (Page, Runtime, DOM, Input,
// Network, Accessibility) and provides Locator factories & AriaSnapshot.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpFrameScanner,
    PageDomain,
    PageNavigateParams,
    PageCaptureScreenshotParams,
    PagePrintToPdfParams,
    RuntimeDomain,
    RuntimeEvaluateParams,
    AccessibilityDomain,
    AccessibilityGetFullAXTreeParams,
} from "cap:cdp";
import {
    PlaywrightError,
    Response,
    Route,
    RouteEntry,
    RouteHandlerFn,
    UrlMatcher,
} from "./network";
import { AriaSnapshot } from "./aria";
import {
    Locator,
    ByRoleOptions,
    WaitForOptions,
    SelectorCompiler,
} from "./locator";
import { Clock } from "./clock";

pub struct GotoOptions {
    referer: []const u8 = "",
    timeout_ms: u32 = 30000,
    wait_until: []const u8 = "load",
}

pub struct ScreenshotOptions {
    format: []const u8 = "png",
    quality: u8 = 80,
    full_page: bool = false,
}

pub struct PdfOptions {
    landscape: bool = false,
    print_background: bool = true,
    scale: f64 = 1.0,
    width: f64 = 8.5,
    height: f64 = 11.0,
}

pub struct Page {
    allocator: std.mem.Allocator = undefined,
    pub endpoint: *CdpEndpoint = undefined,
    pub session_id: []const u8 = "",
    pub target_id: []const u8 = "",
    pub closed: bool = false,
    pub default_timeout_ms: u32 = 30000,

    current_url_buf: [1024]u8 = undefined,
    current_url_len: usize = 0,
    current_title_buf: [256]u8 = undefined,
    current_title_len: usize = 0,
    html_content: std.ArrayList(u8) = .empty,
    history: std.ArrayList([]u8) = .empty,
    history_idx: usize = 0,
    routes: std.ArrayList(RouteEntry) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(
        alloc: std.mem.Allocator,
        endpoint: *CdpEndpoint,
        session_id: []const u8,
        target_id: []const u8,
    ): Page {
        let p = Page{
            allocator: alloc,
            endpoint: endpoint,
            session_id: session_id,
            target_id: target_id,
            closed: false,
            default_timeout_ms: 30000,
            current_url_len: 0,
            current_title_len: 0,
            html_content: .empty,
            history: .empty,
            history_idx: 0,
            routes: .empty,
            owned_strings: .empty,
        };
        p.setUrlInternal("about:blank");
        return p;
    }

    pub fn deinit(*_) {
        _.closed = true;
        _.html_content.deinit(_.allocator);
        _.html_content = .empty;
        for (h of _.history.items) {
            _.allocator.free(h);
        }
        _.history.deinit(_.allocator);
        _.history = .empty;
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

    fn setUrlInternal(*_, u: []const u8) {
        const n = if (u.len <= 1024) u.len else 1024;
        @memcpy(_.current_url_buf[0..n], u[0..n]);
        _.current_url_len = n;
    }

    fn setTitleInternal(*_, t: []const u8) {
        const n = if (t.len <= 256) t.len else 256;
        @memcpy(_.current_title_buf[0..n], t[0..n]);
        _.current_title_len = n;
    }

    fn sendCdp(*_, method: []const u8, params_json: []const u8): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;

        let frame_buf: [4096]u8 = undefined;
        const cmd_id = _.endpoint.sendCommand(_.session_id, method, params_json, &frame_buf) catch |e| {
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

    pub fn url(): []const u8 {
        return _.current_url_buf[0.._.current_url_len];
    }

    pub fn title(): []const u8 {
        return _.current_title_buf[0.._.current_title_len];
    }

    pub fn isClosed(): bool {
        return _.closed;
    }

    pub fn goto(*_, target_url: []const u8, opts: GotoOptions): Response!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        if (target_url.len == 0) return PlaywrightError.InvalidUrl;

        let params_buf: [1536]u8 = undefined;
        const req = PageDomain.navigate(
            _.endpoint.next_id,
            _.session_id,
            PageNavigateParams{
                url: target_url,
                referrer: opts.referer,
                transition_type: "",
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;

        const nav_res = _.sendCdp(req.method, req.params_json)?;
        * = nav_res;
        _.setUrlInternal(target_url);

        const duped = _.allocator.dupe(u8, target_url) catch return PlaywrightError.OutOfMemory;
        _.history.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        _.history_idx = _.history.items.len - 1;

        return Response{
            url: _.url(),
            status: 200,
            status_text: "OK",
            ok: true,
        };
    }

    pub fn reload(*_): Response!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        * = _.sendCdp("Page.reload", "{}")?;
        return Response{
            url: _.url(),
            status: 200,
            status_text: "OK",
            ok: true,
        };
    }

    pub fn goBack(*_): Response?!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        if (_.history.items.len == 0 || _.history_idx == 0) return null;
        _.history_idx -= 1;
        _.setUrlInternal(_.history.items[_.history_idx]);
        * = _.sendCdp("Page.navigateToHistoryEntry", "{}")?;
        return Response{
            url: _.url(),
            status: 200,
            status_text: "OK",
            ok: true,
        };
    }

    pub fn goForward(*_): Response?!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        if (_.history_idx + 1 >= _.history.items.len) return null;
        _.history_idx += 1;
        _.setUrlInternal(_.history.items[_.history_idx]);
        * = _.sendCdp("Page.navigateToHistoryEntry", "{}")?;
        return Response{
            url: _.url(),
            status: 200,
            status_text: "OK",
            ok: true,
        };
    }

    pub fn setContent(*_, html: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        _.html_content.clearRetainingCapacity();
        _.html_content.appendSlice(_.allocator, html) catch return PlaywrightError.OutOfMemory;

        let params_buf: [2048]u8 = undefined;
        const p = std.fmt.bufPrint(&params_buf, "{{\"frameId\":\"{s}\",\"html\":\"{s}\"}}", .{ _.target_id, html }) catch "{}";
        * = _.sendCdp("Page.setDocumentContent", p)?;
    }

    pub fn content(): []const u8 {
        return _.html_content.items;
    }

    pub fn evaluate(*_, expression: []const u8): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [2048]u8 = undefined;
        const req = RuntimeDomain.evaluate(
            _.endpoint.next_id,
            _.session_id,
            RuntimeEvaluateParams{
                expression: expression,
                return_by_value: true,
                await_promise: true,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        return _.sendCdp(req.method, req.params_json);
    }

    pub fn locator(selector: []const u8): Locator {
        return Locator.init(_.endpoint, _.session_id, selector, _.default_timeout_ms);
    }

    pub fn getByRole(role: []const u8, opts: ByRoleOptions): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileRole(role, opts, &buf) catch role;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByText(text: []const u8, exact: bool): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileText(text, exact, &buf) catch text;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByLabel(text: []const u8, exact: bool): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileLabel(text, exact, &buf) catch text;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByPlaceholder(text: []const u8, exact: bool): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compilePlaceholder(text, exact, &buf) catch text;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByTestId(test_id: []const u8): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileTestId(test_id, &buf) catch test_id;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn click(*_, selector: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.click()?;
    }

    pub fn fill(*_, selector: []const u8, value: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.fill(value)?;
    }

    pub fn typeText(*_, selector: []const u8, text: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.fill(text)?;
    }

    pub fn press(*_, selector: []const u8, key: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.press(key)?;
    }

    pub fn waitForSelector(*_, selector: []const u8, opts: WaitForOptions): Locator!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.waitFor(opts)?;
        return loc;
    }

    pub fn route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const pat = _.allocator.dupe(u8, pattern) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, pat) catch {
            _.allocator.free(pat);
            return PlaywrightError.OutOfMemory;
        };
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

    pub fn ariaSnapshot(*_, alloc: std.mem.Allocator): AriaSnapshot!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const req = AccessibilityDomain.getFullAXTree(
            _.endpoint.next_id,
            _.session_id,
            AccessibilityGetFullAXTreeParams{ depth: -1, frame_id: "" },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.sendCdp(req.method, req.params_json)?;

        let snap = AriaSnapshot.init(alloc);
        if (nodes_arr := CdpFrameScanner.extractTopLevelField(res, "nodes", false)) {
            if (nodes_arr.len >= 2 && nodes_arr[0] == '{') {
                snap.addFromCdpAxNodeJson(nodes_arr)?;
            }
        }
        return snap;
    }

    pub fn screenshot(*_, opts: ScreenshotOptions): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const req = PageDomain.captureScreenshot(
            _.endpoint.next_id,
            _.session_id,
            PageCaptureScreenshotParams{
                format: opts.format,
                quality: opts.quality,
                from_surface: true,
                capture_beyond_viewport: opts.full_page,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        return _.sendCdp(req.method, req.params_json);
    }

    pub fn pdf(*_, opts: PdfOptions): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const req = PageDomain.printToPDF(
            _.endpoint.next_id,
            _.session_id,
            PagePrintToPdfParams{
                landscape: opts.landscape,
                print_background: opts.print_background,
                scale: opts.scale,
                paper_width: opts.width,
                paper_height: opts.height,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        return _.sendCdp(req.method, req.params_json);
    }

    pub fn clock(*_): Clock {
        return Clock.init(_.endpoint, _.session_id);
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        * = _.endpoint.detachSession(_.session_id);
    }
}
