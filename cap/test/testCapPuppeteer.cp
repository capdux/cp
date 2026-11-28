// ============================================================================
// cap/test/testCapPuppeteer.cp — Production Test Suite for packages/puppeteer
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { CdpFrameScanner } from "cap:cdp";
import {
    Puppeteer,
    PuppeteerError,
    Browser,
    BrowserContext,
    Page,
    Frame,
    ElementHandle,
    JSHandle,
    CDPSession,
    Keyboard,
    Mouse,
    Touchscreen,
    HTTPRequest,
    HTTPResponse,
    Dialog,
    DialogType,
    ResourceType,
    LaunchOptions,
    ConnectOptions,
    GoToOptions,
    Viewport,
    ScreenshotOptions,
    PdfOptions,
    ClickOptions,
    TypeOptions,
    WaitForSelectorOptions,
    AccessibilitySnapshotOptions,
    CookieParam,
    HttpHeader,
    CdpEvent,
    launch,
    launchWithTransport,
    connect,
    connectWithTransport,
    defaultExecutablePath,
    Device,
    KnownDevices,
    USKeyboardLayout,
    PSelectorKind,
    PCombinator,
    SimdSelectorScanner,
    PSelectorParser,
    WaitTaskPolling,
    WaitForFunctionOptions,
    WaitTask,
    PuppeteerEventEmitter,
    FileChooser,
    ConsoleMessageType,
    ConsoleMessage,
    SecurityDetails,
    TimeoutSettings,
    Tracing,
    TracingOptions,
    Coverage,
    JSCoverageOptions,
    CSSCoverageOptions,
    CoverageRange,
    convertToDisjointRanges,
} from "../../packages/puppeteer/src/index.cp";

struct MockCdpServer {
    browser: *Browser = undefined,
    has_browser: bool = false,
    send_count: usize = 0,
    last_method_buf: [128]u8 = undefined,
    last_method_len: usize = 0,
    last_session_buf: [128]u8 = undefined,
    last_session_len: usize = 0,
    last_payload_buf: [2048]u8 = undefined,
    last_payload_len: usize = 0,

    fn lastMethod(): []const u8 {
        return _.last_method_buf[0.._.last_method_len];
    }

    fn lastSession(): []const u8 {
        return _.last_session_buf[0.._.last_session_len];
    }

    fn lastPayload(): []const u8 {
        return _.last_payload_buf[0.._.last_payload_len];
    }

    fn handleSend(*_, payload: []const u8): bool {
        _.send_count += 1;
        const plen = if (payload.len <= 2048) payload.len else 2048;
        @memcpy(_.last_payload_buf[0..plen], payload[0..plen]);
        _.last_payload_len = plen;

        const id_str = CdpFrameScanner.extractTopLevelField(payload, "id", false) orelse "1";
        const method = CdpFrameScanner.extractTopLevelField(payload, "method", true) orelse "";
        const sid = CdpFrameScanner.extractTopLevelField(payload, "sessionId", true) orelse "";

        const mlen = if (method.len <= 128) method.len else 128;
        if (mlen > 0) @memcpy(_.last_method_buf[0..mlen], method[0..mlen]);
        _.last_method_len = mlen;

        const slen = if (sid.len <= 128) sid.len else 128;
        if (slen > 0) @memcpy(_.last_session_buf[0..slen], sid[0..slen]);
        _.last_session_len = slen;

        if (!_.has_browser) return true;

        let rbuf: [2048]u8 = undefined;
        let result_json: []const u8 = "{}";
        if (std.mem.eql(u8, method, "Browser.getVersion")) {
            result_json = "{\"product\":\"Chrome/131.0.6778.85\",\"userAgent\":\"Mozilla/5.0 HeadlessChrome/131.0\"}";
        } else if (std.mem.eql(u8, method, "Target.createBrowserContext")) {
            result_json = "{\"browserContextId\":\"ctx-incognito-1\"}";
        } else if (std.mem.eql(u8, method, "Target.createTarget")) {
            result_json = "{\"targetId\":\"target-page-1\"}";
        } else if (std.mem.eql(u8, method, "Target.attachToTarget")) {
            result_json = "{\"sessionId\":\"sess-page-1\"}";
        } else if (std.mem.eql(u8, method, "Runtime.evaluate")) {
            if (std.mem.indexOf(u8, payload, "document.title") != null) {
                result_json = "{\"result\":{\"type\":\"string\",\"value\":\"Example Domain\"}}";
            } else if (std.mem.indexOf(u8, payload, "outerHTML") != null) {
                result_json = "{\"result\":{\"type\":\"string\",\"value\":\"<html><body><h1>Hello</h1></body></html>\"}}";
            } else {
                result_json = "{\"result\":{\"type\":\"number\",\"value\":42}}";
            }
        } else if (std.mem.eql(u8, method, "DOM.getDocument")) {
            result_json = "{\"root\":{\"nodeId\":1}}";
        } else if (std.mem.eql(u8, method, "DOM.querySelector")) {
            if (std.mem.indexOf(u8, payload, ".missing") != null) {
                result_json = "{\"nodeId\":0}";
            } else {
                result_json = "{\"nodeId\":77}";
            }
        } else if (std.mem.eql(u8, method, "DOM.querySelectorAll")) {
            result_json = "{\"nodeIds\":[77,78]}";
        } else if (std.mem.eql(u8, method, "Page.captureScreenshot")) {
            result_json = "{\"data\":\"iVBORw0KGgoAAAANSUhEUg==\"}";
        } else if (std.mem.eql(u8, method, "Page.printToPDF")) {
            result_json = "{\"data\":\"JVBERi0xLjQK\"}";
        } else if (std.mem.eql(u8, method, "Accessibility.getFullAXTree")) {
            result_json = "{\"nodes\":[{\"nodeId\":\"ax1\",\"ignored\":false,\"role\":{\"value\":\"RootWebArea\"},\"name\":{\"value\":\"Example\"},\"backendDOMNodeId\":1},{\"nodeId\":\"ax2\",\"ignored\":true,\"role\":{\"value\":\"generic\"},\"name\":{\"value\":\"\"},\"backendDOMNodeId\":2},{\"nodeId\":\"ax3\",\"ignored\":false,\"role\":{\"value\":\"button\"},\"name\":{\"value\":\"Submit\"},\"backendDOMNodeId\":77}]}";
        } else if (std.mem.eql(u8, method, "Tracing.end")) {
            result_json = "{\"data\":\"[{\\\"cat\\\":\\\"devtools.timeline\\\",\\\"name\\\":\\\"RunTask\\\"}]\"}";
        } else if (std.mem.eql(u8, method, "Profiler.takePreciseCoverage")) {
            result_json = "{\"url\":\"https://example.com/app.js\",\"result\":[{\"scriptId\":\"1\",\"functions\":[{\"ranges\":[{\"startOffset\":0,\"endOffset\":100,\"count\":1},{\"startOffset\":20,\"endOffset\":50,\"count\":0}]}]}]}";
        } else if (std.mem.eql(u8, method, "CSS.stopRuleUsageTracking")) {
            result_json = "{\"url\":\"https://example.com/style.css\",\"ruleUsage\":[{\"styleSheetId\":\"ss-1\",\"startOffset\":0,\"endOffset\":40,\"used\":true},{\"styleSheetId\":\"ss-1\",\"startOffset\":40,\"endOffset\":80,\"used\":false}]}";
        }

        const resp = if (sid.len > 0)
            std.fmt.bufPrint(&rbuf, "{{\"id\":{s},\"sessionId\":\"{s}\",\"result\":{s}}}", .{ id_str, sid, result_json }) catch return false
        else
            std.fmt.bufPrint(&rbuf, "{{\"id\":{s},\"result\":{s}}}", .{ id_str, result_json }) catch return false;

        _.browser.feedIncoming(resp) catch return false;
        return true;
    }
}

fn mockTransportFn(ctx: usize, payload: []const u8): bool {
    const srv = unsafe { ctx as *MockCdpServer! };
    return srv.handleSend(payload);
}

struct EventRecorder {
    count: usize = 0,
    last_method_buf: [64]u8 = undefined,
    last_method_len: usize = 0,

    fn push(*_, ev: CdpEvent) {
        _.count += 1;
        const n = if (ev.method.len <= 64) ev.method.len else 64;
        if (n > 0) @memcpy(_.last_method_buf[0..n], ev.method[0..n]);
        _.last_method_len = n;
    }
}

fn onEventRecorder(ctx: usize, ev: CdpEvent) {
    const rec = unsafe { ctx as *EventRecorder! };
    rec.push(ev);
}

fn testLaunchArgsAndExecutableProbing(): !void {
    const alloc = std.heap.page_allocator;
    const args = Puppeteer.buildLaunchArgs(alloc, LaunchOptions{
        headless: true,
        viewport_width: 1440,
        viewport_height: 900,
        remote_debugging_port: 9222,
        ignore_https_errors: true,
        user_data_dir: "/tmp/cp-puppeteer-profile",
    })?;
    defer Puppeteer.freeLaunchArgs(alloc, args);

    let found_headless = false;
    let found_port = false;
    let found_win = false;
    let found_ignore = false;
    for (a of args) {
        if (std.mem.eql(u8, a, "--headless=new")) found_headless = true;
        if (std.mem.eql(u8, a, "--remote-debugging-port=9222")) found_port = true;
        if (std.mem.eql(u8, a, "--window-size=1440,900")) found_win = true;
        if (std.mem.eql(u8, a, "--ignore-certificate-errors")) found_ignore = true;
    }
    expect_eq(found_headless, true);
    expect_eq(found_port, true);
    expect_eq(found_win, true);
    expect_eq(found_ignore, true);

    const parsed = Puppeteer.parseDevToolsWsUrl("DevTools listening on ws://127.0.0.1:9222/devtools/browser/abc-123\n");
    expect_eq(parsed != null, true);
    if (ep := parsed) {
        expect_eq(ep.port, @as(u16, 9222));
        expect_eq(std.mem.eql(u8, ep.host, "127.0.0.1"), true);
    }

    if (exe := defaultExecutablePath(alloc)) {
        alloc.free(exe);
    }
}

fn testConnectAndBrowserMetadata(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = connectWithTransport(
        alloc,
        ConnectOptions{ browser_ws_url: "ws://127.0.0.1:9222/devtools/browser/test" },
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    expect_eq(browser.isConnected(), true);
    expect_eq(std.mem.eql(u8, browser.wsEndpoint(), "ws://127.0.0.1:9222/devtools/browser/test"), true);

    let vbuf: [128]u8 = undefined;
    const ver = browser.version(&vbuf)?;
    expect_eq(std.mem.eql(u8, ver, "Chrome/131.0.6778.85"), true);

    let uabuf: [128]u8 = undefined;
    const ua = browser.userAgent(&uabuf)?;
    expect_eq(std.mem.indexOf(u8, ua, "HeadlessChrome") != null, true);
}

fn testBrowserContextsAndPermissions(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{ headless: true },
        "ws://127.0.0.1:9222/devtools/browser/ctx",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const def_ctx = browser.defaultBrowserContext();
    expect_eq(def_ctx.isIncognito(), false);
    expect_eq(std.mem.eql(u8, def_ctx.id(), "default"), true);

    let incog = browser.createBrowserContext()?;
    expect_eq(incog.isIncognito(), true);
    expect_eq(std.mem.eql(u8, incog.id(), "ctx-incognito-1"), true);
    expect_eq(browser.browserContexts().len, @as(usize, 2));

    incog.overridePermissions("https://example.com", "[\"geolocation\"]")?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Browser.grantPermissions"), true);
    incog.close()?;
    expect_eq(incog.isClosed(), true);
}

fn testPageNavigationTitleContentAndEvaluate(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/nav",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    expect_eq(browser.pages().len, @as(usize, 1));
    expect_eq(std.mem.eql(u8, page.url(), "about:blank"), true);

    page.goto("https://example.com/dashboard", GoToOptions{})?;
    expect_eq(std.mem.eql(u8, page.url(), "https://example.com/dashboard"), true);

    let tbuf: [128]u8 = undefined;
    const title = page.title(&tbuf)?;
    expect_eq(std.mem.eql(u8, title, "Example Domain"), true);

    let cbuf: [256]u8 = undefined;
    const html = page.content(&cbuf)?;
    expect_eq(std.mem.indexOf(u8, html, "<h1>Hello</h1>") != null, true);

    let ebuf: [128]u8 = undefined;
    const eval_res = page.evaluate("21 * 2", &ebuf)?;
    expect_eq(eval_res.has_exception, false);
    expect_eq(std.mem.eql(u8, eval_res.value_json, "42"), true);

    page.evaluateOnNewDocument("window.__cp = true;")?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Page.addScriptToEvaluateOnNewDocument"), true);
}

fn testSelectorsAndElementHandleInteractions(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/dom",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    let missing_rejected = false;
    const _miss = page.querySelector(".missing") catch |e| blk: {
        if (e == PuppeteerError.ElementNotFound) missing_rejected = true;
        break :blk ElementHandle{};
    };
    * = _miss;
    expect_eq(missing_rejected, true);

    let el = page.waitForSelector("#submit-btn", WaitForSelectorOptions{})?;
    expect_eq(el.nodeId(), @as(i64, 77));
    const bb = el.boundingBox();
    expect_eq(bb != null, true);
    const bm = el.boxModel();
    expect_eq(bm != null, true);
    el.hover()?;
    el.click(ClickOptions{})?;
    el.typeText("hi", TypeOptions{})?;
    el.dispose()?;
    expect_eq(el.isDisposed(), true);

    let list_buf: [8]ElementHandle = undefined;
    const count = page.querySelectorAll("button", &list_buf)?;
    expect_eq(count >= 1, true);
}

fn testKeyboardMouseAndTouchscreen(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/input",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    const kb = page.keyboard();
    kb.down("Shift")?;
    kb.press("A", TypeOptions{})?;
    kb.up("Shift")?;
    kb.typeText("cp", TypeOptions{})?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Input.dispatchKeyEvent"), true);

    const mouse = page.mouse();
    mouse.move(100.0, 200.0)?;
    mouse.click(100.0, 200.0, ClickOptions{})?;
    mouse.dblclick(120.0, 220.0)?;
    mouse.wheel(0.0, 120.0)?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Input.dispatchMouseEvent"), true);

    const touch = page.touchscreen();
    touch.tap(50.0, 60.0)?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Input.dispatchTouchEvent"), true);
}

fn testViewportScreenshotAndPdf(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/media",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    page.setViewport(Viewport{ width: 1920, height: 1080, device_scale_factor: 2.0, has_touch: true })?;
    expect_eq(page.viewport().width, @as(u32, 1920));

    let img_buf: [256]u8 = undefined;
    const png_b64 = page.screenshot(ScreenshotOptions{ format: "png", full_page: true }, &img_buf)?;
    expect_eq(std.mem.eql(u8, png_b64, "iVBORw0KGgoAAAANSUhEUg=="), true);

    let pdf_buf: [256]u8 = undefined;
    const pdf_b64 = page.pdf(PdfOptions{ landscape: true }, &pdf_buf)?;
    expect_eq(std.mem.eql(u8, pdf_b64, "JVBERi0xLjQK"), true);
}

fn testNetworkInterceptionResponseAndDialog(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/net",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    const hdrs: [1]HttpHeader = .{
        HttpHeader{ name: "X-Custom", value: "cp-puppeteer" },
    };
    page.setExtraHttpHeaders(&hdrs)?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Network.setExtraHTTPHeaders"), true);

    page.setRequestInterception(true)?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Fetch.enable"), true);

    let req1 = HTTPRequest.init(&page.session, "req-1", "https://api.example.com/v1", "GET", ResourceType.XHR, false, true);
    req1.respond(200, "application/json", "{\"ok\":true}")?;
    expect_eq(req1.isInterceptResolutionHandled(), true);
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Fetch.fulfillRequest"), true);

    let req2 = HTTPRequest.init(&page.session, "req-2", "https://ads.example.com", "GET", ResourceType.Script, false, true);
    req2.abort("BlockedByClient")?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Fetch.failRequest"), true);

    const resp = HTTPResponse.init("req-1", "https://api.example.com/v1", 200, "OK", "{\"ok\":true}");
    expect_eq(resp.ok(), true);
    expect_eq(resp.status(), @as(u16, 200));
    expect_eq(std.mem.eql(u8, resp.text(), "{\"ok\":true}"), true);

    let dlg = Dialog.init(&page.session, DialogType.Prompt, "Enter name:", "CP");
    expect_eq(dlg.kind() == DialogType.Prompt, true);
    dlg.accept("Antigravity")?;
    expect_eq(dlg.handled, true);
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Page.handleJavaScriptDialog"), true);
}

fn testCookiesAndAccessibilitySnapshot(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/ax",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    const cparams: [1]CookieParam = .{
        CookieParam{ name: "sid", value: "xyz123", domain: "example.com", path: "/", http_only: true, secure: true },
    };
    page.setCookie(&cparams)?;
    expect_eq(page.cookies().len, @as(usize, 1));
    expect_eq(std.mem.eql(u8, page.cookies()[0].name, "sid"), true);

    page.deleteCookie("sid", "example.com")?;
    expect_eq(page.cookies().len, @as(usize, 0));

    const snap = page.accessibilitySnapshot(AccessibilitySnapshotOptions{ interesting_only: true })?;
    const nodes = snap.slice();
    expect_eq(nodes.len, @as(usize, 2));
    expect_eq(std.mem.eql(u8, nodes[0].role, "RootWebArea"), true);
    expect_eq(std.mem.eql(u8, nodes[1].role, "button"), true);
    expect_eq(std.mem.eql(u8, nodes[1].name, "Submit"), true);
}

fn testCdpSessionIsolationEventsAndIdempotentClose(): !void {
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/cdp",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    let cdp_sess = page.createCDPSession("sess-child-1")?;
    let rec = EventRecorder{};
    cdp_sess.on("Page.loadEventFired", onEventRecorder, unsafe { @intFromPtr(&rec) })?;

    browser.feedIncoming("{\"method\":\"Page.loadEventFired\",\"params\":{\"timestamp\":123.4},\"sessionId\":\"sess-child-1\"}")?;
    browser.feedIncoming("{\"method\":\"Page.loadEventFired\",\"params\":{\"timestamp\":999.9},\"sessionId\":\"other-sess\"}")?;
    expect_eq(rec.count, @as(usize, 1));

    cdp_sess.detach()?;
    expect_eq(cdp_sess.detached(), true);

    // Idempotent page & browser close/deinit
    page.close()?;
    page.close()?;
    expect_eq(page.isClosed(), true);

    browser.close()?;
    browser.close()?;
    browser.deinit();
    expect_eq(browser.isConnected(), false);
}

struct PageEventCounter {
    count: usize = 0,
    wild_count: usize = 0,
}

fn onPageTestEvent(event_name: []const u8, payload: []const u8, ctx: usize) {
    const _ign = payload;
    * = _ign;
    let rec: *PageEventCounter = unsafe { @ptrFromInt(ctx) };
    if (std.mem.eql(u8, event_name, "console")) {
        rec.count += 1;
    }
    rec.wild_count += 1;
}

fn testKnownDevicesAndEmulate(): !void {
    expect_eq(KnownDevices.count(), @as(usize, 155));
    expect_eq(KnownDevices.get("Blackberry PlayBook") != null, true);
    expect_eq(KnownDevices.get("BlackBerry Z30") != null, true);
    expect_eq(KnownDevices.get("Kindle Fire HDX") != null, true);
    expect_eq(KnownDevices.get("Nexus 6P") != null, true);
    expect_eq(KnownDevices.get("Moto G4") != null, true);
    expect_eq(KnownDevices.get("Galaxy S9+") != null, true);
    expect_eq(KnownDevices.get("iPhone 17 Pro Max") != null, true);

    const iphone15 = KnownDevices.get("iPhone 15 Pro") orelse unreachable;
    expect_eq(iphone15.viewport.width, @as(u32, 393));
    expect_eq(iphone15.viewport.height, @as(u32, 659));
    expect_eq(iphone15.viewport.is_mobile, true);
    expect_eq(iphone15.viewport.has_touch, true);

    const iphone17 = KnownDevices.iPhone17Pro();
    expect_eq(iphone17.viewport.width, @as(u32, 402));
    const desktop = KnownDevices.desktop1080p();
    expect_eq(desktop.viewport.width, @as(u32, 1920));
    expect_eq(desktop.viewport.is_mobile, false);

    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/dev",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    page.emulate(iphone15)?;
    expect_eq(page.viewport().width, @as(u32, 393));
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Emulation.setUserAgentOverride"), true);
}

fn testUSKeyboardLayoutAndInput(): !void {
    const k_a = USKeyboardLayout.get("a") orelse unreachable;
    expect_eq(k_a.key_code, @as(u16, 65));
    expect_eq(std.mem.eql(u8, k_a.code, "KeyA"), true);

    const k_enter = USKeyboardLayout.get("NumpadEnter") orelse unreachable;
    expect_eq(k_enter.key_code, @as(u16, 13));
    expect_eq(k_enter.location, @as(u8, 3));

    const k_arrow = USKeyboardLayout.get("ArrowLeft") orelse unreachable;
    expect_eq(k_arrow.key_code, @as(u16, 37));

    const shifted = USKeyboardLayout.resolve("KeyA", true);
    expect_eq(std.mem.eql(u8, shifted.key, "A"), true);
    expect_eq(std.mem.eql(u8, shifted.text, "A"), true);

    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/kb",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;
    let kb = page.keyboard();
    kb.down("Shift")?;
    expect_eq(kb.modifiers, @as(i32, 8));
    kb.press("a", TypeOptions{})?;
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Input.dispatchKeyEvent"), true);
    kb.up("Shift")?;
    expect_eq(kb.modifiers, @as(i32, 0));
}

fn testPSelectorsSimdAndWaitTask(): !void {
    expect_eq(SimdSelectorScanner.needsPSelectorParsing("div.container > span#title"), false);
    expect_eq(SimdSelectorScanner.needsPSelectorParsing("button::-p-text(Submit)"), true);
    expect_eq(SimdSelectorScanner.needsPSelectorParsing("div >>> .inner"), true);
    expect_eq(SimdSelectorScanner.indexOfByteSimd("0123456789abcdef:target", ':') orelse 0, @as(usize, 16));

    const pure = PSelectorParser.parse("div.main > ul li.active");
    expect_eq(pure.is_pure_css, true);
    expect_eq(pure.count, @as(usize, 1));

    const p_text = PSelectorParser.parse("button::-p-text('Click Me')");
    expect_eq(p_text.is_pure_css, false);
    expect_eq(p_text.count, @as(usize, 2));
    expect_eq(p_text.segments[0].kind == PSelectorKind.Css, true);
    expect_eq(p_text.segments[1].kind == PSelectorKind.Text, true);
    expect_eq(std.mem.eql(u8, p_text.segments[1].value, "Click Me"), true);

    const p_aria = PSelectorParser.parse("::-p-aria(Submit[role=\"button\"])");
    expect_eq(p_aria.has_aria, true);
    expect_eq(p_aria.segments[0].kind == PSelectorKind.Aria, true);

    const p_pierce = PSelectorParser.parse("my-app >>>> .item ::-p-xpath(.//span)");
    expect_eq(p_pierce.has_pierce, true);
    expect_eq(p_pierce.segments[1].combinator == PCombinator.Child, true);
    expect_eq(p_pierce.segments[3].kind == PSelectorKind.XPath, true);

    let wt = WaitTask.init("document.readyState === 'complete'", WaitForFunctionOptions{ polling: WaitTaskPolling.Interval, polling_ms: 50, timeout_ms: 5000 });
    let wbuf: [4096]u8 = undefined;
    const expr = wt.buildPollerExpression(&wbuf)?;
    expect_eq(std.mem.indexOf(u8, expr, "pollInterval") != null, true);
}

fn testEventsAndExtras(): !void {
    let emitter = PuppeteerEventEmitter.init();
    let rec = PageEventCounter{};
    let rec_ptr = unsafe { @intFromPtr(&rec) };
    expect_eq(emitter.on("console", onPageTestEvent, rec_ptr), true);
    expect_eq(emitter.once("*", onPageTestEvent, rec_ptr), true);
    expect_eq(emitter.listenerCount(""), @as(usize, 2));

    expect_eq(emitter.emit("console", "hello"), true);
    expect_eq(rec.count, @as(usize, 2));
    expect_eq(rec.wild_count, @as(usize, 2));

    // Second emit: wildcard 'once' should not fire again
    expect_eq(emitter.emit("console", "world"), true);
    expect_eq(rec.count, @as(usize, 3));
    expect_eq(rec.wild_count, @as(usize, 3));

    emitter.removeAllListeners("console");
    expect_eq(emitter.listenerCount("console"), @as(usize, 0));

    // ConsoleMessage, SecurityDetails & TimeoutSettings
    const cmsg = ConsoleMessage.fromCdpPayload("{\"type\":\"warning\",\"text\":\"deprecated API\",\"url\":\"https://example.com/app.js\"}");
    expect_eq(cmsg.kind() == ConsoleMessageType.Warn, true);
    expect_eq(std.mem.eql(u8, cmsg.text(), "deprecated API"), true);

    const sec = SecurityDetails.fromCdpPayload("{\"subjectName\":\"example.com\",\"issuer\":\"DigiCert\",\"validFrom\":1700000000,\"validTo\":1730000000,\"protocol\":\"TLS 1.3\"}");
    expect_eq(std.mem.eql(u8, sec.subjectName(), "example.com"), true);
    expect_eq(std.mem.eql(u8, sec.protocol(), "TLS 1.3"), true);

    let ts = TimeoutSettings.init();
    expect_eq(ts.timeout(), @as(u32, 30000));
    expect_eq(ts.navigationTimeout(), @as(u32, 30000));
    ts.setDefaultTimeout(15000);
    expect_eq(ts.timeout(), @as(u32, 15000));
    expect_eq(ts.navigationTimeout(), @as(u32, 15000));
    ts.setDefaultNavigationTimeout(45000);
    expect_eq(ts.navigationTimeout(), @as(u32, 45000));

    // FileChooser single-use guard
    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/fc",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;
    const page = browser.newPage()?;
    const el = page.querySelector("#upload")?;
    let fc = FileChooser.init(el, true);
    expect_eq(fc.isMultiple(), true);
    const files: [1][]const u8 = .{"/tmp/demo.txt"};
    fc.accept(&files)?;
    expect_eq(fc.isHandled(), true);
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "DOM.setFileInputFiles"), true);
}

fn testTracingAndCoverage(): !void {
    // 1. Disjoint ranges scanline algorithm
    const starts: [3]usize = [0, 20, 60];
    const ends: [3]usize = [100, 50, 80];
    const counts: [3]usize = [1, 0, 2];
    let disjoint: [8]CoverageRange = undefined;
    const dlen = convertToDisjointRanges(&starts, &ends, &counts, &disjoint);
    expect_eq(dlen, @as(usize, 2));
    expect_eq(disjoint[0].start, @as(usize, 0));
    expect_eq(disjoint[0].end, @as(usize, 20));
    expect_eq(disjoint[1].start, @as(usize, 50));
    expect_eq(disjoint[1].end, @as(usize, 100));

    const alloc = std.heap.page_allocator;
    let srv = MockCdpServer{};
    let browser = launchWithTransport(
        alloc,
        LaunchOptions{},
        "ws://127.0.0.1:9222/devtools/browser/tc",
        mockTransportFn,
        unsafe { @intFromPtr(&srv) },
    )?;
    defer browser.deinit();
    srv.browser = &browser;
    srv.has_browser = true;

    const page = browser.newPage()?;

    // 2. Page.tracing().start / stop
    const tr = page.tracing();
    tr.start(TracingOptions{ screenshots: true, buffer_size: 4096 })?;
    expect_eq(tr.isRecording(), true);
    expect_eq(std.mem.eql(u8, srv.lastMethod(), "Tracing.start"), true);
    let tbuf: [1024]u8 = undefined;
    const trace_out = tr.stop(&tbuf)?;
    expect_eq(tr.isRecording(), false);
    expect_eq(std.mem.indexOf(u8, trace_out, "devtools.timeline") != null, true);

    // 3. Page.coverage().startJSCoverage / stopJSCoverage
    const cov = page.coverage();
    cov.startJSCoverage(JSCoverageOptions{})?;
    const js_report = cov.stopJSCoverage()?;
    const js_entries = js_report.slice();
    expect_eq(js_entries.len, @as(usize, 1));
    expect_eq(std.mem.eql(u8, js_entries[0].url(), "https://example.com/app.js"), true);
    const js_ranges = js_entries[0].rangeSlice();
    expect_eq(js_ranges.len, @as(usize, 2));
    expect_eq(js_ranges[0].start, @as(usize, 0));
    expect_eq(js_ranges[0].end, @as(usize, 20));
    expect_eq(js_ranges[1].start, @as(usize, 50));
    expect_eq(js_ranges[1].end, @as(usize, 100));

    // 4. Page.coverage().startCSSCoverage / stopCSSCoverage
    cov.startCSSCoverage(CSSCoverageOptions{})?;
    const css_report = cov.stopCSSCoverage()?;
    const css_entries = css_report.slice();
    expect_eq(css_entries.len, @as(usize, 1));
    expect_eq(std.mem.eql(u8, css_entries[0].url(), "https://example.com/style.css"), true);
    const css_ranges = css_entries[0].rangeSlice();
    expect_eq(css_ranges.len, @as(usize, 1));
    expect_eq(css_ranges[0].start, @as(usize, 0));
    expect_eq(css_ranges[0].end, @as(usize, 40));
}

pub fn main() {
    test("1. Launch Args Builder, Executable Probing & WsUrl Parser", () => {
        testLaunchArgsAndExecutableProbing() catch unreachable;
    });
    test("2. ConnectWithTransport & Browser Metadata (version / userAgent)", () => {
        testConnectAndBrowserMetadata() catch unreachable;
    });
    test("3. BrowserContext Isolation & Permissions", () => {
        testBrowserContextsAndPermissions() catch unreachable;
    });
    test("4. Page Navigation, Title, Content & Runtime Evaluate", () => {
        testPageNavigationTitleContentAndEvaluate() catch unreachable;
    });
    test("5. Selectors (querySelector / querySelectorAll) & ElementHandle", () => {
        testSelectorsAndElementHandleInteractions() catch unreachable;
    });
    test("6. Keyboard, Mouse & Touchscreen Input Controllers", () => {
        testKeyboardMouseAndTouchscreen() catch unreachable;
    });
    test("7. Viewport, Screenshot & PDF Generation", () => {
        testViewportScreenshotAndPdf() catch unreachable;
    });
    test("8. Network Interception, HTTPResponse & Dialog Handling", () => {
        testNetworkInterceptionResponseAndDialog() catch unreachable;
    });
    test("9. Cookies CRUD & Accessibility Snapshot (cap:cdp AXTree)", () => {
        testCookiesAndAccessibilitySnapshot() catch unreachable;
    });
    test("10. CDPSession Multi-Target Isolation, Events & Idempotent Close", () => {
        testCdpSessionIsolationEventsAndIdempotentClose() catch unreachable;
    });
    test("11. KnownDevices Dictionary & Page.emulate()", () => {
        testKnownDevicesAndEmulate() catch unreachable;
    });
    test("12. USKeyboardLayout Table & Keyboard Precision", () => {
        testUSKeyboardLayoutAndInput() catch unreachable;
    });
    test("13. PSelectorParser, @Vector(16, u8) SIMD & WaitTask", () => {
        testPSelectorsSimdAndWaitTask() catch unreachable;
    });
    test("14. PuppeteerEventEmitter, FileChooser, ConsoleMessage, SecurityDetails & TimeoutSettings", () => {
        testEventsAndExtras() catch unreachable;
    });
    test("15. Tracing, Coverage (JS/CSS) & Disjoint Range Scanline", () => {
        testTracingAndCoverage() catch unreachable;
    });
}
