// ============================================================================
// packages/puppeteer/src/page.cp — Page API Implementation
// Composes CDPSession, Frame, Keyboard, Mouse, Touchscreen, Cookies & Network
// ============================================================================

const std = @import("std");
import {
    CdpFrameScanner,
    CdpEndpoint,
    PageCaptureScreenshotParams,
    PagePrintToPdfParams,
    PageDomain,
    NetworkEnableParams,
    NetworkDomain,
} from "cap:cdp";
import {
    PuppeteerError,
    Viewport,
    GoToOptions,
    ScreenshotOptions,
    PdfOptions,
    ClickOptions,
    TypeOptions,
    WaitForSelectorOptions,
    AccessibilitySnapshotOptions,
    AccessibilitySnapshot,
    Cookie,
    CookieParam,
    HttpHeader,
    EvaluateResult,
    mapCdpError,
} from "./types";
import { CDPSession } from "./cdp_session";
import { Keyboard, Mouse, Touchscreen } from "./input";
import { ElementHandle } from "./handles";
import { Frame } from "./frame";

pub struct Page {
    session: CDPSession = .{},
    main_frame: Frame = .{},
    kb: Keyboard = .{},
    mouse_ctrl: Mouse = .{},
    touch_ctrl: Touchscreen = .{},
    current_viewport: Viewport = .{},
    extra_headers: [16]HttpHeader = undefined,
    extra_headers_len: usize = 0,
    request_interception_enabled: bool = false,
    cookies_store: [32]Cookie = undefined,
    cookies_len: usize = 0,
    closed: bool = false,

    pub static fn init(
        endpoint: *CdpEndpoint,
        session_id: []const u8,
        target_id: []const u8,
    ): Page {
        let p = Page{
            session: CDPSession.init(endpoint, session_id, "page", target_id, ""),
            closed: false,
        };
        p.main_frame = Frame.init(&p.session, target_id, "");
        p.kb = Keyboard.init(&p.session);
        p.mouse_ctrl = Mouse.init(&p.session);
        p.touch_ctrl = Touchscreen.init(&p.session);
        return p;
    }

    /// Rebind internal self-referential pointers after Page struct has been moved into final storage.
    pub fn bindSelf(*_) {
        _.main_frame.session = &_.session;
        _.kb.session = &_.session;
        _.mouse_ctrl.session = &_.session;
        _.touch_ctrl.session = &_.session;
    }

    pub fn deinit(*_) {
        if (_.closed) return;
        _.closed = true;
        _.main_frame.detached_flag = true;
        _.session.detach() catch {};
    }

    ~init() {
        _.deinit();
    }

    pub fn isClosed(): bool {
        return _.closed || _.session.detached();
    }

    pub fn mainFrame(*_): *Frame {
        _.bindSelf();
        return &_.main_frame;
    }

    pub fn keyboard(*_): *Keyboard {
        _.bindSelf();
        return &_.kb;
    }

    pub fn mouse(*_): *Mouse {
        _.bindSelf();
        return &_.mouse_ctrl;
    }

    pub fn touchscreen(*_): *Touchscreen {
        _.bindSelf();
        return &_.touch_ctrl;
    }

    pub fn createCDPSession(*_, child_session_id: []const u8): CDPSession!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const sid = if (child_session_id.len > 0) child_session_id else _.session.id();
        if (sid.len > 0 && !_.session.endpoint.hasActiveSession(sid)) {
            _.session.endpoint.createSession(sid, "page", _.session.targetId(), _.session.id()) catch |e| {
                return mapCdpError(e);
            };
        }
        return CDPSession.init(_.session.endpoint, sid, "page", _.session.targetId(), _.session.id());
    }

    pub fn url(): []const u8 {
        return _.main_frame.url();
    }

    pub fn title(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        _.bindSelf();
        return _.main_frame.title(out_buf);
    }

    pub fn content(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        _.bindSelf();
        return _.main_frame.content(out_buf);
    }

    pub fn setContent(*_, html: []const u8): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.setContent(html);
    }

    pub fn goto(*_, target_url: []const u8, options: GoToOptions): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.goto(target_url, options);
    }

    pub fn goBack(*_): void!PuppeteerError {
        let buf: [128]u8 = undefined;
        const _ign_1 = _.evaluate("history.back()", &buf)?;
        * = _ign_1;
    }

    pub fn goForward(*_): void!PuppeteerError {
        let buf: [128]u8 = undefined;
        const _ign_2 = _.evaluate("history.forward()", &buf)?;
        * = _ign_2;
    }

    pub fn reload(*_, ignore_cache: bool): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [128]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = if (ignore_cache) "{\"ignoreCache\":true}" else "{\"ignoreCache\":false}";
        * = pbuf;
        const _ign_3 = _.session.send("Page.reload", params, &fbuf)?;
        * = _ign_3;
    }

    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError {
        _.bindSelf();
        return _.main_frame.evaluate(expression, out_buf);
    }

    pub fn evaluateOnNewDocument(*_, script: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [2048]u8 = undefined;
        let fbuf: [2560]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"source\":\"{s}\"}}",
            .{script},
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_4 = _.session.send("Page.addScriptToEvaluateOnNewDocument", params, &fbuf)?;
        * = _ign_4;
    }

    pub fn setViewport(*_, vp: Viewport): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.current_viewport = vp;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const mob_str: []const u8 = if (vp.is_mobile) "true" else "false";
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"width\":{d},\"height\":{d},\"deviceScaleFactor\":{d:.2},\"mobile\":{s}}}",
            .{ vp.width, vp.height, vp.device_scale_factor, mob_str },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_5 = _.session.send("Emulation.setDeviceMetricsOverride", params, &fbuf)?;
        * = _ign_5;
        if (vp.has_touch) {
            const _ign_6 = _.session.send("Emulation.setTouchEmulationEnabled", "{\"enabled\":true}", &fbuf)?;
            * = _ign_6;
        }
    }

    pub fn viewport(): Viewport {
        return _.current_viewport;
    }

    pub fn screenshot(*_, options: ScreenshotOptions, out_buf: []u8): ([]const u8)!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const req = PageDomain.captureScreenshot(
            _.session.endpoint.allocateId(),
            _.session.id(),
            PageCaptureScreenshotParams{
                format: options.format,
                quality: options.quality,
                from_surface: options.from_surface,
                capture_beyond_viewport: options.full_page,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.session.send(req.method, req.params_json, &fbuf)?;
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ProtocolError;
            const data = CdpFrameScanner.extractTopLevelField(resp.result_json, "data", true) orelse "";
            const n = if (data.len <= out_buf.len) data.len else out_buf.len;
            if (n > 0) @memcpy(out_buf[0..n], data[0..n]);
            return out_buf[0..n];
        }
        return out_buf[0..0];
    }

    pub fn pdf(*_, options: PdfOptions, out_buf: []u8): ([]const u8)!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const req = PageDomain.printToPDF(
            _.session.endpoint.allocateId(),
            _.session.id(),
            PagePrintToPdfParams{
                landscape: options.landscape,
                print_background: options.print_background,
                scale: options.scale,
                paper_width: options.paper_width,
                paper_height: options.paper_height,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.session.send(req.method, req.params_json, &fbuf)?;
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ProtocolError;
            const data = CdpFrameScanner.extractTopLevelField(resp.result_json, "data", true) orelse "";
            const n = if (data.len <= out_buf.len) data.len else out_buf.len;
            if (n > 0) @memcpy(out_buf[0..n], data[0..n]);
            return out_buf[0..n];
        }
        return out_buf[0..0];
    }

    pub fn querySelector(*_, selector: []const u8): ElementHandle!PuppeteerError {
        _.bindSelf();
        return _.main_frame.querySelector(selector);
    }

    pub fn querySelectorAll(*_, selector: []const u8, out_handles: []ElementHandle): usize!PuppeteerError {
        _.bindSelf();
        return _.main_frame.querySelectorAll(selector, out_handles);
    }

    pub fn waitForSelector(*_, selector: []const u8, options: WaitForSelectorOptions): ElementHandle!PuppeteerError {
        _.bindSelf();
        return _.main_frame.waitForSelector(selector, options);
    }

    pub fn click(*_, selector: []const u8, options: ClickOptions): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.click(selector, options);
    }

    pub fn typeText(*_, selector: []const u8, text: []const u8, options: TypeOptions): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.typeText(selector, text, options);
    }

    pub fn focus(*_, selector: []const u8): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.focus(selector);
    }

    pub fn hover(*_, selector: []const u8): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.hover(selector);
    }

    pub fn tap(*_, selector: []const u8): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.tap(selector);
    }

    pub fn setExtraHttpHeaders(*_, headers: []const HttpHeader): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const count = if (headers.len <= 16) headers.len else 16;
        let i: usize = 0;
        while (i < count) {
            _.extra_headers[i] = headers[i];
            i += 1;
        }
        _.extra_headers_len = count;

        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const net_req = NetworkDomain.enable(
            _.session.endpoint.allocateId(),
            _.session.id(),
            NetworkEnableParams{},
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const _ign_7 = _.session.send(net_req.method, net_req.params_json, &fbuf)?;
        * = _ign_7;

        let hbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        const prefix = "{\"headers\":{";
        @memcpy(hbuf[0..prefix.len], prefix);
        pos += prefix.len;
        let k: usize = 0;
        while (k < count) {
            if (k > 0) {
                hbuf[pos] = ',';
                pos += 1;
            }
            const entry = std.fmt.bufPrint(
                hbuf[pos..],
                "\"{s}\":\"{s}\"",
                .{ headers[k].name, headers[k].value },
            ) catch return PuppeteerError.BufferTooSmall;
            pos += entry.len;
            k += 1;
        }
        const suffix = "}}";
        if (pos + suffix.len > hbuf.len) return PuppeteerError.BufferTooSmall;
        @memcpy(hbuf[pos .. pos + suffix.len], suffix);
        pos += suffix.len;
        const _ign_8 = _.session.send("Network.setExtraHTTPHeaders", hbuf[0..pos], &fbuf)?;
        * = _ign_8;
    }

    pub fn setRequestInterception(*_, enabled: bool): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.request_interception_enabled = enabled;
        let fbuf: [512]u8 = undefined;
        if (enabled) {
            const _ign_9 = _.session.send("Fetch.enable", "{\"patterns\":[{\"urlPattern\":\"*\"}]}", &fbuf)?;
            * = _ign_9;
        } else {
            const _ign_10 = _.session.send("Fetch.disable", "{}", &fbuf)?;
            * = _ign_10;
        }
    }

    pub fn setUserAgent(*_, user_agent: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"userAgent\":\"{s}\"}}",
            .{user_agent},
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_11 = _.session.send("Emulation.setUserAgentOverride", params, &fbuf)?;
        * = _ign_11;
    }

    pub fn setCookie(*_, cookie_list: []const CookieParam): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;
        for (c of cookie_list) {
            const params = std.fmt.bufPrint(
                &pbuf,
                "{{\"name\":\"{s}\",\"value\":\"{s}\",\"domain\":\"{s}\",\"path\":\"{s}\",\"httpOnly\":{s},\"secure\":{s}}}",
                .{
                    c.name,
                    c.value,
                    c.domain,
                    c.path,
                    if (c.http_only) "true" else "false",
                    if (c.secure) "true" else "false",
                },
            ) catch return PuppeteerError.BufferTooSmall;
            const _ign_12 = _.session.send("Network.setCookie", params, &fbuf)?;
            * = _ign_12;
            if (_.cookies_len < 32) {
                _.cookies_store[_.cookies_len] = Cookie{
                    name: c.name,
                    value: c.value,
                    domain: c.domain,
                    path: c.path,
                    http_only: c.http_only,
                    secure: c.secure,
                    same_site: c.same_site,
                    expires: c.expires,
                };
                _.cookies_len += 1;
            }
        }
    }

    pub fn cookies(*_): []const Cookie {
        return _.cookies_store[0.._.cookies_len];
    }

    pub fn deleteCookie(*_, name: []const u8, domain: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"name\":\"{s}\",\"domain\":\"{s}\"}}",
            .{ name, domain },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_13 = _.session.send("Network.deleteCookies", params, &fbuf)?;
        * = _ign_13;
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.cookies_len) {
            if (!std.mem.eql(u8, _.cookies_store[read_idx].name, name)) {
                _.cookies_store[write_idx] = _.cookies_store[read_idx];
                write_idx += 1;
            }
            read_idx += 1;
        }
        _.cookies_len = write_idx;
    }

    pub fn accessibilitySnapshot(*_, options: AccessibilitySnapshotOptions): AccessibilitySnapshot!PuppeteerError {
        _.bindSelf();
        return _.main_frame.accessibilitySnapshot(options);
    }

    pub fn close(*_): void!PuppeteerError {
        if (_.closed) return;
        if (_.session.endpoint.state == .Connected) {
            let fbuf: [512]u8 = undefined;
            const _ign_14 = _.session.send("Page.close", "{}", &fbuf) catch 0;
            * = _ign_14;
        }
        _.deinit();
    }
}
