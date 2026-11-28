// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/webkit/webkit.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/webkit.cp — WebKit BrowserType, WkConnection & Engine
// Pure CP WebKit Inspector JSON-RPC protocol driver (no bundled browser binary).
// Reuses cap:cdp CdpFrameScanner 16-lane SIMD JSON scanning for frame routing.
// ============================================================================

const std = @import("std");
import {
    CdpFrameScanner,
    CdpSendTransportFn,
} from "cap:cdp";
import {
    PlaywrightError,
    RouteAction,
    Request,
    Response,
    Route,
    RouteEntry,
    RouteHandlerFn,
    UrlMatcher,
} from "./network";
import { BrowserContextOptions } from "./context";
import { GotoOptions, ScreenshotOptions } from "./page";

pub const WK_BROWSER_VERSION: []const u8 = "27.2";
pub const WK_DEFAULT_USER_AGENT: []const u8 = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/27.2 Safari/605.1.15";
pub const WK_BROWSER_CLOSE_MESSAGE_ID: i64 = -9999;

pub struct WebKitLaunchOptions {
    headless: bool = true,
    executable_path: []const u8 = "",
    user_data_dir: []const u8 = "",
    args: []const []const u8 = &.{},
    viewport_width: u32 = 1280,
    viewport_height: u32 = 720,
    timeout_ms: u32 = 30000,
    ignore_https_errors: bool = false,
    require_executable: bool = false,
    proxy_server: []const u8 = "",
    proxy_bypass: []const u8 = "",
    transport_fn: CdpSendTransportFn? = null,
    transport_ctx: usize = 0,
}

pub enum WkMessageKind {
    Request,
    Response,
    ErrorResponse,
    Event,
}

pub struct WkMessage {
    kind: WkMessageKind = WkMessageKind.Event,
    id: i64 = 0,
    method: []const u8 = "",
    params_json: []const u8 = "{}",
    result_json: []const u8 = "{}",
    error_code: i64 = 0,
    error_message: []const u8 = "",
    page_proxy_id: []const u8 = "",
    target_id: []const u8 = "",
}

pub struct WkPageProxySession {
    allocator: std.mem.Allocator = undefined,
    pub page_proxy_id: []const u8 = "",
    pub target_id: []const u8 = "",
    pub crashed: bool = false,
    pub disposed: bool = false,
    pub messages_received: usize = 0,
    pub last_method: []const u8 = "",
    pub last_result_json: []const u8 = "{}",
    connection: *WkConnection = undefined,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(
        alloc: std.mem.Allocator,
        conn: *WkConnection,
        page_proxy_id: []const u8,
        target_id: []const u8,
    ): WkPageProxySession {
        return WkPageProxySession{
            allocator: alloc,
            page_proxy_id: page_proxy_id,
            target_id: target_id,
            crashed: false,
            disposed: false,
            messages_received: 0,
            last_method: "",
            last_result_json: "{}",
            connection: conn,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.disposed = true;
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
        _.last_method = "";
        _.last_result_json = "{}";
    }

    ~init() {
        _.deinit();
    }

    fn trackString(*_, s: []const u8): ([]const u8)!PlaywrightError {
        const duped = _.allocator.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        return duped;
    }

    pub fn send(*_, method: []const u8, params_json: []const u8): i64!PlaywrightError {
        if (_.crashed || _.disposed) return PlaywrightError.ConnectionClosed;
        return _.connection.sendWithProxy(method, params_json, _.page_proxy_id);
    }

    pub fn sendToTarget(*_, inner_method: []const u8, inner_params_json: []const u8): i64!PlaywrightError {
        if (_.crashed || _.disposed) return PlaywrightError.ConnectionClosed;
        if (_.target_id.len == 0) {
            return _.connection.sendWithProxy(inner_method, inner_params_json, _.page_proxy_id);
        }
        const inner_id = _.connection.nextMessageId();
        let inner_buf: [2048]u8 = undefined;
        const inner_p = if (inner_params_json.len > 0) inner_params_json else "{}";
        const inner_msg = std.fmt.bufPrint(
            &inner_buf,
            "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s}}}",
            .{ inner_id, inner_method, inner_p },
        ) catch return PlaywrightError.BufferTooSmall;

        let outer_buf: [2560]u8 = undefined;
        const outer_params = std.fmt.bufPrint(
            &outer_buf,
            "{{\"targetId\":\"{s}\",\"message\":{s}}}",
            .{ _.target_id, inner_msg },
        ) catch return PlaywrightError.BufferTooSmall;

        return _.send("Target.sendMessageToTarget", outer_params);
    }

    pub fn dispatch(*_, msg: WkMessage): void!PlaywrightError {
        if (_.disposed) return;
        _.messages_received += 1;
        if (msg.method.len > 0) {
            _.last_method = _.trackString(msg.method)?;
        }
        if (msg.kind == WkMessageKind.Response && msg.result_json.len > 0) {
            _.last_result_json = _.trackString(msg.result_json)?;
        }
        if (msg.target_id.len > 0 && _.target_id.len == 0) {
            _.target_id = _.trackString(msg.target_id)?;
        }
    }

    pub fn markAsCrashed(*_) {
        _.crashed = true;
    }

    pub fn isDisposed(): bool {
        return _.disposed;
    }

    pub fn dispose(*_) {
        _.disposed = true;
    }
}

pub struct WkConnection {
    allocator: std.mem.Allocator = undefined,
    transport_fn: CdpSendTransportFn? = null,
    transport_ctx: usize = 0,
    last_id: i64 = 0,
    closed: bool = false,
    pub dispatched_count: usize = 0,
    pub ignored_close_count: usize = 0,
    pub last_browser_method: []const u8 = "",
    pub last_browser_result: []const u8 = "{}",
    sessions: std.ArrayList(*WkPageProxySession) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator): WkConnection {
        return WkConnection{
            allocator: alloc,
            transport_fn: null,
            transport_ctx: 0,
            last_id: 0,
            closed: false,
            dispatched_count: 0,
            ignored_close_count: 0,
            last_browser_method: "",
            last_browser_result: "{}",
            sessions: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.close();
        for (sess of _.sessions.items) {
            sess.deinit();
            _.allocator.destroy(sess);
        }
        _.sessions.deinit(_.allocator);
        _.sessions = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
        _.last_browser_method = "";
        _.last_browser_result = "{}";
    }

    ~init() {
        _.deinit();
    }

    fn trackString(*_, s: []const u8): ([]const u8)!PlaywrightError {
        const duped = _.allocator.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        return duped;
    }

    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize) {
        _.transport_fn = cb;
        _.transport_ctx = ctx;
    }

    pub fn nextMessageId(*_): i64 {
        _.last_id += 1;
        return _.last_id;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        for (sess of _.sessions.items) {
            sess.dispose();
        }
    }

    /// Encode a WebKit Inspector JSON-RPC frame into `out_buf` (zero-heap).
    pub static fn encodeFrame(
        id: i64,
        method: []const u8,
        params_json: []const u8,
        page_proxy_id: []const u8,
        out_buf: []u8,
    ): usize!PlaywrightError {
        if (method.len == 0) return PlaywrightError.ProtocolError;
        const p = if (params_json.len > 0) params_json else "{}";

        if (page_proxy_id.len > 0) {
            const s = std.fmt.bufPrint(
                out_buf,
                "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s},\"pageProxyId\":\"{s}\"}}",
                .{ id, method, p, page_proxy_id },
            ) catch return PlaywrightError.BufferTooSmall;
            return s.len;
        }
        const s = std.fmt.bufPrint(
            out_buf,
            "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s}}}",
            .{ id, method, p },
        ) catch return PlaywrightError.BufferTooSmall;
        return s.len;
    }

    /// Decode a raw WebKit Inspector JSON-RPC frame using 16-lane SIMD CdpFrameScanner.
    pub static fn decodeFrame(raw: []const u8): WkMessage!PlaywrightError {
        const trimmed = CdpFrameScanner.trimWs(raw);
        if (trimmed.len < 2 || trimmed[0] != '{') return PlaywrightError.ProtocolError;

        const id_slice = CdpFrameScanner.extractTopLevelField(trimmed, "id", false);
        const method_slice = CdpFrameScanner.extractTopLevelField(trimmed, "method", true);
        const page_proxy_id = CdpFrameScanner.extractTopLevelField(trimmed, "pageProxyId", true) orelse "";
        const target_id = CdpFrameScanner.extractTopLevelField(trimmed, "targetId", true) orelse "";

        if (id_raw := id_slice) {
            const parsed_id = CdpFrameScanner.parseFastI64(id_raw) orelse return PlaywrightError.ProtocolError;
            if (m := method_slice) {
                const params_raw = CdpFrameScanner.extractTopLevelField(trimmed, "params", false) orelse "{}";
                return WkMessage{
                    kind: WkMessageKind.Request,
                    id: parsed_id,
                    method: m,
                    params_json: params_raw,
                    page_proxy_id: page_proxy_id,
                    target_id: target_id,
                };
            }
            if (err_obj := CdpFrameScanner.extractTopLevelField(trimmed, "error", false)) {
                let err_code: i64 = -1;
                if (code_raw := CdpFrameScanner.extractTopLevelField(err_obj, "code", false)) {
                    if (c := CdpFrameScanner.parseFastI64(code_raw)) {
                        err_code = c;
                    }
                }
                const err_msg = CdpFrameScanner.extractTopLevelField(err_obj, "message", true) orelse "WebKit protocol error";
                return WkMessage{
                    kind: WkMessageKind.ErrorResponse,
                    id: parsed_id,
                    error_code: err_code,
                    error_message: err_msg,
                    page_proxy_id: page_proxy_id,
                    target_id: target_id,
                };
            }
            const result_raw = CdpFrameScanner.extractTopLevelField(trimmed, "result", false) orelse "{}";
            return WkMessage{
                kind: WkMessageKind.Response,
                id: parsed_id,
                result_json: result_raw,
                page_proxy_id: page_proxy_id,
                target_id: target_id,
            };
        }

        const ev_method = method_slice orelse return PlaywrightError.ProtocolError;
        const ev_params = CdpFrameScanner.extractTopLevelField(trimmed, "params", false) orelse "{}";
        return WkMessage{
            kind: WkMessageKind.Event,
            method: ev_method,
            params_json: ev_params,
            page_proxy_id: page_proxy_id,
            target_id: target_id,
        };
    }

    pub fn rawSend(*_, payload: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.ConnectionClosed;
        if (cb := _.transport_fn) {
            if (!cb(_.transport_ctx, payload)) {
                return PlaywrightError.NotConnected;
            }
            return;
        }
        return PlaywrightError.NotConnected;
    }

    pub fn send(*_, method: []const u8, params_json: []const u8): i64!PlaywrightError {
        return _.sendWithProxy(method, params_json, "");
    }

    pub fn sendWithProxy(
        *_,
        method: []const u8,
        params_json: []const u8,
        page_proxy_id: []const u8,
    ): i64!PlaywrightError {
        if (_.closed) return PlaywrightError.ConnectionClosed;
        const msg_id = _.nextMessageId();
        let buf: [4096]u8 = undefined;
        const n = WkConnection.encodeFrame(msg_id, method, params_json, page_proxy_id, &buf)?;
        _.rawSend(buf[0..n])?;
        return msg_id;
    }

    pub fn sendGracefulClose(*_) {
        if (_.closed) return;
        let buf: [256]u8 = undefined;
        const n = WkConnection.encodeFrame(WK_BROWSER_CLOSE_MESSAGE_ID, "Playwright.close", "{}", "", &buf) catch 0;
        if (n > 0) {
            if (cb := _.transport_fn) {
                * = cb(_.transport_ctx, buf[0..n]);
            }
        }
    }

    pub fn getOrCreatePageProxySession(
        *_,
        page_proxy_id: []const u8,
        target_id: []const u8,
    ): (*WkPageProxySession)!PlaywrightError {
        if (existing := _.getPageProxySession(page_proxy_id)) {
            return existing;
        }
        const owned_pid = _.trackString(page_proxy_id)?;
        const owned_tid = if (target_id.len > 0) _.trackString(target_id)? else "";
        let sess_ptr = _.allocator.create(WkPageProxySession) catch return PlaywrightError.OutOfMemory;
        sess_ptr.* = WkPageProxySession.init(_.allocator, _, owned_pid, owned_tid);
        _.sessions.append(_.allocator, sess_ptr) catch {
            sess_ptr.deinit();
            _.allocator.destroy(sess_ptr);
            return PlaywrightError.OutOfMemory;
        };
        return sess_ptr;
    }

    pub fn getPageProxySession(page_proxy_id: []const u8): (*WkPageProxySession)? {
        for (sess of _.sessions.items) {
            if (std.mem.eql(u8, sess.page_proxy_id, page_proxy_id)) {
                return sess;
            }
        }
        return null;
    }

    pub fn sessionCount(): usize {
        return _.sessions.items.len;
    }

    pub fn dispatchMessage(*_, raw: []const u8): void!PlaywrightError {
        const msg = WkConnection.decodeFrame(raw)?;
        if (msg.id == WK_BROWSER_CLOSE_MESSAGE_ID) {
            _.ignored_close_count += 1;
            return;
        }
        _.dispatched_count += 1;

        if (msg.page_proxy_id.len > 0) {
            const sess = _.getOrCreatePageProxySession(msg.page_proxy_id, msg.target_id)?;
            sess.dispatch(msg)?;
            return;
        }

        if (msg.kind == WkMessageKind.Event) {
            if (std.mem.eql(u8, msg.method, "Playwright.pageProxyCreated")) {
                if (pid := CdpFrameScanner.extractTopLevelField(msg.params_json, "pageProxyId", true)) {
                    * = _.getOrCreatePageProxySession(pid, "")?;
                }
            } else if (std.mem.eql(u8, msg.method, "Playwright.pageProxyDestroyed")) {
                if (pid := CdpFrameScanner.extractTopLevelField(msg.params_json, "pageProxyId", true)) {
                    if (sess := _.getPageProxySession(pid)) {
                        sess.dispose();
                    }
                }
            }
            _.last_browser_method = _.trackString(msg.method)?;
        } else if (msg.kind == WkMessageKind.Response) {
            _.last_browser_result = _.trackString(msg.result_json)?;
        }
    }
}

pub struct WebKitPage {
    allocator: std.mem.Allocator = undefined,
    pub session: *WkPageProxySession = undefined,
    pub closed: bool = false,
    pub viewport_width: u32 = 1280,
    pub viewport_height: u32 = 720,

    current_url_buf: [1024]u8 = undefined,
    current_url_len: usize = 0,
    current_title_buf: [256]u8 = undefined,
    current_title_len: usize = 0,
    html_content: std.ArrayList(u8) = .empty,
    history: std.ArrayList([]u8) = .empty,
    history_idx: usize = 0,
    routes: std.ArrayList(RouteEntry) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator, sess: *WkPageProxySession): WebKitPage {
        let p = WebKitPage{
            allocator: alloc,
            session: sess,
            closed: false,
            viewport_width: 1280,
            viewport_height: 720,
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

    pub fn url(): []const u8 {
        return _.current_url_buf[0.._.current_url_len];
    }

    pub fn title(): []const u8 {
        return _.current_title_buf[0.._.current_title_len];
    }

    pub fn content(): []const u8 {
        return _.html_content.items;
    }

    pub fn isClosed(): bool {
        return _.closed;
    }

    pub fn goto(*_, target_url: []const u8, opts: GotoOptions): Response!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        * = opts.timeout_ms;
        * = opts.wait_until;
        if (target_url.len == 0) return PlaywrightError.InvalidUrl;

        const action = _.interceptRequest(target_url, "GET", "document")?;
        if (action == RouteAction.Aborted) {
            return PlaywrightError.ProtocolError;
        }

        let params_buf: [1280]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"url\":\"{s}\"}}",
            .{target_url},
        ) catch return PlaywrightError.BufferTooSmall;
        const nav_id = _.session.send("Page.navigate", params)?;
        * = nav_id;

        _.setUrlInternal(target_url);
        const duped_url = _.allocator.dupe(u8, target_url) catch return PlaywrightError.OutOfMemory;
        _.history.append(_.allocator, duped_url) catch {
            _.allocator.free(duped_url);
            return PlaywrightError.OutOfMemory;
        };
        _.history_idx = _.history.items.len - 1;

        return Response{
            url: _.url(),
            status: 200,
            status_text: "OK",
            ok: true,
            headers_json: "{}",
            body_bytes: _.html_content.items,
        };
    }

    pub fn setContent(*_, html: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        _.html_content.clearRetainingCapacity();
        _.html_content.appendSlice(_.allocator, html) catch return PlaywrightError.OutOfMemory;

        let params_buf: [2048]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"frameId\":\"main\",\"html\":\"{s}\"}}",
            .{html},
        ) catch return PlaywrightError.BufferTooSmall;
        const sc_id = _.session.send("Page.setContent", params)?;
        * = sc_id;
    }

    pub fn evaluate(*_, expression: []const u8): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [2048]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"expression\":\"{s}\",\"returnByValue\":true}}",
            .{expression},
        ) catch return PlaywrightError.BufferTooSmall;
        const eval_id = _.session.send("Runtime.evaluate", params)?;
        * = eval_id;
        return _.session.last_result_json;
    }

    pub fn setViewport(*_, width: u32, height: u32): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        _.viewport_width = width;
        _.viewport_height = height;
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"width\":{d},\"height\":{d}}}",
            .{ width, height },
        ) catch return PlaywrightError.BufferTooSmall;
        const vp_id = _.session.send("Page.setScreenSizeOverride", params)?;
        * = vp_id;
    }

    pub fn screenshot(*_, opts: ScreenshotOptions): ([]const u8)!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        * = opts.format;
        * = opts.quality;
        * = opts.full_page;
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"x\":0,\"y\":0,\"width\":{d},\"height\":{d},\"coordinateSystem\":\"Viewport\",\"omitBackground\":false}}",
            .{ _.viewport_width, _.viewport_height },
        ) catch return PlaywrightError.BufferTooSmall;
        const snap_id = _.session.send("Page.snapshotRect", params)?;
        * = snap_id;
        return _.session.last_result_json;
    }

    // ---- WebKit Input (Keyboard & Mouse) ----

    pub fn keyboardDown(*_, key: []const u8, modifiers: u32): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"type\":\"keyDown\",\"modifiers\":{d},\"key\":\"{s}\",\"code\":\"{s}\"}}",
            .{ modifiers, key, key },
        ) catch return PlaywrightError.BufferTooSmall;
        const kd_id = _.session.send("Input.dispatchKeyEvent", params)?;
        * = kd_id;
    }

    pub fn keyboardUp(*_, key: []const u8, modifiers: u32): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"type\":\"keyUp\",\"modifiers\":{d},\"key\":\"{s}\",\"code\":\"{s}\"}}",
            .{ modifiers, key, key },
        ) catch return PlaywrightError.BufferTooSmall;
        const ku_id = _.session.send("Input.dispatchKeyEvent", params)?;
        * = ku_id;
    }

    pub fn insertText(*_, text: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [1024]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"text\":\"{s}\"}}",
            .{text},
        ) catch return PlaywrightError.BufferTooSmall;
        const it_id = _.session.send("Page.insertText", params)?;
        * = it_id;
    }

    pub fn mouseMove(*_, x: f64, y: f64): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"type\":\"move\",\"x\":{d},\"y\":{d},\"button\":\"none\",\"modifiers\":0}}",
            .{ x, y },
        ) catch return PlaywrightError.BufferTooSmall;
        const mv_id = _.session.send("Input.dispatchMouseEvent", params)?;
        * = mv_id;
    }

    pub fn mouseDown(*_, x: f64, y: f64, button: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"type\":\"down\",\"x\":{d},\"y\":{d},\"button\":\"{s}\",\"clickCount\":1,\"modifiers\":0}}",
            .{ x, y, button },
        ) catch return PlaywrightError.BufferTooSmall;
        const dn_id = _.session.send("Input.dispatchMouseEvent", params)?;
        * = dn_id;
    }

    pub fn mouseUp(*_, x: f64, y: f64, button: []const u8): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"type\":\"up\",\"x\":{d},\"y\":{d},\"button\":\"{s}\",\"clickCount\":1,\"modifiers\":0}}",
            .{ x, y, button },
        ) catch return PlaywrightError.BufferTooSmall;
        const up_id = _.session.send("Input.dispatchMouseEvent", params)?;
        * = up_id;
    }

    pub fn click(*_, x: f64, y: f64): void!PlaywrightError {
        _.mouseMove(x, y)?;
        _.mouseDown(x, y, "left")?;
        _.mouseUp(x, y, "left")?;
    }

    // ---- WebKit Network Interception ----

    pub fn route(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError {
        if (_.closed) return PlaywrightError.PageClosed;
        const duped = _.allocator.dupe(u8, pattern) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        _.routes.append(_.allocator, RouteEntry{
            pattern: duped,
            handler: handler,
            ctx: ctx,
        }) catch return PlaywrightError.OutOfMemory;

        const net_id = _.session.send("Network.setInterceptionEnabled", "{\"enabled\":true}")?;
        * = net_id;
    }

    pub fn unroute(*_, pattern: []const u8) {
        let i: usize = 0;
        while (i < _.routes.items.len) {
            if (std.mem.eql(u8, _.routes.items[i].pattern, pattern)) {
                * = _.routes.swapRemove(i);
            } else {
                i += 1;
            }
        }
    }

    pub fn interceptRequest(
        *_,
        request_url: []const u8,
        method: []const u8,
        resource_type: []const u8,
    ): RouteAction!PlaywrightError {
        let i = _.routes.items.len;
        while (i > 0) {
            i -= 1;
            const entry = _.routes.items[i];
            if (UrlMatcher.matches(entry.pattern, request_url)) {
                let r = Route{
                    request: Request{
                        url: request_url,
                        method: method,
                        resource_type: resource_type,
                        is_navigation_request: std.mem.eql(u8, resource_type, "document"),
                    },
                };
                entry.handler(entry.ctx, &r);
                if (r.action == RouteAction.Fulfilled) {
                    let params_buf: [512]u8 = undefined;
                    const p = std.fmt.bufPrint(
                        &params_buf,
                        "{{\"requestId\":\"wk-req-1\",\"status\":{d}}}",
                        .{r.fulfill_status},
                    ) catch return PlaywrightError.BufferTooSmall;
                    const resp_id = _.session.send("Network.interceptWithResponse", p)?;
                    * = resp_id;
                    return RouteAction.Fulfilled;
                } else if (r.action == RouteAction.Aborted) {
                    * = _.session.send("Network.interceptRequestWithError", "{\"requestId\":\"wk-req-1\",\"errorType\":\"Cancellation\"}")?;
                    return RouteAction.Aborted;
                } else if (r.action == RouteAction.Continued) {
                    * = _.session.send("Network.interceptContinue", "{\"requestId\":\"wk-req-1\"}")?;
                    return RouteAction.Continued;
                }
            }
        }
        return RouteAction.Pending;
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        _.session.dispose();
    }
}

pub struct WebKitBrowserContext {
    allocator: std.mem.Allocator = undefined,
    pub connection: *WkConnection = undefined,
    pub context_id: []const u8 = "",
    pub options: BrowserContextOptions = .{},
    pub closed: bool = false,
    next_page_num: u64 = 1,
    pages_list: std.ArrayList(*WebKitPage) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(
        alloc: std.mem.Allocator,
        conn: *WkConnection,
        context_id: []const u8,
        opts: BrowserContextOptions,
    ): WebKitBrowserContext {
        return WebKitBrowserContext{
            allocator: alloc,
            connection: conn,
            context_id: context_id,
            options: opts,
            closed: false,
            next_page_num: 1,
            pages_list: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.close();
        for (p of _.pages_list.items) {
            p.deinit();
            _.allocator.destroy(p);
        }
        _.pages_list.deinit(_.allocator);
        _.pages_list = .empty;
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    pub fn newPage(*_): (*WebKitPage)!PlaywrightError {
        if (_.closed) return PlaywrightError.ContextClosed;
        const n = _.next_page_num;
        _.next_page_num += 1;

        const pid = std.fmt.allocPrint(_.allocator, "wk-page-{d}", .{n}) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, pid) catch {
            _.allocator.free(pid);
            return PlaywrightError.OutOfMemory;
        };

        let params_buf: [256]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"browserContextId\":\"{s}\"}}",
            .{_.context_id},
        ) catch return PlaywrightError.BufferTooSmall;
        const cp_id = _.connection.send("Playwright.createPage", params)?;
        * = cp_id;

        const sess = _.connection.getOrCreatePageProxySession(pid, "")?;
        let page_ptr = _.allocator.create(WebKitPage) catch return PlaywrightError.OutOfMemory;
        page_ptr.* = WebKitPage.init(_.allocator, sess);
        page_ptr.viewport_width = _.options.viewport_width;
        page_ptr.viewport_height = _.options.viewport_height;

        _.pages_list.append(_.allocator, page_ptr) catch {
            page_ptr.deinit();
            _.allocator.destroy(page_ptr);
            return PlaywrightError.OutOfMemory;
        };
        return page_ptr;
    }

    pub fn pages(): []*WebKitPage {
        return _.pages_list.items;
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        for (p of _.pages_list.items) {
            p.close();
        }
    }
}

pub struct WebKitBrowser {
    allocator: std.mem.Allocator = undefined,
    pub connection: WkConnection = undefined,
    pub version_str: []const u8 = WK_BROWSER_VERSION,
    pub executable_path: []const u8 = "",
    pub launch_args: [][]u8 = &.{},
    pub closed: bool = false,
    next_ctx_num: u64 = 1,
    contexts_list: std.ArrayList(*WebKitBrowserContext) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator): WebKitBrowser {
        return WebKitBrowser{
            allocator: alloc,
            connection: WkConnection.init(alloc),
            version_str: WK_BROWSER_VERSION,
            executable_path: "",
            launch_args: &.{},
            closed: false,
            next_ctx_num: 1,
            contexts_list: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.close();
        for (ctx of _.contexts_list.items) {
            ctx.deinit();
            _.allocator.destroy(ctx);
        }
        _.contexts_list.deinit(_.allocator);
        _.contexts_list = .empty;

        if (_.launch_args.len > 0) {
            WebKitBrowserType.freeLaunchArgs(_.allocator, _.launch_args);
            _.launch_args = &.{};
        }

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;

        _.connection.deinit();
    }

    ~init() {
        _.deinit();
    }

    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize) {
        _.connection.setTransport(cb, ctx);
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

    pub fn newContext(*_, opts: BrowserContextOptions): (*WebKitBrowserContext)!PlaywrightError {
        if (_.closed) return PlaywrightError.BrowserClosed;
        const n = _.next_ctx_num;
        _.next_ctx_num += 1;

        const cid = std.fmt.allocPrint(_.allocator, "wk-ctx-{d}", .{n}) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, cid) catch {
            _.allocator.free(cid);
            return PlaywrightError.OutOfMemory;
        };

        * = _.connection.send("Playwright.createContext", "{}")?;

        let ctx_ptr = _.allocator.create(WebKitBrowserContext) catch return PlaywrightError.OutOfMemory;
        ctx_ptr.* = WebKitBrowserContext.init(_.allocator, &_.connection, cid, opts);

        _.contexts_list.append(_.allocator, ctx_ptr) catch {
            ctx_ptr.deinit();
            _.allocator.destroy(ctx_ptr);
            return PlaywrightError.OutOfMemory;
        };
        return ctx_ptr;
    }

    pub fn newPage(*_): (*WebKitPage)!PlaywrightError {
        if (_.closed) return PlaywrightError.BrowserClosed;
        const ctx = _.newContext(.{})?;
        return ctx.newPage();
    }

    pub fn contexts(): []*WebKitBrowserContext {
        return _.contexts_list.items;
    }

    pub fn version(): []const u8 {
        return _.version_str;
    }

    pub fn isConnected(): bool {
        return !_.closed && !_.connection.isClosed();
    }

    pub fn close(*_) {
        if (_.closed) return;
        _.closed = true;
        for (ctx of _.contexts_list.items) {
            ctx.close();
        }
        _.connection.sendGracefulClose();
        _.connection.close();
    }
}

pub struct WebKitBrowserType {
    pub static fn name(): []const u8 {
        return "webkit";
    }

    static fn isExecutableReadable(path: []const u8): bool {
        if (path.len == 0 || path.len >= 4096) return false;
        let zbuf: [4097]u8 = undefined;
        @memcpy(zbuf[0..path.len], path);
        zbuf[path.len] = 0;
        const zptr: [*:0]const u8 = unsafe { @ptrCast(&zbuf[0]) };
        return std.c.access(zptr, 1) == 0;
    }

    static fn probePlaywrightCacheDir(alloc: std.mem.Allocator, cache_dir: []const u8): ([]u8)? {
        if (cache_dir.len == 0 || cache_dir.len >= 4096) return null;
        let zbuf: [4097]u8 = undefined;
        @memcpy(zbuf[0..cache_dir.len], cache_dir);
        zbuf[cache_dir.len] = 0;
        const c_dir_path: [*:0]const u8 = unsafe { @ptrCast(&zbuf[0]) };
        const dir = std.c.opendir(c_dir_path);
        if (dir == null) return null;
        defer * = std.c.closedir(dir.?);

        while (entry := std.c.readdir(dir.?)) {
            let name_len: usize = 0;
            while (name_len < 256 && entry.name[name_len] != 0) {
                name_len += 1;
            }
            if (name_len == 0) continue;
            const entry_name = entry.name[0..name_len];
            if (std.mem.startsWith(u8, entry_name, "webkit-")) {
                const rels: [3][]const u8 = .{
                    "pw_run.sh",
                    "minibrowser-wpe/MiniBrowser",
                    "minibrowser-gtk/MiniBrowser",
                };
                for (rel of rels) {
                    const candidate = std.fmt.allocPrint(alloc, "{s}/{s}/{s}", .{ cache_dir, entry_name, rel }) catch continue;
                    if (WebKitBrowserType.isExecutableReadable(candidate)) {
                        return candidate;
                    }
                    alloc.free(candidate);
                }
            }
        }
        return null;
    }

    /// Probe Playwright WebKit executable from `preferred`, `PLAYWRIGHT_WEBKIT_EXECUTABLE_PATH`,
    /// `~/.cache/ms-playwright/webkit-*/`, `/root/.cache/ms-playwright/webkit-*/`, and `/usr/bin/MiniBrowser`.
    pub static fn executablePath(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)? {
        if (preferred.len > 0) {
            if (WebKitBrowserType.isExecutableReadable(preferred)) {
                return alloc.dupe(u8, preferred) catch null;
            }
        }

        if (env_ptr := std.c.getenv("PLAYWRIGHT_WEBKIT_EXECUTABLE_PATH")) {
            const env_path = std.mem.span(env_ptr);
            if (WebKitBrowserType.isExecutableReadable(env_path)) {
                return alloc.dupe(u8, env_path) catch null;
            }
        }

        if (pw_browsers_ptr := std.c.getenv("PLAYWRIGHT_BROWSERS_PATH")) {
            const pw_browsers = std.mem.span(pw_browsers_ptr);
            if (found := WebKitBrowserType.probePlaywrightCacheDir(alloc, pw_browsers)) {
                return found;
            }
        }

        if (home_ptr := std.c.getenv("HOME")) {
            const home = std.mem.span(home_ptr);
            if (cache_dir := std.fmt.allocPrint(alloc, "{s}/.cache/ms-playwright", .{home}) catch null) {
                defer alloc.free(cache_dir);
                if (found := WebKitBrowserType.probePlaywrightCacheDir(alloc, cache_dir)) {
                    return found;
                }
            }
        }

        if (found_root := WebKitBrowserType.probePlaywrightCacheDir(alloc, "/root/.cache/ms-playwright")) {
            return found_root;
        }

        const static_candidates: [4][]const u8 = .{
            "/usr/bin/MiniBrowser",
            "/usr/local/bin/MiniBrowser",
            "/usr/bin/webkit-minibrowser",
            "/Applications/Playwright.app/Contents/MacOS/Playwright",
        };
        for (cand of static_candidates) {
            if (WebKitBrowserType.isExecutableReadable(cand)) {
                return alloc.dupe(u8, cand) catch null;
            }
        }
        return null;
    }

    pub static fn freeLaunchArgs(alloc: std.mem.Allocator, args: [][]u8) {
        for (arg of args) {
            alloc.free(arg);
        }
        alloc.free(args);
    }

    /// Build WebKit MiniBrowser / pw_run.sh CLI launch flags (`--inspector-pipe`, `--headless`, etc.).
    pub static fn buildLaunchArgs(alloc: std.mem.Allocator, opts: WebKitLaunchOptions): ([]([]u8))!PlaywrightError {
        let out: std.ArrayList([]u8) = .empty;
        errdefer {
            for (item of out.items) alloc.free(item);
            out.deinit(alloc);
        }

        const pipe_arg = alloc.dupe(u8, "--inspector-pipe") catch return PlaywrightError.OutOfMemory;
        out.append(alloc, pipe_arg) catch {
            alloc.free(pipe_arg);
            return PlaywrightError.OutOfMemory;
        };

        if (opts.headless) {
            const h_arg = alloc.dupe(u8, "--headless") catch return PlaywrightError.OutOfMemory;
            out.append(alloc, h_arg) catch {
                alloc.free(h_arg);
                return PlaywrightError.OutOfMemory;
            };
        }

        if (opts.user_data_dir.len > 0) {
            const udd = std.fmt.allocPrint(alloc, "--user-data-dir={s}", .{opts.user_data_dir}) catch return PlaywrightError.OutOfMemory;
            out.append(alloc, udd) catch {
                alloc.free(udd);
                return PlaywrightError.OutOfMemory;
            };
        } else {
            const nsw = alloc.dupe(u8, "--no-startup-window") catch return PlaywrightError.OutOfMemory;
            out.append(alloc, nsw) catch {
                alloc.free(nsw);
                return PlaywrightError.OutOfMemory;
            };
        }

        if (opts.proxy_server.len > 0) {
            const px = std.fmt.allocPrint(alloc, "--proxy={s}", .{opts.proxy_server}) catch return PlaywrightError.OutOfMemory;
            out.append(alloc, px) catch {
                alloc.free(px);
                return PlaywrightError.OutOfMemory;
            };
            if (opts.proxy_bypass.len > 0) {
                const ig = std.fmt.allocPrint(alloc, "--ignore-host={s}", .{opts.proxy_bypass}) catch return PlaywrightError.OutOfMemory;
                out.append(alloc, ig) catch {
                    alloc.free(ig);
                    return PlaywrightError.OutOfMemory;
                };
            }
        }

        for (extra of opts.args) {
            if (extra.len == 0) continue;
            if (std.mem.startsWith(u8, extra, "--user-data-dir")) {
                return PlaywrightError.InvalidUrl;
            }
            const duped = alloc.dupe(u8, extra) catch return PlaywrightError.OutOfMemory;
            out.append(alloc, duped) catch {
                alloc.free(duped);
                return PlaywrightError.OutOfMemory;
            };
        }

        if (opts.user_data_dir.len > 0) {
            const blank = alloc.dupe(u8, "about:blank") catch return PlaywrightError.OutOfMemory;
            out.append(alloc, blank) catch {
                alloc.free(blank);
                return PlaywrightError.OutOfMemory;
            };
        }

        return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }

    pub static fn launch(alloc: std.mem.Allocator, opts: WebKitLaunchOptions): WebKitBrowser!PlaywrightError {
        let browser = WebKitBrowser.init(alloc);
        errdefer browser.deinit();

        browser.launch_args = WebKitBrowserType.buildLaunchArgs(alloc, opts)?;

        if (exe := WebKitBrowserType.executablePath(alloc, opts.executable_path)) {
            defer alloc.free(exe);
            browser.setExecutablePath(exe)?;
        } else if (opts.require_executable) {
            return PlaywrightError.ExecutableNotFound;
        }

        if (cb := opts.transport_fn) {
            browser.setTransport(cb, opts.transport_ctx);
            * = browser.connection.send("Playwright.enable", "{}")?;
        } else {
            return PlaywrightError.NotConnected;
        }

        return browser;
    }

    pub static fn launchWithTransport(
        alloc: std.mem.Allocator,
        opts: WebKitLaunchOptions,
        cb: CdpSendTransportFn,
        ctx: usize,
    ): WebKitBrowser!PlaywrightError {
        return WebKitBrowserType.launch(alloc, WebKitLaunchOptions{
            headless: opts.headless,
            executable_path: opts.executable_path,
            user_data_dir: opts.user_data_dir,
            args: opts.args,
            viewport_width: opts.viewport_width,
            viewport_height: opts.viewport_height,
            timeout_ms: opts.timeout_ms,
            ignore_https_errors: opts.ignore_https_errors,
            require_executable: opts.require_executable,
            proxy_server: opts.proxy_server,
            proxy_bypass: opts.proxy_bypass,
            transport_fn: cb,
            transport_ctx: ctx,
        });
    }

    pub static fn connectWithTransport(
        alloc: std.mem.Allocator,
        opts: WebKitLaunchOptions,
        cb: CdpSendTransportFn,
        ctx: usize,
    ): WebKitBrowser!PlaywrightError {
        * = opts.timeout_ms;
        let browser = WebKitBrowser.init(alloc);
        errdefer browser.deinit();
        browser.setTransport(cb, ctx);
        * = browser.connection.send("Playwright.enable", "{}")?;
        return browser;
    }
}

pub const webkit = WebKitBrowserType;
