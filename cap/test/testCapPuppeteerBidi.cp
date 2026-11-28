// ============================================================================
// cap/test/testCapPuppeteerBidi.cp — Production Test Suite for WebDriver BiDi
// Covers BidiConnection SIMD framing & event dispatch, BidiSession/UserContext/
// Browser, BidiBrowsingContext/Navigation/Frame/Realm/Dialog, BidiNetworkIntercept/
// Request/Response, BidiInput/Keyboard/Mouse, and DebugAllocator zero-leak check.
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BidiError,
    BidiMessageType,
    BidiCommand,
    BidiEvent,
    BidiFrameScanner,
    BidiConnection,
    BidiSession,
    BidiUserContext,
    BidiBrowser,
    BidiNavigationState,
    BidiNavigation,
    BidiDialog,
    BidiRealm,
    BidiBrowsingContext,
    BidiFrame,
    BidiNetworkIntercept,
    BidiRequest,
    BidiResponse,
    BidiKeyboard,
    BidiMouse,
    BidiInput,
    getBidiKeyJsonValue,
    SessionCapabilitiesRequest,
    CreateUserContextParams,
    ProxyConfiguration,
    ProxyType,
    DownloadBehaviorParams,
    DownloadBehaviorPolicy,
    CreateContextType,
    ReadinessState,
    CaptureScreenshotParams,
    CaptureScreenshotOrigin,
    PrintParams,
    PrintOrientation,
    SetViewportParams,
    BidiLocator,
    LocatorType,
    UserPromptType,
    EvaluateResultKind,
    InterceptPhase,
    UrlPattern,
    UrlPatternType,
    BidiHeader,
    ContinueRequestParams,
    ProvideResponseParams,
    ContinueResponseParams,
    AuthAction,
    ContinueWithAuthParams,
    BidiMouseButton,
} from "../../packages/puppeteer/src/bidi/index.cp";

struct MockBidiServer {
    conn: *BidiConnection = undefined,
    has_conn: bool = false,
    send_count: usize = 0,
    last_method_buf: [128]u8 = undefined,
    last_method_len: usize = 0,
    last_payload_buf: [4096]u8 = undefined,
    last_payload_len: usize = 0,

    fn lastMethod(): []const u8 {
        return _.last_method_buf[0.._.last_method_len];
    }

    fn lastPayload(): []const u8 {
        return _.last_payload_buf[0.._.last_payload_len];
    }

    fn handleSend(*_, payload: []const u8): bool {
        _.send_count += 1;
        const plen = if (payload.len <= 4096) payload.len else 4096;
        @memcpy(_.last_payload_buf[0..plen], payload[0..plen]);
        _.last_payload_len = plen;

        const id_str = BidiFrameScanner.extractField(payload, "id", false) orelse "1";
        const method = BidiFrameScanner.extractField(payload, "method", true) orelse "";
        const mlen = if (method.len <= 128) method.len else 128;
        if (mlen > 0) @memcpy(_.last_method_buf[0..mlen], method[0..mlen]);
        _.last_method_len = mlen;

        if (!_.has_conn) return true;

        let result_json: []const u8 = "{}";
        if (std.mem.eql(u8, method, "session.new")) {
            result_json = "{\"sessionId\":\"bidi-sess-001\",\"capabilities\":{\"acceptInsecureCerts\":true,\"browserName\":\"chrome\",\"browserVersion\":\"131.0.6778.85\",\"userAgent\":\"Mozilla/5.0 BiDiChrome/131.0\"}}";
        } else if (std.mem.eql(u8, method, "session.status")) {
            result_json = "{\"ready\":true,\"message\":\"WebDriver BiDi ready\"}";
        } else if (std.mem.eql(u8, method, "browser.getUserContexts")) {
            result_json = "{\"userContexts\":[{\"userContext\":\"default\"},{\"userContext\":\"uc-existing-1\"}]}";
        } else if (std.mem.eql(u8, method, "browser.createUserContext")) {
            result_json = "{\"userContext\":\"uc-incognito-42\"}";
        } else if (std.mem.eql(u8, method, "browsingContext.create")) {
            result_json = "{\"context\":\"ctx-tab-101\"}";
        } else if (std.mem.eql(u8, method, "browsingContext.navigate")) {
            result_json = "{\"navigation\":\"nav-777\",\"url\":\"https://example.com/bidi\"}";
        } else if (std.mem.eql(u8, method, "browsingContext.reload")) {
            result_json = "{\"navigation\":\"nav-778\",\"url\":\"https://example.com/bidi\"}";
        } else if (std.mem.eql(u8, method, "browsingContext.captureScreenshot")) {
            result_json = "{\"data\":\"iVBORw0KGgoBiDiPng==\"}";
        } else if (std.mem.eql(u8, method, "browsingContext.print")) {
            result_json = "{\"data\":\"JVBERi0xLjQBiDiPdf==\"}";
        } else if (std.mem.eql(u8, method, "browsingContext.locateNodes")) {
            result_json = "{\"nodes\":[{\"type\":\"node\",\"sharedId\":\"node-shared-99\"}]}";
        } else if (std.mem.eql(u8, method, "script.evaluate")) {
            if (std.mem.indexOf(u8, payload, "throw") != null) {
                result_json = "{\"type\":\"exception\",\"realm\":\"realm-1\",\"exceptionDetails\":{\"text\":\"ReferenceError: boom\",\"lineNumber\":1,\"columnNumber\":4}}";
            } else {
                result_json = "{\"type\":\"success\",\"realm\":\"realm-1\",\"result\":{\"type\":\"number\",\"value\":42,\"handle\":\"h-42\"}}";
            }
        } else if (std.mem.eql(u8, method, "script.callFunction")) {
            result_json = "{\"type\":\"success\",\"realm\":\"realm-1\",\"result\":{\"type\":\"string\",\"value\":\"hello-bidi\"}}";
        } else if (std.mem.eql(u8, method, "script.addPreloadScript")) {
            result_json = "{\"script\":\"preload-script-9\"}";
        } else if (std.mem.eql(u8, method, "network.addIntercept")) {
            result_json = "{\"intercept\":\"intercept-id-55\"}";
        } else if (std.mem.eql(u8, method, "storage.getCookies")) {
            result_json = "{\"cookies\":[{\"name\":\"sid\",\"value\":{\"type\":\"string\",\"value\":\"abc\"}}]}";
        }

        let rbuf: [2048]u8 = undefined;
        const resp = std.fmt.bufPrint(
            &rbuf,
            "{{\"type\":\"success\",\"id\":{s},\"result\":{s}}}",
            .{ id_str, result_json },
        ) catch return false;
        const msg = _.conn.onMessage(resp) catch return false;
        return !msg.isError();
    }
}

fn mockBidiTransportCb(ctx: usize, payload: []const u8): bool {
    const srv = unsafe { ctx as *MockBidiServer! };
    return srv.handleSend(payload);
}

struct EventCounter {
    exact_hits: usize = 0,
    prefix_hits: usize = 0,
    wildcard_hits: usize = 0,
    once_hits: usize = 0,
    pipe_hits: usize = 0,
}

fn onExactEvent(ctx: usize, ev: BidiEvent) {
    if (ev.method.len == 0) return;
    const c = unsafe { ctx as *EventCounter! };
    c.exact_hits += 1;
}

fn onPrefixEvent(ctx: usize, ev: BidiEvent) {
    if (ev.method.len == 0) return;
    const c = unsafe { ctx as *EventCounter! };
    c.prefix_hits += 1;
}

fn onWildcardEvent(ctx: usize, ev: BidiEvent) {
    if (ev.method.len == 0) return;
    const c = unsafe { ctx as *EventCounter! };
    c.wildcard_hits += 1;
}

fn onOnceEvent(ctx: usize, ev: BidiEvent) {
    if (ev.method.len == 0) return;
    const c = unsafe { ctx as *EventCounter! };
    c.once_hits += 1;
}

fn onPipeEvent(ctx: usize, ev: BidiEvent) {
    if (ev.method.len == 0) return;
    const c = unsafe { ctx as *EventCounter! };
    c.pipe_hits += 1;
}

fn testFrameScannerSimdAndCodec(): !void {
    const sample = "0123456789abcdef\"type\":\"event\"";
    expect_eq(BidiFrameScanner.indexOfByte(sample, '"').?, 16);
    expect_eq(BidiFrameScanner.indexOfBidiDelimiter(sample).?, 16);

    let buf: [256]u8 = undefined;
    const n = BidiFrameScanner.encodeCommand(BidiCommand{
        id: 7,
        method: "session.status",
        params_json: "{}",
    }, &buf)?;
    expect_eq(std.mem.eql(u8, buf[0..n], "{\"id\":7,\"method\":\"session.status\",\"params\":{}}"), true);

    const ok_msg = BidiFrameScanner.decodeMessage("{\"type\":\"success\",\"id\":7,\"result\":{\"ready\":true}}")?;
    expect_eq(ok_msg.isSuccess(), true);
    expect_eq(ok_msg.id, 7);
    expect_eq(std.mem.eql(u8, ok_msg.result_json, "{\"ready\":true}"), true);

    const err_msg = BidiFrameScanner.decodeMessage("{\"type\":\"error\",\"id\":8,\"error\":\"no such frame\",\"message\":\"Context missing\",\"stacktrace\":\"at line 1\"}")?;
    expect_eq(err_msg.isError(), true);
    expect_eq(err_msg.id, 8);
    expect_eq(std.mem.eql(u8, err_msg.error_code, "no such frame"), true);
    expect_eq(std.mem.eql(u8, err_msg.error_message, "Context missing"), true);

    const ev_msg = BidiFrameScanner.decodeMessage("{\"type\":\"event\",\"method\":\"goog:cdp.Page.loadEventFired\",\"params\":{\"session\":\"s1\"}}")?;
    expect_eq(ev_msg.isEvent(), true);
    expect_eq(ev_msg.isCdpEvent(), true);
}

fn testConnectionCorrelationAndEvents(): !void {
    let dba: std.heap.DebugAllocator(.{}) = .{};
    const alloc = dba.allocator();
    {
        let conn = BidiConnection.init(alloc, "ws://127.0.0.1:9222/session");
        let srv = MockBidiServer{};
        srv.conn = *conn;
        srv.has_conn = true;
        conn.setTransport(mockBidiTransportCb, unsafe { @intFromPtr(&srv) });

        let counter = EventCounter{};
        const c_ptr = unsafe { @intFromPtr(&counter) };
        conn.on("network.beforeRequestSent", onExactEvent, c_ptr)?;
        conn.on("network.*", onPrefixEvent, c_ptr)?;
        conn.on("*", onWildcardEvent, c_ptr)?;
        conn.once("network.beforeRequestSent", onOnceEvent, c_ptr)?;
        conn.pipeTo(onPipeEvent, c_ptr)?;

        const m1 = conn.onMessage("{\"type\":\"event\",\"method\":\"network.beforeRequestSent\",\"params\":{\"context\":\"c1\"}}")?;
        const m2 = conn.onMessage("{\"type\":\"event\",\"method\":\"network.beforeRequestSent\",\"params\":{\"context\":\"c1\"}}")?;
        const m3 = conn.onMessage("{\"type\":\"event\",\"method\":\"browsingContext.load\",\"params\":{\"context\":\"c1\"}}")?;
        expect_eq(m1.isEvent(), true);
        expect_eq(m2.isEvent(), true);
        expect_eq(m3.isEvent(), true);

        expect_eq(counter.exact_hits, 2);
        expect_eq(counter.prefix_hits, 2);
        expect_eq(counter.wildcard_hits, 3);
        expect_eq(counter.once_hits, 1);
        expect_eq(counter.pipe_hits, 3);
        expect_eq(conn.total_events, 3);

        let frame_buf: [256]u8 = undefined;
        srv.has_conn = false;
        const req_id = conn.sendCommand("browsingContext.close", "{\"context\":\"missing\"}", &frame_buf)?;
        expect_eq(conn.pendingCount(), 1);
        const err_m = conn.onMessage("{\"type\":\"error\",\"id\":1,\"error\":\"no such frame\",\"message\":\"not found\"}")?;
        expect_eq(err_m.isError(), true);
        const p = conn.takePending(req_id).?;
        expect_eq(p.completed, true);
        expect_eq(p.is_error, true);
        expect_eq(std.mem.eql(u8, p.error_code, "no such frame"), true);

        conn.close();
        expect_eq(conn.closed(), true);
    }
    expect_eq(dba.deinit() == .ok, true);
}

fn testSessionUserContextAndBrowser(): !void {
    let dba: std.heap.DebugAllocator(.{}) = .{};
    const alloc = dba.allocator();
    {
        let conn = BidiConnection.init(alloc, "ws://127.0.0.1:9222/session");
        let srv = MockBidiServer{};
        srv.conn = *conn;
        srv.has_conn = true;
        conn.setTransport(mockBidiTransportCb, unsafe { @intFromPtr(&srv) });

        let session = BidiSession.init(*conn);
        session.start(SessionCapabilitiesRequest{
            accept_insecure_certs: true,
            browser_name: "chrome",
        })?;

        expect_eq(std.mem.eql(u8, session.id(), "bidi-sess-001"), true);
        expect_eq(std.mem.eql(u8, session.browserName(), "chrome"), true);
        expect_eq(session.accept_insecure_certs, true);

        const st = session.status()?;
        expect_eq(st.ready, true);

        const events: [2][]const u8 = .{ "browsingContext.load", "network.beforeRequestSent" };
        session.subscribe(&events, events[0..0])?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "session.subscribe"), true);

        let browser = BidiBrowser.init(alloc, *session);
        const synced = browser.getUserContexts()?;
        expect_eq(synced, 2);
        expect_eq(browser.hasUserContext("default"), true);
        expect_eq(browser.hasUserContext("uc-existing-1"), true);

        let uc = browser.createUserContext(CreateUserContextParams{
            accept_insecure_certs: true,
            has_proxy: true,
            proxy: ProxyConfiguration{
                proxy_type: ProxyType.Manual,
                http_proxy: "http://127.0.0.1:8080",
            },
        })?;
        expect_eq(std.mem.eql(u8, uc.id(), "uc-incognito-42"), true);
        expect_eq(uc.isDefault(), false);
        expect_eq(browser.userContextCount(), 3);

        let cid_buf: [64]u8 = undefined;
        const new_ctx_id = uc.createBrowsingContext(CreateContextType.Tab, false, &cid_buf)?;
        expect_eq(std.mem.eql(u8, new_ctx_id, "ctx-tab-101"), true);

        browser.setDownloadBehavior(DownloadBehaviorParams{
            policy: DownloadBehaviorPolicy.Allowed,
            destination_folder: "/tmp/bidi-downloads",
        })?;

        browser.removeUserContext("uc-incognito-42")?;
        expect_eq(browser.hasUserContext("uc-incognito-42"), false);

        browser.close()?;
        expect_eq(browser.closed, true);
        session.end()?;
        expect_eq(session.ended, true);
    }
    expect_eq(dba.deinit() == .ok, true);
}

fn testBrowsingContextNavigationRealmAndDialog(): !void {
    let dba: std.heap.DebugAllocator(.{}) = .{};
    const alloc = dba.allocator();
    {
        let conn = BidiConnection.init(alloc, "ws://127.0.0.1:9222/session");
        let srv = MockBidiServer{};
        srv.conn = *conn;
        srv.has_conn = true;
        conn.setTransport(mockBidiTransportCb, unsafe { @intFromPtr(&srv) });

        let session = BidiSession.init(*conn);
        session.start(SessionCapabilitiesRequest{})?;

        let ctx = BidiBrowsingContext.create(*session, CreateContextType.Tab, "default", false)?;
        expect_eq(std.mem.eql(u8, ctx.id(), "ctx-tab-101"), true);

        let nav = ctx.navigate("https://example.com/bidi", ReadinessState.Complete)?;
        expect_eq(std.mem.eql(u8, nav.id(), "nav-777"), true);
        expect_eq(std.mem.eql(u8, ctx.url(), "https://example.com/bidi"), true);
        expect_eq(nav.state == BidiNavigationState.Loaded, true);

        nav.applyEvent("browsingContext.fragmentNavigated", "{\"navigation\":\"nav-777\",\"url\":\"https://example.com/bidi#sec\",\"timestamp\":1700000}");
        expect_eq(nav.state == BidiNavigationState.FragmentNavigated, true);
        expect_eq(nav.timestamp, 1700000);

        const rel = ctx.reload(true, ReadinessState.Interactive)?;
        expect_eq(std.mem.eql(u8, rel.navigation, "nav-778"), true);

        const png = ctx.captureScreenshot(CaptureScreenshotParams{
            origin: CaptureScreenshotOrigin.Document,
            format_type: "image/png",
        })?;
        expect_eq(std.mem.eql(u8, png, "iVBORw0KGgoBiDiPng=="), true);

        const pdf = ctx.print(PrintParams{
            background: true,
            orientation: PrintOrientation.Landscape,
        })?;
        expect_eq(std.mem.eql(u8, pdf, "JVBERi0xLjQBiDiPdf=="), true);

        ctx.setViewport(SetViewportParams{ width: 1440, height: 900, device_pixel_ratio: 2.0 })?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "browsingContext.setViewport"), true);

        const nodes = ctx.locateNodes(BidiLocator{
            locator_type: LocatorType.Css,
            value: "#submit-btn",
        }, 5)?;
        expect_eq(std.mem.indexOf(u8, nodes, "node-shared-99") != null, true);

        const eval_ok = ctx.evaluate("21 * 2", true)?;
        expect_eq(eval_ok.kind == EvaluateResultKind.Success, true);
        expect_eq(std.mem.eql(u8, eval_ok.result.value_raw, "42"), true);

        const eval_err = ctx.evaluate("throw new Error()", true)?;
        expect_eq(eval_err.kind == EvaluateResultKind.Exception, true);
        expect_eq(std.mem.eql(u8, eval_err.exception.text, "ReferenceError: boom"), true);

        const fn_res = ctx.callFunction("() => 'hello-bidi'", "[]", true)?;
        expect_eq(std.mem.eql(u8, fn_res.result.value_raw, "hello-bidi"), true);

        const preload_id = ctx.addPreloadScript("() => { window.__bidi = true; }", "")?;
        expect_eq(std.mem.eql(u8, preload_id, "preload-script-9"), true);

        const handles: [1][]const u8 = .{"h-42"};
        ctx.disown(&handles)?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "script.disown"), true);

        let dialog = BidiDialog.init(*session, ctx.id(), UserPromptType.Prompt, "Enter name");
        dialog.accept("Alice")?;
        expect_eq(dialog.handled, true);
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "browsingContext.handleUserPrompt"), true);

        let frame = BidiFrame.init(*ctx, "main");
        expect_eq(std.mem.eql(u8, frame.name(), "main"), true);
        expect_eq(frame.detached(), false);

        ctx.close(false)?;
        expect_eq(frame.detached(), true);
    }
    expect_eq(dba.deinit() == .ok, true);
}

fn testNetworkInterceptAndInput(): !void {
    let dba: std.heap.DebugAllocator(.{}) = .{};
    const alloc = dba.allocator();
    {
        let conn = BidiConnection.init(alloc, "ws://127.0.0.1:9222/session");
        let srv = MockBidiServer{};
        srv.conn = *conn;
        srv.has_conn = true;
        conn.setTransport(mockBidiTransportCb, unsafe { @intFromPtr(&srv) });

        let session = BidiSession.init(*conn);
        session.start(SessionCapabilitiesRequest{})?;

        let net_mgr = BidiNetworkIntercept.init(alloc, *session);
        const phases: [2]InterceptPhase = .{ InterceptPhase.BeforeRequestSent, InterceptPhase.ResponseStarted };
        const contexts: [1][]const u8 = .{"ctx-tab-101"};
        const patterns: [1]UrlPattern = .{
            UrlPattern{ pattern_type: UrlPatternType.String, pattern: "https://api.example.com/*" },
        };
        const iid = net_mgr.addIntercept(&phases, &contexts, &patterns)?;
        expect_eq(std.mem.eql(u8, iid, "intercept-id-55"), true);
        expect_eq(net_mgr.activeCount(), 1);
        expect_eq(net_mgr.hasIntercept("intercept-id-55"), true);

        let req = BidiRequest.fromEventJson(
            *session,
            "{\"context\":\"ctx-tab-101\",\"isBlocked\":true,\"navigation\":\"nav-1\",\"redirectCount\":0,\"request\":{\"request\":\"req-900\",\"url\":\"https://api.example.com/v1\",\"method\":\"POST\"}}",
        )?;
        expect_eq(std.mem.eql(u8, req.id(), "req-900"), true);
        expect_eq(std.mem.eql(u8, req.method(), "POST"), true);
        expect_eq(req.is_blocked, true);

        const hdrs: [1]BidiHeader = .{ BidiHeader{ name: "X-Bidi", value: "1" } };
        req.continueRequest(ContinueRequestParams{
            method: "PUT",
            headers: &hdrs,
        })?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "network.continueRequest"), true);
        expect_eq(req.handled, true);

        req.provideResponse(ProvideResponseParams{
            status_code: 201,
            reason_phrase: "Created",
            body: "{\"ok\":true}",
            headers: &hdrs,
        })?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "network.provideResponse"), true);

        req.continueWithAuth(ContinueWithAuthParams{
            action: AuthAction.ProvideCredentials,
            username: "u",
            password: "p",
        })?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "network.continueWithAuth"), true);

        req.failRequest()?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "network.failRequest"), true);

        let resp = BidiResponse.fromEventJson(
            *session,
            "{\"request\":{\"request\":\"req-900\"},\"response\":{\"url\":\"https://api.example.com/v1\",\"status\":200,\"statusText\":\"OK\",\"fromCache\":false,\"bytesReceived\":1024,\"protocol\":\"h2\",\"mimeType\":\"application/json\"}}",
        )?;
        expect_eq(resp.status, 200);
        expect_eq(resp.ok(), true);
        expect_eq(resp.bytes_received, 1024);

        resp.continueResponse(ContinueResponseParams{ status_code: 202, reason_phrase: "Accepted" })?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "network.continueResponse"), true);

        net_mgr.removeIntercept("intercept-id-55")?;
        expect_eq(net_mgr.activeCount(), 0);

        expect_eq(std.mem.eql(u8, getBidiKeyJsonValue("Enter"), "\\uE006"), true);
        expect_eq(std.mem.eql(u8, getBidiKeyJsonValue("ArrowDown"), "\\uE015"), true);
        expect_eq(std.mem.eql(u8, getBidiKeyJsonValue("Escape"), "\\uE00C"), true);

        let inp = BidiInput.init(*session, "ctx-tab-101");
        inp.keyboard.press("Enter", 10)?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "input.performActions"), true);
        expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "\\uE006") != null, true);

        inp.keyboard.typeText("hi", 0)?;
        expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "__puppeteer_keyboard") != null, true);

        inp.mouse.move(120.4, 240.6, 5)?;
        expect_eq(inp.mouse.x, 120.4);
        expect_eq(inp.mouse.y, 240.6);

        inp.mouse.click(120.0, 240.0, BidiMouseButton.Left, 2)?;
        expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "pointerDown") != null, true);

        inp.mouse.wheel(0, 120)?;
        expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "\"deltaY\":120") != null, true);

        inp.releaseActions()?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "input.releaseActions"), true);

        const files: [2][]const u8 = .{ "/tmp/a.txt", "/tmp/b.png" };
        inp.setFiles("node-shared-99", &files)?;
        expect_eq(std.mem.eql(u8, srv.lastMethod(), "input.setFiles"), true);
        expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "node-shared-99") != null, true);
    }
    expect_eq(dba.deinit() == .ok, true);
}

pub fn main() {
    test("1. BidiFrameScanner 16-Lane SIMD & Codec (Success/Error/Event/CDP)", () => {
        testFrameScannerSimdAndCodec() catch unreachable;
    });
    test("2. BidiConnection Correlation, Multi-Subscriber Events & PipeTo", () => {
        testConnectionCorrelationAndEvents() catch unreachable;
    });
    test("3. BidiSession, BidiUserContext & BidiBrowser Lifecycle (Zero-Leak)", () => {
        testSessionUserContextAndBrowser() catch unreachable;
    });
    test("4. BidiBrowsingContext, Navigation, Frame, Realm & Dialog", () => {
        testBrowsingContextNavigationRealmAndDialog() catch unreachable;
    });
    test("5. BidiNetworkIntercept, Request/Response & BidiInput Actions", () => {
        testNetworkInterceptAndInput() catch unreachable;
    });
}
