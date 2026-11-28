// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/Page.ts | Synced: 2026-11-28
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
    MediaFeature,
    GeolocationOptions,
    NetworkConditions,
    Credentials,
    PageMetrics,
    WebWorkerInfo,
    WaitForNetworkIdleOptions,
    mapCdpError,
} from "./types";
import { CDPSession } from "./cdp_session";
import { Keyboard, Mouse, Touchscreen } from "./input";
import { ElementHandle, PuppeteerLocator } from "./handles";
import { Frame } from "./frame";
import { Device } from "./devices";
import { WaitForFunctionOptions } from "./selectors";
import { PuppeteerEventEmitter, EventHandlerFn } from "./events";
import { TimeoutSettings, FileChooser } from "./extras";
import { HTTPRequest, HTTPResponse } from "./network";
import { Tracing } from "./tracing";
import { Coverage } from "./coverage";

pub struct Page {
    session: CDPSession = .{},
    main_frame: Frame = .{},
    kb: Keyboard = .{},
    mouse_ctrl: Mouse = .{},
    touch_ctrl: Touchscreen = .{},
    tracing_ctrl: Tracing = .{},
    coverage_ctrl: Coverage = .{},
    current_viewport: Viewport = .{},
    extra_headers: [16]HttpHeader = undefined,
    extra_headers_len: usize = 0,
    request_interception_enabled: bool = false,
    cookies_store: [32]Cookie = undefined,
    cookies_len: usize = 0,
    timeout_settings: TimeoutSettings = .{},
    emitter: PuppeteerEventEmitter = .{},
    js_enabled: bool = true,
    bypass_service_worker: bool = false,
    bypass_csp: bool = false,
    cache_enabled: bool = true,
    offline_mode_enabled: bool = false,
    drag_interception_enabled: bool = false,
    has_credentials: bool = false,
    auth_credentials: Credentials = .{},
    exposed_functions: [16][]const u8 = undefined,
    exposed_functions_len: usize = 0,
    workers_list: [8]WebWorkerInfo = undefined,
    workers_len: usize = 0,
    frame_ptrs: [17]*Frame = undefined,
    recorded_requests: [16]HTTPRequest = undefined,
    recorded_requests_len: usize = 0,
    recorded_responses: [16]HTTPResponse = undefined,
    recorded_responses_len: usize = 0,
    inflight_requests: u32 = 0,
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
        p.tracing_ctrl = Tracing.init(&p.session);
        p.coverage_ctrl = Coverage.init(&p.session);
        return p;
    }

    /// Rebind internal self-referential pointers after Page struct has been moved into final storage.
    pub fn bindSelf(*_) {
        _.main_frame.session = &_.session;
        _.kb.session = &_.session;
        _.mouse_ctrl.session = &_.session;
        _.touch_ctrl.session = &_.session;
        _.tracing_ctrl.session = &_.session;
        _.coverage_ctrl.bindSession(&_.session);
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

    pub fn emulate(*_, device: Device): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.setViewport(device.viewport)?;
        if (device.user_agent.len > 0) {
            _.setUserAgent(device.user_agent)?;
        }
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

    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError {
        _.bindSelf();
        return _.main_frame.waitForFunction(expression, options, out_buf);
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

    pub fn tracing(*_): *Tracing {
        _.bindSelf();
        return &_.tracing_ctrl;
    }

    pub fn coverage(*_): *Coverage {
        _.bindSelf();
        return &_.coverage_ctrl;
    }

    pub fn setDefaultTimeout(*_, timeout_ms: u32) {
        _.timeout_settings.setDefaultTimeout(timeout_ms);
    }

    pub fn setDefaultNavigationTimeout(*_, timeout_ms: u32) {
        _.timeout_settings.setDefaultNavigationTimeout(timeout_ms);
    }

    pub fn getDefaultTimeout(): u32 {
        return _.timeout_settings.timeout();
    }

    pub fn getDefaultNavigationTimeout(): u32 {
        return _.timeout_settings.navigationTimeout();
    }

    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.on(event_name, handler, ctx);
    }

    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.once(event_name, handler, ctx);
    }

    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn) {
        _.emitter.off(event_name, handler);
    }

    pub fn emit(*_, event_name: []const u8, payload: []const u8): bool {
        return _.emitter.emit(event_name, payload);
    }

    // --- Emulation & Environment ---

    pub fn emulateMediaType(*_, media_type: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(&pbuf, "{{\"media\":\"{s}\"}}", .{media_type}) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Emulation.setEmulatedMedia", params, &fbuf)?;
        * = _ign;
    }

    pub fn emulateMediaFeatures(*_, features: []const MediaFeature): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;
        let pos: usize = 0;
        const prefix = "{\"features\":[";
        @memcpy(pbuf[0..prefix.len], prefix);
        pos = prefix.len;
        let i: usize = 0;
        while (i < features.len) {
            if (i > 0) {
                if (pos >= pbuf.len) return PuppeteerError.BufferTooSmall;
                pbuf[pos] = ',';
                pos += 1;
            }
            const item = std.fmt.bufPrint(
                pbuf[pos..],
                "{{\"name\":\"{s}\",\"value\":\"{s}\"}}",
                .{ features[i].name, features[i].value },
            ) catch return PuppeteerError.BufferTooSmall;
            pos += item.len;
            i += 1;
        }
        const suffix = "]}";
        if (pos + suffix.len > pbuf.len) return PuppeteerError.BufferTooSmall;
        @memcpy(pbuf[pos .. pos + suffix.len], suffix);
        pos += suffix.len;
        const _ign = _.session.send("Emulation.setEmulatedMedia", pbuf[0..pos], &fbuf)?;
        * = _ign;
    }

    pub fn emulateTimezone(*_, timezone_id: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(&pbuf, "{{\"timezoneId\":\"{s}\"}}", .{timezone_id}) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Emulation.setTimezoneOverride", params, &fbuf)?;
        * = _ign;
    }

    pub fn emulateVisionDeficiency(*_, vision_type: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const vtype = if (vision_type.len > 0) vision_type else "none";
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(&pbuf, "{{\"type\":\"{s}\"}}", .{vtype}) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Emulation.setEmulatedVisionDeficiency", params, &fbuf)?;
        * = _ign;
    }

    pub fn emulateCPUThrottling(*_, factor: f64): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const rate = if (factor < 1.0) 1.0 else factor;
        let pbuf: [128]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(&pbuf, "{{\"rate\":{d:.2}}}", .{rate}) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Emulation.setCPUThrottlingRate", params, &fbuf)?;
        * = _ign;
    }

    pub fn emulateNetworkConditions(*_, conditions: NetworkConditions): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.offline_mode_enabled = conditions.offline;
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const off_str: []const u8 = if (conditions.offline) "true" else "false";
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"offline\":{s},\"latency\":{d:.2},\"downloadThroughput\":{d:.2},\"uploadThroughput\":{d:.2}}}",
            .{ off_str, conditions.latency, conditions.download_throughput, conditions.upload_throughput },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Network.emulateNetworkConditions", params, &fbuf)?;
        * = _ign;
    }

    pub fn setGeolocation(*_, options: GeolocationOptions): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        if (options.latitude < -90.0 || options.latitude > 90.0 || options.longitude < -180.0 || options.longitude > 180.0) {
            return PuppeteerError.InvalidArgument;
        }
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"latitude\":{d:.6},\"longitude\":{d:.6},\"accuracy\":{d:.2}}}",
            .{ options.latitude, options.longitude, options.accuracy },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Emulation.setGeolocationOverride", params, &fbuf)?;
        * = _ign;
    }

    pub fn setOfflineMode(*_, enabled: bool): void!PuppeteerError {
        return _.emulateNetworkConditions(NetworkConditions{
            offline: enabled,
            download_throughput: -1.0,
            upload_throughput: -1.0,
            latency: 0.0,
        });
    }

    pub fn setCacheEnabled(*_, enabled: bool): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.cache_enabled = enabled;
        let fbuf: [512]u8 = undefined;
        const params = if (enabled) "{\"cacheDisabled\":false}" else "{\"cacheDisabled\":true}";
        const _ign = _.session.send("Network.setCacheDisabled", params, &fbuf)?;
        * = _ign;
    }

    pub fn setBypassCSP(*_, enabled: bool): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.bypass_csp = enabled;
        let fbuf: [512]u8 = undefined;
        const params = if (enabled) "{\"enabled\":true}" else "{\"enabled\":false}";
        const _ign = _.session.send("Page.setBypassCSP", params, &fbuf)?;
        * = _ign;
    }

    pub fn setJavaScriptEnabled(*_, enabled: bool): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.js_enabled = enabled;
        let fbuf: [512]u8 = undefined;
        const params = if (enabled) "{\"value\":false}" else "{\"value\":true}";
        const _ign = _.session.send("Emulation.setScriptExecutionDisabled", params, &fbuf)?;
        * = _ign;
    }

    pub fn isJavaScriptEnabled(): bool {
        return _.js_enabled;
    }

    pub fn setBypassServiceWorker(*_, bypass: bool): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.bypass_service_worker = bypass;
        let fbuf: [512]u8 = undefined;
        const params = if (bypass) "{\"bypass\":true}" else "{\"bypass\":false}";
        const _ign = _.session.send("Network.setBypassServiceWorker", params, &fbuf)?;
        * = _ign;
    }

    pub fn isServiceWorkerBypassed(): bool {
        return _.bypass_service_worker;
    }

    pub fn setDragInterception(*_, enabled: bool): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        _.drag_interception_enabled = enabled;
        let fbuf: [512]u8 = undefined;
        const params = if (enabled) "{\"enabled\":true}" else "{\"enabled\":false}";
        const _ign = _.session.send("Input.setInterceptDrags", params, &fbuf)?;
        * = _ign;
    }

    pub fn authenticate(*_, credentials: Credentials?): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let fbuf: [512]u8 = undefined;
        if (creds := credentials) {
            _.has_credentials = true;
            _.auth_credentials = creds;
            const _ign = _.session.send("Fetch.enable", "{\"handleAuthRequests\":true}", &fbuf)?;
            * = _ign;
        } else {
            _.has_credentials = false;
            _.auth_credentials = Credentials{};
            const _ign = _.session.send("Fetch.disable", "{}", &fbuf)?;
            * = _ign;
        }
    }

    // --- Interaction, Navigation, Frames, Workers & Metrics ---

    pub fn bringToFront(*_): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let fbuf: [512]u8 = undefined;
        const _ign = _.session.send("Page.bringToFront", "{}", &fbuf)?;
        * = _ign;
    }

    pub fn evalOnSelector(
        *_,
        selector: []const u8,
        page_function: []const u8,
        out_buf: []u8,
    ): EvaluateResult!PuppeteerError {
        _.bindSelf();
        return _.main_frame.evalOnSelector(selector, page_function, out_buf);
    }

    pub fn evalOnSelectorAll(
        *_,
        selector: []const u8,
        page_function: []const u8,
        out_buf: []u8,
    ): EvaluateResult!PuppeteerError {
        _.bindSelf();
        return _.main_frame.evalOnSelectorAll(selector, page_function, out_buf);
    }

    pub fn select(
        *_,
        selector: []const u8,
        values: []const []const u8,
        out_buf: []u8,
    ): ([]const u8)!PuppeteerError {
        _.bindSelf();
        return _.main_frame.select(selector, values, out_buf);
    }

    pub fn addScriptTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.addScriptTag(url_or_content, is_url);
    }

    pub fn addStyleTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.addStyleTag(url_or_content, is_url);
    }

    pub fn exposeFunction(*_, name: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(&pbuf, "{{\"name\":\"{s}\"}}", .{name}) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Runtime.addBinding", params, &fbuf)?;
        * = _ign;
        if (_.exposed_functions_len < 16) {
            _.exposed_functions[_.exposed_functions_len] = name;
            _.exposed_functions_len += 1;
        }
    }

    pub fn removeExposedFunction(*_, name: []const u8): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(&pbuf, "{{\"name\":\"{s}\"}}", .{name}) catch return PuppeteerError.BufferTooSmall;
        const _ign = _.session.send("Runtime.removeBinding", params, &fbuf)?;
        * = _ign;
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.exposed_functions_len) {
            if (!std.mem.eql(u8, _.exposed_functions[read_idx], name)) {
                _.exposed_functions[write_idx] = _.exposed_functions[read_idx];
                write_idx += 1;
            }
            read_idx += 1;
        }
        _.exposed_functions_len = write_idx;
    }

    pub fn metrics(*_): PageMetrics!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        let fbuf: [512]u8 = undefined;
        const req_id = _.session.send("Performance.getMetrics", "{}", &fbuf)?;
        let out = PageMetrics{};
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ProtocolError;
            if (m_arr := CdpFrameScanner.extractTopLevelField(resp.result_json, "metrics", false)) {
                let i: usize = 0;
                while (i < m_arr.len) {
                    if (m_arr[i] == '{') {
                        const start = i;
                        let depth: i32 = 0;
                        let in_str = false;
                        while (i < m_arr.len) {
                            const ch = m_arr[i];
                            if (ch == '"' && (i == 0 || m_arr[i - 1] != '\\')) {
                                in_str = !in_str;
                            } else if (!in_str) {
                                if (ch == '{') depth += 1;
                                if (ch == '}') {
                                    depth -= 1;
                                    if (depth == 0) {
                                        const obj = m_arr[start .. i + 1];
                                        const mname = CdpFrameScanner.extractTopLevelField(obj, "name", true) orelse "";
                                        const mval_str = CdpFrameScanner.extractTopLevelField(obj, "value", false) orelse "0";
                                        const mval = std.fmt.parseFloat(f64, mval_str) catch 0.0;
                                        if (std.mem.eql(u8, mname, "Timestamp")) {
                                            out.timestamp = mval;
                                        } else if (std.mem.eql(u8, mname, "Documents")) {
                                            out.documents = mval;
                                        } else if (std.mem.eql(u8, mname, "Frames")) {
                                            out.frames = mval;
                                        } else if (std.mem.eql(u8, mname, "JSEventListeners")) {
                                            out.js_event_listeners = mval;
                                        } else if (std.mem.eql(u8, mname, "Nodes")) {
                                            out.nodes = mval;
                                        } else if (std.mem.eql(u8, mname, "LayoutCount")) {
                                            out.layout_count = mval;
                                        } else if (std.mem.eql(u8, mname, "RecalcStyleCount")) {
                                            out.recalc_style_count = mval;
                                        } else if (std.mem.eql(u8, mname, "LayoutDuration")) {
                                            out.layout_duration = mval;
                                        } else if (std.mem.eql(u8, mname, "RecalcStyleDuration")) {
                                            out.recalc_style_duration = mval;
                                        } else if (std.mem.eql(u8, mname, "ScriptDuration")) {
                                            out.script_duration = mval;
                                        } else if (std.mem.eql(u8, mname, "TaskDuration")) {
                                            out.task_duration = mval;
                                        } else if (std.mem.eql(u8, mname, "JSHeapUsedSize")) {
                                            out.js_heap_used_size = mval;
                                        } else if (std.mem.eql(u8, mname, "JSHeapTotalSize")) {
                                            out.js_heap_total_size = mval;
                                        }
                                        break;
                                    }
                                }
                            }
                            i += 1;
                        }
                    }
                    i += 1;
                }
            }
        }
        return out;
    }

    pub fn workers(): []const WebWorkerInfo {
        return _.workers_list[0.._.workers_len];
    }

    pub fn registerWorker(*_, info: WebWorkerInfo) {
        if (_.workers_len < 8) {
            _.workers_list[_.workers_len] = info;
            _.workers_len += 1;
        }
    }

    pub fn frames(*_): []const *Frame {
        _.bindSelf();
        _.frame_ptrs[0] = &_.main_frame;
        const children = _.main_frame.childFrames();
        let count: usize = 1;
        let i: usize = 0;
        while (i < children.len && count < 17) {
            _.frame_ptrs[count] = children[i];
            count += 1;
            i += 1;
        }
        return _.frame_ptrs[0..count];
    }

    pub fn locator(*_, selector: []const u8): PuppeteerLocator {
        _.bindSelf();
        return _.main_frame.locator(selector);
    }

    // --- Waiters ---

    pub fn waitForNavigation(*_, options: GoToOptions): void!PuppeteerError {
        _.bindSelf();
        return _.main_frame.waitForNavigation(options);
    }

    pub fn recordRequest(*_, req: HTTPRequest) {
        if (_.recorded_requests_len < 16) {
            _.recorded_requests[_.recorded_requests_len] = req;
            _.recorded_requests_len += 1;
        }
    }

    pub fn recordResponse(*_, resp: HTTPResponse) {
        if (_.recorded_responses_len < 16) {
            _.recorded_responses[_.recorded_responses_len] = resp;
            _.recorded_responses_len += 1;
        }
    }

    pub fn waitForRequest(*_, url_pattern: []const u8, timeout_ms: u32): HTTPRequest!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const _ign_t = timeout_ms;
        * = _ign_t;
        let i: usize = 0;
        while (i < _.recorded_requests_len) {
            if (url_pattern.len == 0 || std.mem.indexOf(u8, _.recorded_requests[i].url(), url_pattern) != null) {
                return _.recorded_requests[i];
            }
            i += 1;
        }
        return PuppeteerError.Timeout;
    }

    pub fn waitForResponse(*_, url_pattern: []const u8, timeout_ms: u32): HTTPResponse!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const _ign_t = timeout_ms;
        * = _ign_t;
        let i: usize = 0;
        while (i < _.recorded_responses_len) {
            if (url_pattern.len == 0 || std.mem.indexOf(u8, _.recorded_responses[i].url(), url_pattern) != null) {
                return _.recorded_responses[i];
            }
            i += 1;
        }
        return PuppeteerError.Timeout;
    }

    pub fn waitForFrame(*_, url_or_name: []const u8, timeout_ms: u32): (*Frame)!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const _ign_t = timeout_ms;
        * = _ign_t;
        const all_frames = _.frames();
        for (f of all_frames) {
            if (url_or_name.len == 0) return f;
            if (std.mem.indexOf(u8, f.url(), url_or_name) != null || std.mem.indexOf(u8, f.name(), url_or_name) != null) {
                return f;
            }
        }
        return PuppeteerError.Timeout;
    }

    pub fn waitForNetworkIdle(*_, options: WaitForNetworkIdleOptions): void!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        if (_.inflight_requests > options.concurrency) {
            return PuppeteerError.Timeout;
        }
    }

    pub fn waitForFileChooser(*_, timeout_ms: u32): FileChooser!PuppeteerError {
        if (_.isClosed()) return PuppeteerError.SessionClosed;
        const _ign_t = timeout_ms;
        * = _ign_t;
        let fbuf: [512]u8 = undefined;
        const _ign_fc = _.session.send("Page.setInterceptFileChooserDialog", "{\"enabled\":true}", &fbuf)?;
        * = _ign_fc;
        _.bindSelf();
        const el = _.main_frame.querySelector("input[type=file]") catch ElementHandle.init(&_.session, "input[type=file]", 77, "");
        return FileChooser.init(el, false);
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
