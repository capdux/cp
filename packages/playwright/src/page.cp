// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/page.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/page.cp — Playwright Page Abstraction
// Wraps a CDP target session via cap:cdp Domains (Page, Runtime, DOM, Input,
// Network, Emulation, Accessibility) with full DOM, emulation & wait APIs.
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
    Request,
    Response,
    Route,
    RouteEntry,
    RouteHandlerFn,
    WebSocketRoute,
    WebSocketRouteEntry,
    WebSocketRouteHandlerFn,
    UrlMatcher,
    ScriptTagOptions,
    StyleTagOptions,
    WorkerInfo,
    PageEventRecord,
    ExposedBindingFn,
    ExposedBindingEntry,
} from "./network";
import { AriaSnapshot } from "./aria";
import {
    Locator,
    ByRoleOptions,
    WaitForOptions,
    SelectorCompiler,
} from "./locator";
import { Clock } from "./clock";
import { Frame } from "./frames";
import { Coverage } from "./coverage";
import { Video } from "./artifacts";

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

pub struct ViewportSize {
    pub width: u32 = 1280,
    pub height: u32 = 720,
}

pub struct EmulateMediaOptions {
    pub media: []const u8 = "",
    pub color_scheme: []const u8 = "",
    pub reduced_motion: []const u8 = "",
    pub forced_colors: []const u8 = "",
}

pub struct Page {
    allocator: std.mem.Allocator = undefined,
    pub endpoint: *CdpEndpoint = undefined,
    pub session_id: []const u8 = "",
    pub target_id: []const u8 = "",
    pub closed: bool = false,
    pub default_timeout_ms: u32 = 30000,
    pub default_navigation_timeout_ms: u32 = 30000,
    pub viewport_width: u32 = 1280,
    pub viewport_height: u32 = 720,
    pub paused: bool = false,
    pub emulated_media: EmulateMediaOptions = .{},
    pub extra_http_headers_json: []const u8 = "{}",
    pub opener_ptr: (*Page)? = null,

    current_url_buf: [1024]u8 = undefined,
    current_url_len: usize = 0,
    current_title_buf: [256]u8 = undefined,
    current_title_len: usize = 0,
    html_content: std.ArrayList(u8) = .empty,
    history: std.ArrayList([]u8) = .empty,
    history_idx: usize = 0,
    routes: std.ArrayList(RouteEntry) = .empty,
    ws_routes: std.ArrayList(WebSocketRouteEntry) = .empty,
    init_scripts: std.ArrayList([]const u8) = .empty,
    exposed_bindings: std.ArrayList(ExposedBindingEntry) = .empty,
    workers_list: std.ArrayList(WorkerInfo) = .empty,
    pending_events: std.ArrayList(PageEventRecord) = .empty,
    recorded_requests: std.ArrayList(Request) = .empty,
    recorded_responses: std.ArrayList(Response) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    main_frame_ptr: (*Frame)? = null,
    video_ptr: (*Video)? = null,

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
            default_navigation_timeout_ms: 30000,
            viewport_width: 1280,
            viewport_height: 720,
            paused: false,
            emulated_media: .{},
            extra_http_headers_json: "{}",
            opener_ptr: null,
            current_url_len: 0,
            current_title_len: 0,
            html_content: .empty,
            history: .empty,
            history_idx: 0,
            routes: .empty,
            ws_routes: .empty,
            init_scripts: .empty,
            exposed_bindings: .empty,
            workers_list: .empty,
            pending_events: .empty,
            recorded_requests: .empty,
            recorded_responses: .empty,
            owned_strings: .empty,
            main_frame_ptr: null,
            video_ptr: null,
        };
        p.setUrlInternal("about:blank");
        return p;
    }

    pub fn deinit(*_) {
        _.closed = true;
        if (mf := _.main_frame_ptr) {
            mf.deinit();
            _.allocator.destroy(mf);
            _.main_frame_ptr = null;
        }
        if (vp := _.video_ptr) {
            vp.deinit();
            _.allocator.destroy(vp);
            _.video_ptr = null;
        }
        _.html_content.deinit(_.allocator);
        _.html_content = .empty;
        for (h of _.history.items) {
            _.allocator.free(h);
        }
        _.history.deinit(_.allocator);
        _.history = .empty;
        _.routes.deinit(_.allocator);
        _.routes = .empty;
        _.ws_routes.deinit(_.allocator);
        _.ws_routes = .empty;
        _.init_scripts.deinit(_.allocator);
        _.init_scripts = .empty;
        _.exposed_bindings.deinit(_.allocator);
        _.exposed_bindings = .empty;
        _.workers_list.deinit(_.allocator);
        _.workers_list = .empty;
        _.pending_events.deinit(_.allocator);
        _.pending_events = .empty;
        _.recorded_requests.deinit(_.allocator);
        _.recorded_requests = .empty;
        _.recorded_responses.deinit(_.allocator);
        _.recorded_responses = .empty;
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

    fn evalSelectorAction(*_, selector: []const u8, action_js: []const u8): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        if (selector.len == 0) return PlaywrightError.InvalidSelector;
        let js_buf: [1536]u8 = undefined;
        const expr = SelectorCompiler.toJsQueryExpression(selector, action_js, &js_buf)?;
        return _.evaluate(expr);
    }

    static fn extractEvalString(alloc: std.mem.Allocator, res: []const u8): ([]u8)!PlaywrightError {
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", true)) {
                return alloc.dupe(u8, val) catch return PlaywrightError.OutOfMemory;
            }
        }
        return alloc.dupe(u8, "") catch return PlaywrightError.OutOfMemory;
    }

    static fn extractEvalBool(res: []const u8, default_val: bool): bool {
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", false)) {
                return std.mem.eql(u8, val, "true");
            }
        }
        return default_val;
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

    pub fn opener(): (*Page)? {
        return _.opener_ptr;
    }

    pub fn workers(): []const WorkerInfo {
        return _.workers_list.items;
    }

    pub fn registerWorker(*_, worker_url: []const u8, worker_target_id: []const u8): void!PlaywrightError {
        const wu = _.intern(worker_url)?;
        const wt = _.intern(worker_target_id)?;
        _.workers_list.append(_.allocator, WorkerInfo{
            url: wu,
            target_id: wt,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn viewportSize(): ViewportSize {
        return ViewportSize{
            width: _.viewport_width,
            height: _.viewport_height,
        };
    }

    pub fn setViewportSize(*_, width: u32, height: u32): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        _.viewport_width = width;
        _.viewport_height = height;
        let params_buf: [256]u8 = undefined;
        const p = std.fmt.bufPrint(
            &params_buf,
            "{{\"width\":{d},\"height\":{d},\"deviceScaleFactor\":1,\"mobile\":false}}",
            .{ width, height },
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.sendCdp("Emulation.setDeviceMetricsOverride", p)?;
        * = res;
    }

    pub fn bringToFront(*_): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        * = _.sendCdp("Page.bringToFront", "{}")?;
    }

    pub fn emulateMedia(*_, opts: EmulateMediaOptions): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        _.emulated_media = opts;
        let params_buf: [512]u8 = undefined;
        const p = std.fmt.bufPrint(
            &params_buf,
            "{{\"media\":\"{s}\",\"features\":[{{\"name\":\"prefers-color-scheme\",\"value\":\"{s}\"}},{{\"name\":\"prefers-reduced-motion\",\"value\":\"{s}\"}},{{\"name\":\"forced-colors\",\"value\":\"{s}\"}}]}}",
            .{ opts.media, opts.color_scheme, opts.reduced_motion, opts.forced_colors },
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.sendCdp("Emulation.setEmulatedMedia", p)?;
        * = res;
    }

    pub fn setExtraHTTPHeaders(*_, headers_json: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const stored = _.intern(if (headers_json.len > 0) headers_json else "{}")?;
        _.extra_http_headers_json = stored;
        let params_buf: [1024]u8 = undefined;
        const p = std.fmt.bufPrint(&params_buf, "{{\"headers\":{s}}}", .{stored}) catch return PlaywrightError.BufferTooSmall;
        const res = _.sendCdp("Network.setExtraHTTPHeaders", p)?;
        * = res;
    }

    pub fn pause(*_): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        _.paused = true;
        * = _.sendCdp("Debugger.pause", "{}")?;
    }

    pub fn addInitScript(*_, script: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const stored = _.intern(script)?;
        _.init_scripts.append(_.allocator, stored) catch return PlaywrightError.OutOfMemory;
        let params_buf: [2048]u8 = undefined;
        const p = std.fmt.bufPrint(&params_buf, "{{\"source\":\"{s}\"}}", .{script}) catch return PlaywrightError.BufferTooSmall;
        const res = _.sendCdp("Page.addScriptToEvaluateOnNewDocument", p)?;
        * = res;
    }

    pub fn addScriptTag(*_, opts: ScriptTagOptions): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let js_buf: [1536]u8 = undefined;
        const expr = if (opts.url.len > 0)
            std.fmt.bufPrint(&js_buf, "(() => {{ const s = document.createElement('script'); s.src = '{s}'; if ('{s}') s.type = '{s}'; document.head.appendChild(s); return 'script'; }})()", .{ opts.url, opts.tag_type, opts.tag_type }) catch return PlaywrightError.BufferTooSmall
        else
            std.fmt.bufPrint(&js_buf, "(() => {{ const s = document.createElement('script'); s.textContent = '{s}'; if ('{s}') s.type = '{s}'; document.head.appendChild(s); return 'script'; }})()", .{ opts.content, opts.tag_type, opts.tag_type }) catch return PlaywrightError.BufferTooSmall;
        return _.evaluate(expr);
    }

    pub fn addStyleTag(*_, opts: StyleTagOptions): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let js_buf: [1536]u8 = undefined;
        const expr = if (opts.url.len > 0)
            std.fmt.bufPrint(&js_buf, "(() => {{ const l = document.createElement('link'); l.rel = 'stylesheet'; l.href = '{s}'; document.head.appendChild(l); return 'link'; }})()", .{opts.url}) catch return PlaywrightError.BufferTooSmall
        else
            std.fmt.bufPrint(&js_buf, "(() => {{ const s = document.createElement('style'); s.textContent = '{s}'; document.head.appendChild(s); return 'style'; }})()", .{opts.content}) catch return PlaywrightError.BufferTooSmall;
        return _.evaluate(expr);
    }

    pub fn exposeBinding(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const stored_name = _.intern(name)?;
        _.exposed_bindings.append(_.allocator, ExposedBindingEntry{
            name: stored_name,
            callback: callback,
            ctx: ctx,
        }) catch return PlaywrightError.OutOfMemory;
        let params_buf: [256]u8 = undefined;
        const p = std.fmt.bufPrint(&params_buf, "{{\"name\":\"{s}\"}}", .{stored_name}) catch return PlaywrightError.BufferTooSmall;
        const res = _.sendCdp("Runtime.addBinding", p)?;
        * = res;
    }

    pub fn exposeFunction(*_, name: []const u8, callback: ExposedBindingFn, ctx: usize): void!PlaywrightError {
        return _.exposeBinding(name, callback, ctx);
    }

    pub fn invokeExposedBinding(*_, name: []const u8, args_json: []const u8): ([]const u8)? {
        for (entry of _.exposed_bindings.items) {
            if (std.mem.eql(u8, entry.name, name)) {
                return entry.callback(entry.ctx, args_json);
            }
        }
        return null;
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

    pub fn dblclick(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.dispatchEvent(new MouseEvent('dblclick', {bubbles:true})); return true;")?;
    }

    pub fn hover(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.dispatchEvent(new MouseEvent('mouseover', {bubbles:true})); el.dispatchEvent(new MouseEvent('mousemove', {bubbles:true})); return true;")?;
    }

    pub fn tap(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.dispatchEvent(new Event('touchstart', {bubbles:true})); el.dispatchEvent(new Event('touchend', {bubbles:true})); el.click(); return true;")?;
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

    pub fn check(*_, selector: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.check()?;
    }

    pub fn uncheck(*_, selector: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.uncheck()?;
    }

    pub fn selectOption(*_, selector: []const u8, value: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.selectOption(value)?;
    }

    pub fn focus(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.focus(); return true;")?;
    }

    pub fn dispatchEvent(*_, selector: []const u8, event_type: []const u8, event_init_json: []const u8): void!PlaywrightError {
        let action_buf: [512]u8 = undefined;
        const init_arg = if (event_init_json.len > 0) event_init_json else "{}";
        const action = std.fmt.bufPrint(
            &action_buf,
            "if (!el) return false; el.dispatchEvent(new CustomEvent('{s}', {s})); return true;",
            .{ event_type, init_arg },
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.evalSelectorAction(selector, action)?;
        * = res;
    }

    pub fn dragAndDrop(*_, source_selector: []const u8, target_selector: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        if (source_selector.len == 0 || target_selector.len == 0) return PlaywrightError.InvalidSelector;
        let js_buf: [1024]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ const s = document.querySelector('{s}'); const t = document.querySelector('{s}'); if (!s || !t) return false; s.dispatchEvent(new Event('dragstart', {{bubbles:true}})); t.dispatchEvent(new Event('drop', {{bubbles:true}})); s.dispatchEvent(new Event('dragend', {{bubbles:true}})); return true; }})()",
            .{ source_selector, target_selector },
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.evaluate(expr)?;
        * = res;
    }

    pub fn innerHTML(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return el ? (el.innerHTML || '') : '';")?;
        return Page.extractEvalString(alloc, res);
    }

    pub fn innerText(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.innerText(alloc);
    }

    pub fn textContent(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.textContent(alloc);
    }

    pub fn inputValue(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return el ? (el.value || '') : '';")?;
        return Page.extractEvalString(alloc, res);
    }

    pub fn getAttribute(*_, alloc: std.mem.Allocator, selector: []const u8, attr_name: []const u8): (([]u8)?)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.getAttribute(alloc, attr_name);
    }

    pub fn isChecked(*_, selector: []const u8): bool!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return Boolean(el && el.checked);")?;
        return Page.extractEvalBool(res, false);
    }

    pub fn isDisabled(*_, selector: []const u8): bool!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return Boolean(el && el.disabled);")?;
        return Page.extractEvalBool(res, false);
    }

    pub fn isEditable(*_, selector: []const u8): bool!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return Boolean(el && !el.disabled && !el.readOnly);")?;
        return Page.extractEvalBool(res, true);
    }

    pub fn isEnabled(*_, selector: []const u8): bool!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.isEnabled();
    }

    pub fn isHidden(*_, selector: []const u8): bool!PlaywrightError {
        const visible = _.isVisible(selector)?;
        return !visible;
    }

    pub fn isVisible(*_, selector: []const u8): bool!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.isVisible();
    }

    pub fn waitForSelector(*_, selector: []const u8, opts: WaitForOptions): Locator!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.waitFor(opts)?;
        return loc;
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
        if (_.closed) return PlaywrightError.PageClosed;
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

    pub fn waitForFunction(*_, expression: []const u8, timeout_ms: u32): ([]const u8)!PlaywrightError {
        * = timeout_ms;
        if (_.closed) return PlaywrightError.PageClosed;
        return _.evaluate(expression);
    }

    pub fn waitForLoadState(*_, state: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let js_buf: [256]u8 = undefined;
        const target = if (state.len > 0) state else "load";
        const expr = std.fmt.bufPrint(&js_buf, "(() => ({{ state: '{s}', readyState: document.readyState }}))()", .{target}) catch return PlaywrightError.BufferTooSmall;
        const res = _.evaluate(expr)?;
        * = res;
    }

    pub fn recordRequest(*_, req: Request): void!PlaywrightError {
        _.recorded_requests.append(_.allocator, req) catch return PlaywrightError.OutOfMemory;
    }

    pub fn recordResponse(*_, resp: Response): void!PlaywrightError {
        _.recorded_responses.append(_.allocator, resp) catch return PlaywrightError.OutOfMemory;
    }

    pub fn waitForRequest(*_, url_pattern: []const u8, timeout_ms: u32): Request!PlaywrightError {
        * = timeout_ms;
        if (_.closed) return PlaywrightError.PageClosed;
        let i: usize = _.recorded_requests.items.len;
        while (i > 0) {
            i -= 1;
            const req = _.recorded_requests.items[i];
            if (UrlMatcher.matches(url_pattern, req.url)) {
                return req;
            }
        }
        return PlaywrightError.Timeout;
    }

    pub fn waitForResponse(*_, url_pattern: []const u8, timeout_ms: u32): Response!PlaywrightError {
        * = timeout_ms;
        if (_.closed) return PlaywrightError.PageClosed;
        let i: usize = _.recorded_responses.items.len;
        while (i > 0) {
            i -= 1;
            const resp = _.recorded_responses.items[i];
            if (UrlMatcher.matches(url_pattern, resp.url)) {
                return resp;
            }
        }
        return PlaywrightError.Timeout;
    }

    pub fn waitForURL(*_, url_pattern: []const u8, timeout_ms: u32): void!PlaywrightError {
        * = timeout_ms;
        if (_.closed) return PlaywrightError.PageClosed;
        if (!UrlMatcher.matches(url_pattern, _.url())) {
            return PlaywrightError.Timeout;
        }
    }

    pub fn waitForTimeout(*_, timeout_ms: u32): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        if (timeout_ms > 0) {
            const ts: std.os.linux.timespec = .{
                sec: (timeout_ms / 1000) as isize,
                nsec: ((timeout_ms % 1000) as isize) * 1_000_000,
            };
            const r = std.os.linux.nanosleep(&ts, null);
            * = r;
        }
    }

    pub fn route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
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

    pub fn coverage(*_): Coverage {
        return Coverage.init(_.allocator, _.endpoint, _.session_id);
    }

    pub fn mainFrame(*_): (*Frame)!PlaywrightError {
        if (mf := _.main_frame_ptr) {
            return mf;
        }
        let mf = _.allocator.create(Frame) catch return PlaywrightError.OutOfMemory;
        mf.* = Frame.init(_.allocator, _.endpoint, _.session_id, _.target_id, "", _.url(), null);
        mf.default_timeout_ms = _.default_timeout_ms;
        mf.page_handle = unsafe { @intFromPtr(_) };
        _.main_frame_ptr = mf;
        return mf;
    }

    pub fn frames(*_, alloc: std.mem.Allocator): ([]*Frame)!PlaywrightError {
        const mf = _.mainFrame()?;
        let out: std.ArrayList(*Frame) = .empty;
        errdefer out.deinit(alloc);
        mf.collectFrames(&out)?;
        return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }

    pub fn frameByName(*_, target_name: []const u8): (*Frame)? {
        const mf = _.mainFrame() catch return null;
        return mf.frameByName(target_name);
    }

    pub fn frameByUrl(*_, url_pattern: []const u8): (*Frame)? {
        const mf = _.mainFrame() catch return null;
        return mf.frameByUrl(url_pattern);
    }

    pub fn routeWebSocket(*_, pattern: []const u8, handler: WebSocketRouteHandlerFn, ctx: usize): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const pat = _.intern(pattern)?;
        _.ws_routes.append(_.allocator, WebSocketRouteEntry{
            pattern: pat,
            handler: handler,
            ctx: ctx,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn unrouteWebSocket(*_, pattern: []const u8) {
        let i: usize = _.ws_routes.items.len;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.ws_routes.items[i].pattern, pattern)) {
                * = _.ws_routes.orderedRemove(i);
            }
        }
    }

    pub fn dispatchWebSocketRoute(*_, ws_route: *WebSocketRoute): bool {
        let i: usize = _.ws_routes.items.len;
        while (i > 0) {
            i -= 1;
            const entry = _.ws_routes.items[i];
            if (UrlMatcher.matches(entry.pattern, ws_route.url)) {
                entry.handler(entry.ctx, ws_route);
                return true;
            }
        }
        return false;
    }

    pub fn video(*_): (*Video)!PlaywrightError {
        if (vp := _.video_ptr) {
            return vp;
        }
        let vp = _.allocator.create(Video) catch return PlaywrightError.OutOfMemory;
        vp.* = Video.init(_.allocator, _.endpoint, _.session_id, "");
        _.video_ptr = vp;
        return vp;
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        * = _.endpoint.detachSession(_.session_id);
    }
}
