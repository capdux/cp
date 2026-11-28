// Source: cp/cap/test/testCapPlaywright.cp
// Comprehensive Verification Suite for packages/playwright (cap:cdp backed)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { CdpFrameScanner, CdpBrowserLauncher } from "cap:cdp";
import {
    PlaywrightError,
    RouteAction,
    NameValue,
    ResourceTiming,
    RequestSizes,
    RequestFailure,
    SecurityDetails,
    RemoteAddr,
    FulfillOptions,
    RouteContinueOverrides,
    ScriptTagOptions,
    StyleTagOptions,
    WorkerInfo,
    Request,
    Response,
    Route,
    UrlMatcher,
    AriaRoleNode,
    AriaSnapshot,
    WaitForState,
    WaitForOptions,
    ByRoleOptions,
    LocatorOptions,
    SelectorCompiler,
    Locator,
    GotoOptions,
    ScreenshotOptions,
    PdfOptions,
    ViewportSize,
    EmulateMediaOptions,
    Page,
    Cookie,
    Geolocation,
    HTTPCredentials,
    CDPSession,
    StorageState,
    BrowserContextOptions,
    BrowserContext,
    Browser,
    LaunchOptions,
    ConnectOverCdpOptions,
    chromium,
    WebKitLaunchOptions,
    WkMessageKind,
    WkPageProxySession,
    WkConnection,
    WebKitPage,
    WebKitBrowser,
    WebKitBrowserType,
    webkit,
    ActionabilityState,
    ActionabilityCheckKind,
    ActionabilityOptions,
    ActionabilityEngine,
    ClockMode,
    Clock,
    Download,
    FileChooser,
    ConsoleMessage,
    HarRecordOptions,
    HarEntry,
    HarRecorder,
    simdScanHarCleanSpan,
    RemoteMode,
    PlaywrightRemoteConfig,
    PlaywrightRemoteServer,
    PlaywrightRemoteClient,
    CliCommand,
    PlaywrightCli,
    Frame,
    Tracing,
    TraceViewerOptions,
    TraceViewerServer,
    Coverage,
    APIRequestContext,
    WebSocketRoute,
    Video,
} from "../../packages/playwright/src/index.cp";

struct MockCdpTransport {
    sent_count: usize = 0,
    last_method: [128]u8 = undefined,
    last_method_len: usize = 0,
    last_payload: [2048]u8 = undefined,
    last_payload_len: usize = 0,

    fn record(*_, payload: []const u8): bool {
        _.sent_count += 1;
        const n = if (payload.len <= 2048) payload.len else 2048;
        @memcpy(_.last_payload[0..n], payload[0..n]);
        _.last_payload_len = n;
        if (m := CdpFrameScanner.extractTopLevelField(payload, "method", true)) {
            const mn = if (m.len <= 128) m.len else 128;
            @memcpy(_.last_method[0..mn], m[0..mn]);
            _.last_method_len = mn;
        }
        return true;
    }

    fn lastMethod(): []const u8 {
        return _.last_method[0.._.last_method_len];
    }

    fn lastPayload(): []const u8 {
        return _.last_payload[0.._.last_payload_len];
    }
}

fn mockTransportSend(ctx: usize, payload: []const u8): bool {
    const mt = unsafe { ctx as *MockCdpTransport! };
    return mt.record(payload);
}

struct RouteTestState {
    intercepted_count: usize = 0,
    mode: u8 = 0,
}

fn testRouteHandler(ctx: usize, r: *Route) {
    const st = unsafe { ctx as *RouteTestState! };
    st.intercepted_count += 1;
    if (st.mode == 0) {
        r.fulfill(.{
            status: 200,
            content_type: "application/json",
            body: "{\"ok\":true}",
        }) catch {};
    } else if (st.mode == 1) {
        r.abort("blockedbyclient") catch {};
    } else if (st.mode == 2) {
        r.continue_("https://example.org/override", "POST") catch {};
    } else {
        r.fallback() catch {};
    }
}

fn testChromiumLaunchAndLauncherProbe(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    expect_eq(std.mem.eql(u8, chromium.name(), "chromium"), true);

    // BrowserLauncher probing via cap:cdp (may return null in container without browser, or path if present)
    if (exe := chromium.executablePath(alloc, "/nonexistent/browser/bin")) {
        alloc.free(exe);
    }

    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        headless: true,
        viewport_width: 1440,
        viewport_height: 900,
        ignore_https_errors: true,
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;

    expect_eq(browser.isConnected(), true);
    expect_eq(browser.launch_args.len >= 15, true);
    expect_eq(std.mem.eql(u8, browser.version(), "130.0.6723.0"), true);

    // Verify CdpBrowserLauncher flag generation
    let found_headless = false;
    let found_viewport = false;
    let found_ignore_https = false;
    for (arg of browser.launch_args) {
        if (std.mem.eql(u8, arg, "--headless=new")) found_headless = true;
        if (std.mem.eql(u8, arg, "--window-size=1440,900")) found_viewport = true;
        if (std.mem.eql(u8, arg, "--ignore-certificate-errors")) found_ignore_https = true;
    }
    expect_eq(found_headless, true);
    expect_eq(found_viewport, true);
    expect_eq(found_ignore_https, true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testConnectOverCDP(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    // Invalid URL rejected
    let rejected = false;
    if (b := chromium.connectOverCDP(alloc, "http://127.0.0.1:9222", .{}) catch null) {
        let mut_b = b;
        mut_b.deinit();
    } else {
        rejected = true;
    }
    expect_eq(rejected, true);

    // Valid ws:// URL accepted
    let transport = MockCdpTransport{};
    let browser = chromium.connectOverCDP(
        alloc,
        "ws://127.0.0.1:9222/devtools/browser/abc-123",
        .{
            transport_fn: mockTransportSend,
            transport_ctx: unsafe { @intFromPtr(&transport) },
        },
    )?;

    expect_eq(browser.isConnected(), true);
    expect_eq(std.mem.eql(u8, browser.ws_endpoint, "ws://127.0.0.1:9222/devtools/browser/abc-123"), true);

    // Also verify DevTools stderr parser from cap:cdp
    const parsed = CdpBrowserLauncher.parseDevToolsWsUrl("DevTools listening on ws://127.0.0.1:9333/devtools/browser/xyz");
    expect_eq(parsed != null, true);

    browser.close();
    expect_eq(browser.isConnected(), false);
    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testBrowserContextCookiesAndStorageState(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;

    const ctx = browser.newContext(.{
        viewport_width: 1280,
        viewport_height: 720,
        locale: "en-US",
    })?;
    expect_eq(browser.contexts().len, 1);

    const p1 = ctx.newPage()?;
    const p2 = ctx.newPage()?;
    expect_eq(ctx.pages().len, 2);
    expect_eq(p1.isClosed(), false);
    expect_eq(p2.isClosed(), false);

    const sample_cookies: [2]Cookie = .{
        Cookie{ name: "session", value: "xyz123", domain: "example.com", path: "/", http_only: true },
        Cookie{ name: "theme", value: "dark", domain: "example.com", path: "/", secure: true },
    };
    ctx.addCookies(&sample_cookies)?;
    expect_eq(ctx.cookies().len, 2);
    expect_eq(std.mem.eql(u8, ctx.cookies()[0].name, "session"), true);
    expect_eq(std.mem.eql(u8, ctx.cookies()[1].value, "dark"), true);

    let state = ctx.storageState(alloc)?;
    expect_eq(state.cookies.items.len, 2);
    const json_out = state.toJson(alloc)?;
    expect_eq(std.mem.indexOf(u8, json_out, "\"session\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json_out, "\"theme\"") != null, true);
    alloc.free(json_out);
    state.deinit();

    ctx.clearCookies();
    expect_eq(ctx.cookies().len, 0);

    ctx.close();
    expect_eq(p1.isClosed(), true);
    expect_eq(p2.isClosed(), true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testPageNavigationContentAndCdpDomains(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;

    const page = browser.newPage()?;
    expect_eq(std.mem.eql(u8, page.url(), "about:blank"), true);

    const resp1 = page.goto("https://example.com/login", .{})?;
    expect_eq(resp1.isOk(), true);
    expect_eq(std.mem.eql(u8, page.url(), "https://example.com/login"), true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.navigate"), true);

    const resp2 = page.goto("https://example.com/dashboard", .{})?;
    expect_eq(resp2.status, 200);
    expect_eq(std.mem.eql(u8, page.url(), "https://example.com/dashboard"), true);

    const back_resp = page.goBack()?;
    expect_eq(back_resp != null, true);
    expect_eq(std.mem.eql(u8, page.url(), "https://example.com/login"), true);

    const fwd_resp = page.goForward()?;
    expect_eq(fwd_resp != null, true);
    expect_eq(std.mem.eql(u8, page.url(), "https://example.com/dashboard"), true);

    * = page.reload()?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.reload"), true);

    page.setContent("<h1>Hello Playwright</h1>")?;
    expect_eq(std.mem.eql(u8, page.content(), "<h1>Hello Playwright</h1>"), true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.setDocumentContent"), true);

    * = page.evaluate("document.title")?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);

    * = page.screenshot(.{ format: "png", full_page: true })?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.captureScreenshot"), true);

    * = page.pdf(.{ landscape: true })?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.printToPDF"), true);

    page.close();
    expect_eq(page.isClosed(), true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testLocatorSelectorsChainingAndActions(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;
    const page = browser.newPage()?;

    // Semantic locator compilation
    const by_role = page.getByRole("button", .{ name: "Submit", exact: true });
    expect_eq(std.mem.eql(u8, by_role.selector(), "internal:role=button[name=\"Submit\"s]"), true);

    const by_text = page.getByText("Welcome", false);
    expect_eq(std.mem.eql(u8, by_text.selector(), "internal:text=\"Welcome\""), true);

    const by_label = page.getByLabel("Email", true);
    expect_eq(std.mem.eql(u8, by_label.selector(), "internal:label=\"Email\"s"), true);

    const by_ph = page.getByPlaceholder("Search...", false);
    expect_eq(std.mem.eql(u8, by_ph.selector(), "internal:attr=[placeholder*=\"Search...\"]"), true);

    const by_tid = page.getByTestId("submit-btn");
    expect_eq(std.mem.eql(u8, by_tid.selector(), "internal:testid=[data-testid=\"submit-btn\"]"), true);

    // Chaining: locator -> filter -> first -> nth -> last
    const chained = page.locator("form.login").locator("button").filter(.{ has_text: "Sign in" }).first();
    expect_eq(
        std.mem.eql(u8, chained.selector(), "form.login >> button >> internal:has-text=\"Sign in\" >> nth=0"),
        true,
    );
    const nth2 = page.locator("li.item").nth(2);
    expect_eq(std.mem.eql(u8, nth2.selector(), "li.item >> nth=2"), true);
    const last_item = page.locator("li.item").last();
    expect_eq(std.mem.eql(u8, last_item.selector(), "li.item >> nth=-1"), true);

    // Auto-waiting & CDP dispatch actions
    by_tid.click()?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);
    expect_eq(std.mem.indexOf(u8, transport.lastPayload(), "data-testid") != null, true);

    const email_input = page.locator("#email");
    email_input.fill("alice@example.com")?;
    email_input.press("Enter")?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Input.dispatchKeyEvent"), true);

    const agree = page.locator("#agree");
    agree.check()?;
    agree.uncheck()?;

    const country = page.locator("#country");
    country.selectOption("AU")?;

    const txt = email_input.textContent(alloc)?;
    alloc.free(txt);
    const itxt = email_input.innerText(alloc)?;
    alloc.free(itxt);
    const attr = email_input.getAttribute(alloc, "type")?;
    if (a := attr) {
        alloc.free(a);
    }

    expect_eq(email_input.isVisible()?, true);
    expect_eq(email_input.isEnabled()?, true);
    * = email_input.count()?;

    // Page shorthand actions
    page.click("#submit")?;
    page.fill("#username", "bob")?;
    page.typeText("#bio", "hello")?;
    page.press("#username", "Tab")?;
    * = page.waitForSelector("#done", .{ state: WaitForState.Visible })?;

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testNetworkRoutingAndUrlMatcher(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    expect_eq(UrlMatcher.matches("https://api.example.com/**", "https://api.example.com/v1/users"), true);
    expect_eq(UrlMatcher.matches("**/users/*.json", "https://api.example.com/users/42.json"), true);
    expect_eq(UrlMatcher.matches("**/users/*.json", "https://api.example.com/posts/42.xml"), false);

    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;
    const ctx = browser.newContext(.{})?;
    const page = ctx.newPage()?;

    let state = RouteTestState{};
    ctx.route("https://api.example.com/**", testRouteHandler, unsafe { @intFromPtr(&state) })?;
    page.route("**/local/**", testRouteHandler, unsafe { @intFromPtr(&state) })?;

    // Mode 0: fulfill
    let r1 = Route{ request: Request{ url: "https://api.example.com/v1/data", method: "GET" } };
    expect_eq(ctx.dispatchRoute(&r1), true);
    expect_eq(r1.action == RouteAction.Fulfilled, true);
    expect_eq(r1.fulfill_status, 200);
    expect_eq(std.mem.eql(u8, r1.fulfill_body, "{\"ok\":true}"), true);

    // Mode 1: abort
    state.mode = 1;
    let r2 = Route{ request: Request{ url: "https://example.com/local/script.js", method: "GET" } };
    expect_eq(page.dispatchRoute(&r2), true);
    expect_eq(r2.action == RouteAction.Aborted, true);
    expect_eq(std.mem.eql(u8, r2.abort_error_code, "blockedbyclient"), true);

    // Mode 2: continue_
    state.mode = 2;
    let r3 = Route{ request: Request{ url: "https://api.example.com/v1/submit", method: "GET" } };
    expect_eq(ctx.dispatchRoute(&r3), true);
    expect_eq(r3.action == RouteAction.Continued, true);
    expect_eq(std.mem.eql(u8, r3.override_method, "POST"), true);

    expect_eq(state.intercepted_count, 3);

    page.unroute("**/local/**");
    ctx.unroute("https://api.example.com/**");
    let r4 = Route{ request: Request{ url: "https://api.example.com/v1/data" } };
    expect_eq(ctx.dispatchRoute(&r4), false);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testAriaSnapshotAndCdpAXTree(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let snap = AriaSnapshot.init(alloc);
    snap.addFromCdpAxNodeJson("{\"nodeId\":\"1\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"heading\"},\"name\":{\"type\":\"computedString\",\"value\":\"Sign In\"},\"backendDOMNodeId\":101}")?;
    snap.addFromCdpAxNodeJson("{\"nodeId\":\"2\",\"ignored\":true,\"role\":{\"type\":\"role\",\"value\":\"generic\"}}")?;
    snap.addFromCdpAxNodeJson("{\"nodeId\":\"3\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"button\"},\"name\":{\"type\":\"computedString\",\"value\":\"Submit\"},\"backendDOMNodeId\":102}")?;

    expect_eq(snap.count(), 2);

    const btn = snap.findByRoleAndName("button", "Submit");
    expect_eq(btn != null, true);
    if (b := btn) {
        expect_eq(b.backend_dom_node_id, 102);
    }

    const yaml = snap.toYamlString(alloc)?;
    expect_eq(std.mem.indexOf(u8, yaml, "- heading \"Sign In\"") != null, true);
    expect_eq(std.mem.indexOf(u8, yaml, "- button \"Submit\"") != null, true);
    alloc.free(yaml);

    snap.deinit();

    // Also test Page.ariaSnapshot CDP dispatch
    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;
    const page = browser.newPage()?;
    let page_snap = page.ariaSnapshot(alloc)?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Accessibility.getFullAXTree"), true);
    page_snap.deinit();

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testActionabilityEngineAndSimdSelectors(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    // SIMD selector & URL wildcard checks
    expect_eq(Locator.hasComplexSelectorSimd("form.login >> button:first-child"), true);
    expect_eq(Locator.hasComplexSelectorSimd("button_submit_plain"), false);
    expect_eq(UrlMatcher.hasWildcardsSimd("https://example.com/api/**"), true);
    expect_eq(UrlMatcher.hasWildcardsSimd("https://example.com/static/app.js"), false);
    expect_eq(UrlMatcher.matches("https://example.com/static/app.js", "https://example.com/static/app.js"), true);

    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;
    const page = browser.newPage()?;
    const btn = page.locator("#submit-btn");

    // 1. Force mode bypasses CDP wait and succeeds immediately
    const force_rep = btn.checkActionability(&.{ ActionabilityCheckKind.Visible }, .{ force: true })?;
    expect_eq(force_rep.state == ActionabilityState.Done, true);
    expect_eq(force_rep.hit_target_ok, true);

    // 2. Trial mode runs actionability check via Runtime.evaluate without clicking
    const sent_before = transport.sent_count;
    const trial_rep = btn.clickWithOptions(.{ trial: true })?;
    expect_eq(trial_rep.state == ActionabilityState.Done, true);
    expect_eq(trial_rep.trial_only, true);
    expect_eq(transport.sent_count, sent_before + 1);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);

    // 3. Normal clickWithOptions runs check + click
    const full_rep = btn.clickWithOptions(.{})?;
    expect_eq(full_rep.state == ActionabilityState.Done, true);
    expect_eq(full_rep.scrolled, true);
    expect_eq(transport.sent_count, sent_before + 3);

    btn.scrollIntoViewIfNeeded()?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testClockTimeControl(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;
    const page = browser.newPage()?;
    let clk = page.clock();

    expect_eq(clk.mode == ClockMode.Real, true);
    clk.install(1700000000000)?;
    expect_eq(clk.installed, true);
    expect_eq(clk.mode == ClockMode.Installed, true);
    expect_eq(clk.now(), 1700000000000);

    clk.fastForward(5000)?;
    expect_eq(clk.now(), 1700000005000);

    clk.fastForwardStr("01:30")?;
    expect_eq(clk.now(), 1700000095000);

    clk.pauseAt(1700000100000)?;
    expect_eq(clk.mode == ClockMode.Paused, true);
    expect_eq(clk.now(), 1700000100000);

    clk.resume_()?;
    expect_eq(clk.mode == ClockMode.Installed, true);

    clk.setFixedTime(1800000000000)?;
    expect_eq(clk.mode == ClockMode.Fixed, true);
    expect_eq(clk.now(), 1800000000000);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);

    clk.uninstall();
    expect_eq(clk.mode == ClockMode.Real, true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testArtifactsDownloadFileChooserConsoleAndHar(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    // Download
    let dl = Download.init(alloc, "https://example.com/report.pdf", "dl-1", "report.pdf")?;
    expect_eq(std.mem.eql(u8, dl.suggestedFilename(), "report.pdf"), true);
    dl.saveAs("/tmp/report.pdf")?;
    expect_eq(dl.path() != null, true);
    dl.cancel();
    expect_eq(dl.path() == null, true);
    dl.deinit();

    // FileChooser
    let transport = MockCdpTransport{};
    let browser = chromium.launch(alloc, .{
        transport_fn: mockTransportSend,
        transport_ctx: unsafe { @intFromPtr(&transport) },
    })?;
    const page = browser.newPage()?;
    let fc = FileChooser.init(alloc, "input[type=file]", true, &browser.endpoint, page.session_id);
    expect_eq(fc.isMultiple(), true);
    const files: [2][]const u8 = .{ "/tmp/a.txt", "/tmp/b.txt" };
    fc.setFiles(&files)?;
    expect_eq(fc.files.items.len, 2);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "DOM.setFileInputFiles"), true);
    fc.deinit();

    // ConsoleMessage
    const msg = ConsoleMessage{
        msg_type: "warn",
        text: "Deprecated API",
        url: "https://example.com/app.js",
        line_number: 42,
    };
    expect_eq(std.mem.eql(u8, msg.type_(), "warn"), true);

    // SIMD HAR clean-span scan + HarRecorder with cap:json serialization
    expect_eq(simdScanHarCleanSpan("clean_har_header_value_1234567890") == null, true);
    expect_eq(simdScanHarCleanSpan("hello\"world") != null, true);

    let har = HarRecorder.init(alloc, HarRecordOptions{
        har_path: "out.har",
        minimal: false,
        url_glob: "https://api.example.com/**",
    });
    expect_eq(har.recordEntry(HarEntry{
        method: "GET",
        url: "https://api.example.com/v1/users",
        status: 200,
        mime_type: "application/json",
        response_body: "{\"users\":[]}",
        body_size: 12,
    })?, true);
    expect_eq(har.recordEntry(HarEntry{
        method: "GET",
        url: "https://cdn.example.com/logo.png",
        status: 200,
    })?, false);
    expect_eq(har.entryCount(), 1);

    const har_json = har.toJson(alloc)?;
    expect_eq(std.mem.indexOf(u8, har_json, "\"version\":\"1.2\"") != null, true);
    expect_eq(std.mem.indexOf(u8, har_json, "Playwright-CP") != null, true);
    expect_eq(std.mem.indexOf(u8, har_json, "https://api.example.com/v1/users") != null, true);
    alloc.free(har_json);
    har.deinit();

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testRemoteServerAndClientRpc(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let srv = PlaywrightRemoteServer.init(alloc, PlaywrightRemoteConfig{
        host: "127.0.0.1",
        port: 9455,
        path: "/pw",
        mode: RemoteMode.LaunchServer,
    });
    srv.setTransport(mockTransportSend, unsafe { @intFromPtr(&transport) });

    let ep_buf: [128]u8 = undefined;
    const ep = srv.wsEndpoint(&ep_buf)?;
    expect_eq(std.mem.eql(u8, ep, "ws://127.0.0.1:9455/pw"), true);

    let client = PlaywrightRemoteClient.connect(alloc, ep)?;
    expect_eq(client.connected, true);

    let rpc_buf: [256]u8 = undefined;
    const init_frame = client.buildRpcFrame("initialize", "", &rpc_buf)?;
    const init_res = srv.handleRpcMessage(alloc, init_frame)?;
    expect_eq(std.mem.indexOf(u8, init_res, "playwright-cp") != null, true);
    alloc.free(init_res);

    const launch_frame = client.buildRpcFrame("launch", "", &rpc_buf)?;
    const launch_res = srv.handleRpcMessage(alloc, launch_frame)?;
    expect_eq(std.mem.indexOf(u8, launch_res, "chromium-launched") != null, true);
    alloc.free(launch_res);

    const goto_frame = client.buildRpcFrame("goto", "https://example.com/rpc", &rpc_buf)?;
    const goto_res = srv.handleRpcMessage(alloc, goto_frame)?;
    expect_eq(std.mem.indexOf(u8, goto_res, "https://example.com/rpc") != null, true);
    alloc.free(goto_res);

    const close_frame = client.buildRpcFrame("close", "", &rpc_buf)?;
    const close_res = srv.handleRpcMessage(alloc, close_frame)?;
    expect_eq(std.mem.indexOf(u8, close_res, "closed") != null, true);
    alloc.free(close_res);

    client.deinit();
    srv.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testPlaywrightCliActions(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};

    // 1. screenshot action
    const ss_args: [4][]const u8 = .{ "screenshot", "https://example.com", "out.png", "--full-page" };
    const ss_opts = PlaywrightCli.parseArgs(&ss_args)?;
    expect_eq(ss_opts.command == CliCommand.Screenshot, true);
    expect_eq(ss_opts.full_page, true);
    let ss_res = PlaywrightCli.execute(alloc, ss_opts, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    expect_eq(ss_res.ok, true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.captureScreenshot"), true);
    ss_res.deinit();

    // 2. pdf action
    const pdf_args: [3][]const u8 = .{ "pdf", "https://example.com", "--landscape" };
    const pdf_opts = PlaywrightCli.parseArgs(&pdf_args)?;
    expect_eq(pdf_opts.command == CliCommand.Pdf, true);
    expect_eq(pdf_opts.landscape, true);
    let pdf_res = PlaywrightCli.execute(alloc, pdf_opts, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    expect_eq(pdf_res.ok, true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.printToPDF"), true);
    pdf_res.deinit();

    // 3. codegen action (CP & Python skeletons)
    const cg_args: [3][]const u8 = .{ "codegen", "https://example.com/login", "--target=cp" };
    const cg_opts = PlaywrightCli.parseArgs(&cg_args)?;
    let cg_res = PlaywrightCli.execute(alloc, cg_opts, null, 0)?;
    expect_eq(std.mem.indexOf(u8, cg_res.generated_code, "chromium.launch") != null, true);
    expect_eq(std.mem.indexOf(u8, cg_res.generated_code, "https://example.com/login") != null, true);
    cg_res.deinit();

    expect_eq(gpa.deinit() == .ok, true);
}

struct WsRouteTestState {
    handled: bool = false,
}

fn testWsRouteHandler(ctx: usize, ws: *WebSocketRoute) {
    const st = unsafe { ctx as *WsRouteTestState! };
    st.handled = true;
    ws.connectToServer();
    ws.send("hello-from-mock") catch {};
    ws.sendToServer("ping") catch {};
}

fn testFrameTreeManagement(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const page = browser.newPage()?;

    const mf = page.mainFrame()?;
    expect_eq(mf.parentFrame() == null, true);
    expect_eq(std.mem.eql(u8, mf.url(), "about:blank"), true);

    const child1 = mf.addChildFrame("frame-1", "sidebar", "https://example.com/sidebar")?;
    const nested = child1.addChildFrame("frame-2", "widget", "https://cdn.other.com/widget")?;
    expect_eq(mf.childFrames().len, 1);
    expect_eq(child1.parentFrame() != null, true);
    expect_eq(nested.parentFrame() != null, true);

    const found_name = page.frameByName("widget");
    expect_eq(found_name != null, true);
    expect_eq(std.mem.eql(u8, found_name.?.id(), "frame-2"), true);

    const found_url = page.frameByUrl("*cdn.other.com*");
    expect_eq(found_url != null, true);
    expect_eq(std.mem.eql(u8, found_url.?.name(), "widget"), true);

    const all_frames = page.frames(alloc)?;
    expect_eq(all_frames.len, 3);
    alloc.free(all_frames);

    const loc = child1.locator("button.nav");
    expect_eq(std.mem.eql(u8, loc.selector(), "button.nav"), true);

    const resp = child1.goto("https://example.com/nav2", "")?;
    expect_eq(resp.isOk(), true);
    expect_eq(std.mem.eql(u8, child1.url(), "https://example.com/nav2"), true);

    const eval_res = nested.evaluate("1 + 1")?;
    * = eval_res;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testTracingAndZipArchive(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const ctx = browser.newContext(.{})?;

    let tr = ctx.tracing();
    expect_eq(tr.isRecording(), false);
    tr.start(.{ name: "e2e", title: "chunk1", screenshots: true, snapshots: true })?;
    expect_eq(tr.isRecording(), true);

    tr.recordEvent("action", "Page.goto", "page-1", "{\"url\":\"https://example.com\"}")?;
    expect_eq(tr.eventCount() >= 2, true);

    const zip1 = tr.stopChunk("")?;
    expect_eq(zip1.len > 22, true);
    expect_eq(zip1[0] == 'P' && zip1[1] == 'K', true);
    alloc.free(zip1);

    tr.startChunk("chunk2")?;
    tr.recordEvent("action", "Locator.click", "page-1", "{}")?;
    const zip2 = tr.stop("")?;
    expect_eq(zip2.len > 22, true);
    expect_eq(tr.isRecording(), false);
    alloc.free(zip2);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testCoverageRecorder(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const page = browser.newPage()?;

    let cov = page.coverage();
    expect_eq(cov.isJSCoverageActive(), false);
    expect_eq(cov.isCSSCoverageActive(), false);

    cov.startJSCoverage(.{ call_count: true, detailed: true })?;
    expect_eq(cov.isJSCoverageActive(), true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Profiler.startPreciseCoverage"), true);

    const js_entries = cov.stopJSCoverage(alloc)?;
    alloc.free(js_entries);
    expect_eq(cov.isJSCoverageActive(), false);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Profiler.stopPreciseCoverage"), true);

    cov.startCSSCoverage(.{})?;
    expect_eq(cov.isCSSCoverageActive(), true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "CSS.startRuleUsageTracking"), true);

    const css_entries = cov.stopCSSCoverage(alloc)?;
    alloc.free(css_entries);
    expect_eq(cov.isCSSCoverageActive(), false);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "CSS.stopRuleUsageTracking"), true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testApiRequestContextAndCookieSync(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const ctx = browser.newContext(.{})?;

    const c: [1]Cookie = .{ Cookie{ name: "auth", value: "token-999", domain: "api.example.com", path: "/" } };
    ctx.addCookies(&c)?;

    let req_ctx = APIRequestContext.init(alloc, .{ base_url: "https://api.example.com" });
    req_ctx.syncCookiesFromContext(ctx)?;
    expect_eq(req_ctx.cookies().len, 1);
    expect_eq(std.mem.eql(u8, req_ctx.cookies()[0].value, "token-999"), true);

    const get_res = req_ctx.get("/v1/users", .{})?;
    expect_eq(get_res.isOk(), true);
    expect_eq(std.mem.eql(u8, get_res.url, "https://api.example.com/v1/users"), true);
    expect_eq(std.mem.indexOf(u8, get_res.raw_request, "GET /v1/users HTTP/1.1") != null, true);

    const post_res = req_ctx.post("/v1/items", .{ data: "{\"id\":42}" })?;
    expect_eq(post_res.statusCode(), 200);
    expect_eq(std.mem.indexOf(u8, post_res.raw_request, "POST /v1/items HTTP/1.1") != null, true);

    const put_res = req_ctx.put("/v1/items/42", .{ data: "{\"id\":42}" })?;
    expect_eq(std.mem.indexOf(u8, put_res.raw_request, "PUT /v1/items/42 HTTP/1.1") != null, true);

    const del_res = req_ctx.delete("/v1/items/42", .{})?;
    expect_eq(std.mem.indexOf(u8, del_res.raw_request, "DELETE /v1/items/42 HTTP/1.1") != null, true);

    req_ctx.addCookie(Cookie{ name: "csrf", value: "abc", domain: "api.example.com", path: "/" })?;
    req_ctx.syncCookiesToContext(ctx)?;
    expect_eq(ctx.cookies().len >= 2, true);

    req_ctx.deinit();
    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testWebSocketRouteAndVideoScreencast(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const page = browser.newPage()?;

    let ws_state = WsRouteTestState{};
    page.routeWebSocket("wss://echo.example.com/*", testWsRouteHandler, unsafe { @intFromPtr(&ws_state) })?;

    let ws_route = WebSocketRoute.init(alloc, "wss://echo.example.com/chat");
    const matched = page.dispatchWebSocketRoute(&ws_route);
    expect_eq(matched, true);
    expect_eq(ws_state.handled, true);
    expect_eq(ws_route.connected_to_server, true);
    expect_eq(ws_route.sent_to_page.items.len, 1);
    expect_eq(ws_route.sent_to_server.items.len, 1);
    ws_route.close(1000, "done");
    expect_eq(ws_route.isClosed(), true);
    ws_route.deinit();

    const vid = page.video()?;
    vid.start("jpeg", 80)?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.startScreencast"), true);
    vid.recordFrame("AQID", 7)?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.screencastFrameAck"), true);
    expect_eq(vid.frameCount(), 1);
    vid.saveAs("/tmp/test-video.webm")?;
    expect_eq(vid.path() != null, true);
    vid.stop()?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.stopScreencast"), true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testTraceViewerAndCliShowTrace(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const ctx = browser.newContext(.{})?;

    let tr = ctx.tracing();
    tr.start(.{ name: "viewer-e2e", title: "viewer-chunk", screenshots: true, snapshots: true })?;
    tr.recordEvent("action", "Page.goto", "page-1", "{\"url\":\"https://example.com\"}")?;
    tr.recordEvent("action", "Locator.click", "page-1", "{\"selector\":\"button\"}")?;
    const zip_bytes = tr.stop("")?;
    expect_eq(zip_bytes.len > 22, true);
    expect_eq(zip_bytes[0] == 'P' && zip_bytes[1] == 'K', true);

    let viewer = TraceViewerServer.init(alloc, TraceViewerOptions{
        host: "127.0.0.1",
        port: 9323,
    });
    viewer.loadZipBytes(zip_bytes)?;
    expect_eq(viewer.entryCount(), 3);
    expect_eq(viewer.event_count >= 3, true);

    let url_buf: [64]u8 = undefined;
    const base_url = viewer.url(&url_buf)?;
    expect_eq(std.mem.eql(u8, base_url, "http://127.0.0.1:9323"), true);

    let root_resp = viewer.handleHttpRequest(alloc, "GET", "/")?;
    expect_eq(root_resp.status, 200);
    expect_eq(std.mem.indexOf(u8, root_resp.content_type, "text/html") != null, true);
    expect_eq(std.mem.indexOf(u8, root_resp.body, "Playwright Trace Viewer (CP)") != null, true);
    expect_eq(std.mem.indexOf(u8, root_resp.body, "/contexts") != null, true);
    root_resp.deinit();

    let ctx_resp = viewer.handleHttpRequest(alloc, "GET", "/contexts")?;
    expect_eq(ctx_resp.status, 200);
    expect_eq(std.mem.indexOf(u8, ctx_resp.content_type, "application/json") != null, true);
    expect_eq(std.mem.indexOf(u8, ctx_resp.body, "\"eventCount\"") != null, true);
    expect_eq(std.mem.indexOf(u8, ctx_resp.body, "\"trace.trace\"") != null, true);
    ctx_resp.deinit();

    let file_resp = viewer.handleHttpRequest(alloc, "GET", "/trace/file?path=trace.trace")?;
    expect_eq(file_resp.status, 200);
    expect_eq(std.mem.indexOf(u8, file_resp.content_type, "application/x-ndjson") != null, true);
    expect_eq(std.mem.indexOf(u8, file_resp.body, "Page.goto") != null, true);
    expect_eq(std.mem.indexOf(u8, file_resp.body, "Locator.click") != null, true);
    file_resp.deinit();

    let health_resp = viewer.handleHttpRequest(alloc, "GET", "/health")?;
    expect_eq(health_resp.status, 200);
    expect_eq(std.mem.indexOf(u8, health_resp.body, "ok") != null, true);
    health_resp.deinit();

    // CLI show-trace parsing verification
    const st_args: [3][]const u8 = .{ "show-trace", "trace.zip", "--port=9444" };
    const st_opts = PlaywrightCli.parseArgs(&st_args)?;
    expect_eq(st_opts.command == CliCommand.ShowTrace, true);
    expect_eq(std.mem.eql(u8, st_opts.trace_path, "trace.zip"), true);
    expect_eq(st_opts.port, 9444);

    viewer.deinit();
    browser.deinit();
    alloc.free(zip_bytes);
    expect_eq(gpa.deinit() == .ok, true);
}

fn testWebKitEngineAndWkConnection(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    // 1. BrowserType name & executablePath probing
    expect_eq(std.mem.eql(u8, webkit.name(), "webkit"), true);
    if (exe_missing := webkit.executablePath(alloc, "/nonexistent/webkit/bin")) {
        alloc.free(exe_missing);
    }
    if (exe_sh := webkit.executablePath(alloc, "/bin/sh")) {
        expect_eq(std.mem.eql(u8, exe_sh, "/bin/sh"), true);
        alloc.free(exe_sh);
    }

    // 2. buildLaunchArgs (--inspector-pipe, --headless, --no-startup-window, --proxy)
    const args1 = webkit.buildLaunchArgs(alloc, .{
        headless: true,
        proxy_server: "http://127.0.0.1:8080",
        proxy_bypass: "localhost",
    })?;
    let found_pipe = false;
    let found_headless = false;
    let found_no_window = false;
    let found_proxy = false;
    let found_ignore = false;
    for (a of args1) {
        if (std.mem.eql(u8, a, "--inspector-pipe")) found_pipe = true;
        if (std.mem.eql(u8, a, "--headless")) found_headless = true;
        if (std.mem.eql(u8, a, "--no-startup-window")) found_no_window = true;
        if (std.mem.eql(u8, a, "--proxy=http://127.0.0.1:8080")) found_proxy = true;
        if (std.mem.eql(u8, a, "--ignore-host=localhost")) found_ignore = true;
    }
    expect_eq(found_pipe, true);
    expect_eq(found_headless, true);
    expect_eq(found_no_window, true);
    expect_eq(found_proxy, true);
    expect_eq(found_ignore, true);
    webkit.freeLaunchArgs(alloc, args1);

    // 3. WkConnection Inspector JSON-RPC frame encode/decode & pageProxyId routing
    let enc_buf: [512]u8 = undefined;
    const enc_len = WkConnection.encodeFrame(42, "Page.navigate", "{\"url\":\"https://webkit.org\"}", "pp-1", &enc_buf)?;
    const decoded = WkConnection.decodeFrame(enc_buf[0..enc_len])?;
    expect_eq(decoded.kind == WkMessageKind.Request, true);
    expect_eq(decoded.id, 42);
    expect_eq(std.mem.eql(u8, decoded.method, "Page.navigate"), true);
    expect_eq(std.mem.eql(u8, decoded.page_proxy_id, "pp-1"), true);

    let conn = WkConnection.init(alloc);
    let mt = MockCdpTransport{};
    conn.setTransport(mockTransportSend, unsafe { @intFromPtr(&mt) });
    conn.dispatchMessage("{\"id\":-9999,\"result\":{}}")?;
    expect_eq(conn.ignored_close_count, 1);

    conn.dispatchMessage("{\"method\":\"Playwright.pageProxyCreated\",\"params\":{\"pageProxyId\":\"pp-1\"}}")?;
    conn.dispatchMessage("{\"method\":\"Playwright.pageProxyCreated\",\"params\":{\"pageProxyId\":\"pp-2\"}}")?;
    expect_eq(conn.sessionCount(), 2);

    conn.dispatchMessage("{\"id\":1,\"result\":{\"value\":\"wk-ok\"},\"pageProxyId\":\"pp-1\",\"targetId\":\"target-1\"}")?;
    conn.dispatchMessage("{\"id\":2,\"result\":{\"value\":\"wk-two\"},\"pageProxyId\":\"pp-2\",\"targetId\":\"target-2\"}")?;
    const s1 = conn.getPageProxySession("pp-1") orelse return PlaywrightError.TargetNotFound;
    const s2 = conn.getPageProxySession("pp-2") orelse return PlaywrightError.TargetNotFound;
    expect_eq(std.mem.eql(u8, s1.last_result_json, "{\"value\":\"wk-ok\"}"), true);
    expect_eq(std.mem.eql(u8, s2.last_result_json, "{\"value\":\"wk-two\"}"), true);
    * = s1.sendToTarget("Runtime.evaluate", "{\"expression\":\"1+1\"}")?;
    expect_eq(std.mem.eql(u8, mt.lastMethod(), "Target.sendMessageToTarget"), true);
    conn.deinit();

    // 4. WebKitBrowser launchWithTransport, Page, Input, Network Route & RAII cleanup
    let mt2 = MockCdpTransport{};
    let browser = webkit.launchWithTransport(alloc, .{
        headless: true,
        viewport_width: 1024,
        viewport_height: 768,
    }, mockTransportSend, unsafe { @intFromPtr(&mt2) })?;
    expect_eq(browser.isConnected(), true);
    expect_eq(std.mem.eql(u8, browser.version(), "27.2"), true);

    const ctx = browser.newContext(.{ viewport_width: 1024, viewport_height: 768 })?;
    const page = ctx.newPage()?;
    const resp = page.goto("https://example.com/wk", .{})?;
    expect_eq(resp.isOk(), true);
    expect_eq(std.mem.eql(u8, page.url(), "https://example.com/wk"), true);

    page.setContent("<h1>WebKit CP</h1>")?;
    expect_eq(std.mem.eql(u8, page.content(), "<h1>WebKit CP</h1>"), true);
    * = page.evaluate("document.title")?;
    page.setViewport(1280, 800)?;
    expect_eq(page.viewport_width, 1280);
    expect_eq(page.viewport_height, 800);
    * = page.screenshot(.{})?;

    page.keyboardDown("Enter", 0)?;
    page.keyboardUp("Enter", 0)?;
    page.insertText("hello webkit")?;
    page.click(10.0, 20.0)?;
    expect_eq(std.mem.eql(u8, mt2.lastMethod(), "Input.dispatchMouseEvent"), true);

    let rstate = RouteTestState{ mode: 0 };
    page.route("**/api/*", testRouteHandler, unsafe { @intFromPtr(&rstate) })?;
    const act = page.interceptRequest("https://example.com/api/ping", "GET", "fetch")?;
    expect_eq(act == RouteAction.Fulfilled, true);
    expect_eq(rstate.intercepted_count, 1);
    page.unroute("**/api/*");

    browser.deinit();

    // 5. connectWithTransport
    let mt3 = MockCdpTransport{};
    let conn_browser = webkit.connectWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&mt3) })?;
    expect_eq(conn_browser.isConnected(), true);
    conn_browser.deinit();

    expect_eq(gpa.deinit() == .ok, true);
}

fn testBindingHandler(ctx: usize, args_json: []const u8): []const u8 {
    * = ctx;
    return if (args_json.len > 0) args_json else "ok";
}

fn testPageFullDomEmulationAndWaits(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const page = browser.newPage()?;

    // Scripts, styles, bindings
    page.addInitScript("window.__init = true;")?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.addScriptToEvaluateOnNewDocument"), true);
    * = page.addScriptTag(.{ content: "window.__tag = 1;" })?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);
    * = page.addStyleTag(.{ content: "body { color: red; }" })?;
    page.exposeBinding("myBinding", testBindingHandler, 0)?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.addBinding"), true);
    page.exposeFunction("myFunc", testBindingHandler, 0)?;
    if (res := page.invokeExposedBinding("myFunc", "[42]")) {
        expect_eq(std.mem.eql(u8, res, "[42]"), true);
    } else {
        expect_eq(true, false);
    }

    // Environment & viewport
    page.bringToFront()?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Page.bringToFront"), true);
    page.emulateMedia(.{ media: "print", color_scheme: "dark", reduced_motion: "reduce" })?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Emulation.setEmulatedMedia"), true);
    page.setExtraHTTPHeaders("{\"X-Test\":\"cp\"}")?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Network.setExtraHTTPHeaders"), true);
    page.setViewportSize(1440, 900)?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Emulation.setDeviceMetricsOverride"), true);
    const vp = page.viewportSize();
    expect_eq(vp.width, 1440);
    expect_eq(vp.height, 900);
    expect_eq(page.isClosed(), false);
    expect_eq(page.opener() == null, true);
    page.registerWorker("https://example.com/worker.js", "w-1")?;
    expect_eq(page.workers().len, 1);
    page.pause()?;
    expect_eq(page.paused, true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Debugger.pause"), true);

    // Quick DOM actions & queries
    page.hover("#btn")?;
    page.dblclick("#btn")?;
    page.tap("#btn")?;
    page.check("#agree")?;
    page.uncheck("#agree")?;
    page.selectOption("#lang", "zh")?;
    page.focus("#email")?;
    page.dispatchEvent("#btn", "custom-click", "{\"detail\":1}")?;
    page.dragAndDrop("#src", "#dst")?;

    const ih = page.innerHTML(alloc, "#box")?;
    alloc.free(ih);
    const it = page.innerText(alloc, "#box")?;
    alloc.free(it);
    const tc = page.textContent(alloc, "#box")?;
    alloc.free(tc);
    const iv = page.inputValue(alloc, "#email")?;
    alloc.free(iv);
    if (attr := page.getAttribute(alloc, "#btn", "role")?) {
        alloc.free(attr);
    }
    * = page.isChecked("#agree")?;
    * = page.isDisabled("#btn")?;
    * = page.isEditable("#email")?;
    * = page.isEnabled("#btn")?;
    const vis = page.isVisible("#btn")?;
    const hid = page.isHidden("#btn")?;
    expect_eq(vis != hid, true);

    // Waits & events
    * = page.goto("https://example.com/dashboard", .{})?;
    page.waitForURL("**/dashboard", 1000)?;
    page.waitForLoadState("networkidle")?;
    * = page.waitForFunction("() => document.readyState === 'complete'", 1000)?;
    page.waitForTimeout(1)?;

    page.emitEvent("dialog", "confirm")?;
    const ev_payload = page.waitForEvent("dialog", 1000)?;
    expect_eq(std.mem.eql(u8, ev_payload, "confirm"), true);

    page.recordRequest(Request{ url: "https://example.com/api/items", method: "GET" })?;
    const matched_req = page.waitForRequest("**/api/items", 1000)?;
    expect_eq(std.mem.eql(u8, matched_req.method, "GET"), true);

    page.recordResponse(Response{ url: "https://example.com/api/items", status: 201, status_text: "Created", ok: true })?;
    const matched_resp = page.waitForResponse("**/api/items", 1000)?;
    expect_eq(matched_resp.status, 201);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testBrowserContextPermissionsGeoOfflineAndCdp(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const ctx = browser.newContext(.{})?;

    expect_eq(ctx.browser() != 0, true);
    expect_eq(ctx.backgroundPages().len, 0);
    ctx.registerServiceWorker("https://example.com/sw.js", "sw-1")?;
    expect_eq(ctx.serviceWorkers().len, 1);

    ctx.addInitScript("window.__ctxInit = 1;")?;
    ctx.exposeBinding("ctxBind", testBindingHandler, 0)?;
    ctx.exposeFunction("ctxFn", testBindingHandler, 0)?;

    const page = ctx.newPage()?;
    expect_eq(page.invokeExposedBinding("ctxFn", "ok") != null, true);

    const perms: [2][]const u8 = .{ "geolocation", "notifications" };
    ctx.grantPermissions(&perms, "https://example.com")?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Browser.grantPermissions"), true);
    ctx.clearPermissions()?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Browser.resetPermissions"), true);

    ctx.setGeolocation(Geolocation{ latitude: 37.7749, longitude: -122.4194, accuracy: 10.0 })?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Emulation.setGeolocationOverride"), true);
    ctx.setGeolocation(null)?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Emulation.clearGeolocationOverride"), true);

    ctx.setExtraHTTPHeaders("{\"Authorization\":\"Bearer tok\"}")?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Network.setExtraHTTPHeaders"), true);

    ctx.setOffline(true)?;
    expect_eq(ctx.is_offline, true);
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Network.emulateNetworkConditions"), true);

    ctx.setHTTPCredentials(HTTPCredentials{ username: "admin", password: "secret", origin: "https://example.com" })?;
    expect_eq(ctx.http_credentials != null, true);

    ctx.setDefaultTimeout(5000);
    ctx.setDefaultNavigationTimeout(12000);
    expect_eq(page.default_timeout_ms, 5000);
    expect_eq(page.default_navigation_timeout_ms, 12000);

    ctx.routeFromHAR("/tmp/mock.har", "**/api/**")?;
    ctx.emitEvent("page", "page-2")?;
    const ctx_ev = ctx.waitForEvent("page", 1000)?;
    expect_eq(std.mem.eql(u8, ctx_ev, "page-2"), true);

    let cdp = ctx.newCDPSession(page)?;
    expect_eq(cdp.sessionId().len > 0, true);
    * = cdp.send("Runtime.enable", "{}")?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.enable"), true);
    cdp.detach()?;
    expect_eq(cdp.detached, true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testFrameFullActionsAndSelectors(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let transport = MockCdpTransport{};
    let browser = chromium.launchWithTransport(alloc, .{}, mockTransportSend, unsafe { @intFromPtr(&transport) })?;
    const page = browser.newPage()?;
    const mf = page.mainFrame()?;

    expect_eq(mf.isDetached(), false);
    expect_eq(mf.page() != 0, true);
    mf.setTitle("Main Frame");
    expect_eq(std.mem.eql(u8, mf.title(), "Main Frame"), true);

    mf.setContent("<div id='app'>Hello Frame</div>")?;
    expect_eq(std.mem.eql(u8, mf.content(), "<div id='app'>Hello Frame</div>"), true);
    * = mf.addScriptTag(.{ url: "https://example.com/a.js" })?;
    * = mf.addStyleTag(.{ url: "https://example.com/a.css" })?;

    // Selectors
    const role_loc = mf.getByRole("button", .{ name: "Save" });
    expect_eq(std.mem.indexOf(u8, role_loc.selector(), "internal:role=button") != null, true);
    const text_loc = mf.getByText("Hello", true);
    expect_eq(std.mem.indexOf(u8, text_loc.selector(), "internal:text=") != null, true);
    const label_loc = mf.getByLabel("Email", false);
    expect_eq(std.mem.indexOf(u8, label_loc.selector(), "internal:label=") != null, true);
    const ph_loc = mf.getByPlaceholder("Search", false);
    expect_eq(std.mem.indexOf(u8, ph_loc.selector(), "placeholder") != null, true);
    const tid_loc = mf.getByTestId("submit-btn");
    expect_eq(std.mem.indexOf(u8, tid_loc.selector(), "data-testid") != null, true);

    // Actions & queries
    mf.click("#go")?;
    mf.dblclick("#go")?;
    mf.fill("#input", "abc")?;
    mf.typeText("#input", "xyz")?;
    mf.press("#input", "Enter")?;
    mf.hover("#go")?;
    mf.focus("#input")?;
    mf.check("#cb")?;
    mf.uncheck("#cb")?;
    mf.selectOption("#sel", "v1")?;
    mf.tap("#go")?;
    mf.dispatchEvent("#go", "ping", "{}")?;
    mf.dragAndDrop("#a", "#b")?;

    const ih = mf.innerHTML(alloc, "#app")?;
    alloc.free(ih);
    const it = mf.innerText(alloc, "#app")?;
    alloc.free(it);
    const tc = mf.textContent(alloc, "#app")?;
    alloc.free(tc);
    const iv = mf.inputValue(alloc, "#input")?;
    alloc.free(iv);
    if (attr := mf.getAttribute(alloc, "#go", "id")?) {
        alloc.free(attr);
    }
    * = mf.isChecked("#cb")?;
    * = mf.isDisabled("#go")?;
    * = mf.isEditable("#input")?;
    * = mf.isEnabled("#go")?;
    * = mf.isVisible("#go")?;
    * = mf.isHidden("#go")?;

    * = mf.waitForSelector("#app", .{})?;
    * = mf.waitForFunction("() => true", 1000)?;
    mf.waitForLoadState("domcontentloaded")?;
    * = mf.goto("https://example.com/frame-page", "")?;
    mf.waitForURL("**/frame-page", 1000)?;
    * = mf.frameElement()?;
    expect_eq(std.mem.eql(u8, transport.lastMethod(), "DOM.getFrameOwner"), true);

    browser.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testNetworkRequestResponseRouteFullApi(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    let prev_req = Request{
        url: "https://example.com/old",
        method: "GET",
        resource_type: "document",
        is_navigation_request: true,
        frame_id: "frame-main",
    };

    let req = Request{
        url: "https://example.com/api/v1/users",
        method: "POST",
        resource_type: "fetch",
        post_data: "{\"name\":\"alice\"}",
        headers_json: "{\"Content-Type\":\"application/json\",\"X-Trace-Id\":\"abc-123\"}",
        is_navigation_request: false,
        frame_id: "frame-main",
        failure_text: "net::ERR_ABORTED",
        redirected_from: &prev_req,
        resource_timing: ResourceTiming{
            start_time: 100.0,
            response_start: 120.0,
            response_end: 135.0,
        },
    };
    prev_req.redirected_to = &req;

    let resp = Response{
        url: "https://example.com/api/v1/users",
        status: 201,
        status_text: "Created",
        ok: true,
        headers_json: "{\"Content-Type\":\"application/json\",\"X-Rate\":\"60\"}",
        body_bytes: "{\"id\":7,\"name\":\"alice\"}",
        from_service_worker: true,
        finish_error: null,
        security_details: SecurityDetails{
            issuer: "Let's Encrypt",
            protocol: "TLS 1.3",
            subject_name: "example.com",
            valid_from: 1700000000,
            valid_to: 1800000000,
        },
        server_addr: RemoteAddr{
            ip_address: "93.184.216.34",
            port: 443,
        },
        request_ptr: &req,
    };
    req.response_ptr = &resp;

    // Request full API
    expect_eq(std.mem.eql(u8, req.allHeaders(), req.headers_json), true);
    const req_hdrs = req.headersArray(alloc)?;
    expect_eq(req_hdrs.len, 2);
    alloc.free(req_hdrs);
    if (ct := req.headerValue("content-type")) {
        expect_eq(std.mem.eql(u8, ct, "application/json"), true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(req.postData() != null, true);
    expect_eq(req.postDataJSON() != null, true);
    expect_eq(req.redirectedFrom() != null, true);
    expect_eq(prev_req.redirectedTo() != null, true);
    expect_eq(std.mem.eql(u8, req.resourceType(), "fetch"), true);
    expect_eq(req.isNavigationRequest(), false);
    expect_eq(req.timing().start_time == 100.0, true);
    expect_eq(req.sizes().request_body_size > 0, true);
    if (fail := req.failure()) {
        expect_eq(std.mem.eql(u8, fail.error_text, "net::ERR_ABORTED"), true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(req.response() != null, true);
    expect_eq(std.mem.eql(u8, req.frame(), "frame-main"), true);

    // Response full API
    expect_eq(std.mem.eql(u8, resp.allHeaders(), resp.headers_json), true);
    const resp_hdrs = resp.headersArray(alloc)?;
    expect_eq(resp_hdrs.len, 2);
    alloc.free(resp_hdrs);
    if (xr := resp.headerValue("x-rate")) {
        expect_eq(std.mem.eql(u8, xr, "60"), true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(std.mem.eql(u8, resp.body(), "{\"id\":7,\"name\":\"alice\"}"), true);
    expect_eq(std.mem.eql(u8, resp.text(), "{\"id\":7,\"name\":\"alice\"}"), true);
    const rj = resp.json()?;
    expect_eq(rj.len > 0 && rj[0] == '{', true);
    expect_eq(resp.finished() == null, true);
    expect_eq(resp.fromServiceWorker(), true);
    if (sec := resp.securityDetails()) {
        expect_eq(std.mem.eql(u8, sec.protocol, "TLS 1.3"), true);
    } else {
        expect_eq(true, false);
    }
    if (addr := resp.serverAddr()) {
        expect_eq(addr.port, 443);
    } else {
        expect_eq(true, false);
    }
    expect_eq(std.mem.eql(u8, resp.statusText(), "Created"), true);
    expect_eq(resp.request() != null, true);

    // Route full API (fallback, fetch, continueWithOverrides)
    let r1 = Route{ request: req };
    const fetched = r1.fetch(.{ url: "https://example.com/api/v2/users", headers_json: "{\"X-Override\":\"1\"}" })?;
    expect_eq(r1.fetched, true);
    expect_eq(std.mem.eql(u8, fetched.url, "https://example.com/api/v2/users"), true);
    r1.continueWithOverrides(.{
        url: "https://example.com/api/v2/users",
        method: "PUT",
        headers_json: "{\"X-Override\":\"1\"}",
        post_data: "{\"updated\":true}",
    })?;
    expect_eq(r1.action == RouteAction.Continued, true);
    expect_eq(std.mem.eql(u8, r1.override_method, "PUT"), true);

    let r2 = Route{ request: req };
    r2.fallback()?;
    expect_eq(r2.action == RouteAction.Fallback, true);

    expect_eq(gpa.deinit() == .ok, true);
}

fn main() {
    test("1. Chromium Launch, Args Builder & Executable Probing", () => {
        testChromiumLaunchAndLauncherProbe() catch unreachable;
    });
    test("2. connectOverCDP & DevTools WebSocket URL Parsing", () => {
        testConnectOverCDP() catch unreachable;
    });
    test("3. BrowserContext Isolation, Cookies & StorageState", () => {
        testBrowserContextCookiesAndStorageState() catch unreachable;
    });
    test("4. Page Navigation, History, Content & CDP Domains", () => {
        testPageNavigationContentAndCdpDomains() catch unreachable;
    });
    test("5. Locator Semantics, Chaining & Auto-Wait Actions", () => {
        testLocatorSelectorsChainingAndActions() catch unreachable;
    });
    test("6. Network Route Interception & Glob UrlMatcher", () => {
        testNetworkRoutingAndUrlMatcher() catch unreachable;
    });
    test("7. AriaSnapshot YAML & CDP Accessibility Tree", () => {
        testAriaSnapshotAndCdpAXTree() catch unreachable;
    });
    test("8. Actionability Engine & SIMD Selector/Glob Acceleration", () => {
        testActionabilityEngineAndSimdSelectors() catch unreachable;
    });
    test("9. Clock Time Control (install/fastForward/pauseAt/setFixedTime)", () => {
        testClockTimeControl() catch unreachable;
    });
    test("10. Download, FileChooser, ConsoleMessage & HarRecorder", () => {
        testArtifactsDownloadFileChooserConsoleAndHar() catch unreachable;
    });
    test("11. PlaywrightRemoteServer & Client WS RPC Protocol", () => {
        testRemoteServerAndClientRpc() catch unreachable;
    });
    test("12. Playwright CLI Actions (screenshot/pdf/codegen)", () => {
        testPlaywrightCliActions() catch unreachable;
    });
    test("13. Frame Tree Management (mainFrame/childFrames/frameByName/frameByUrl)", () => {
        testFrameTreeManagement() catch unreachable;
    });
    test("14. Tracing & ZIP Archive Output (cap:pack ZipWriter)", () => {
        testTracingAndZipArchive() catch unreachable;
    });
    test("15. Coverage Recorder (JS & CSS Precise Coverage)", () => {
        testCoverageRecorder() catch unreachable;
    });
    test("16. APIRequestContext & BrowserContext Cookie Jar Sync", () => {
        testApiRequestContextAndCookieSync() catch unreachable;
    });
    test("17. WebSocketRoute & Video Screencast Handle", () => {
        testWebSocketRouteAndVideoScreencast() catch unreachable;
    });
    test("18. TraceViewerServer & CLI show-trace (PKZIP + HTTP Routes)", () => {
        testTraceViewerAndCliShowTrace() catch unreachable;
    });
    test("19. WebKit Engine, WkConnection, Args Builder & Executable Probing", () => {
        testWebKitEngineAndWkConnection() catch unreachable;
    });
    test("20. Page Full DOM Actions, Emulation, Bindings & Wait Primitives", () => {
        testPageFullDomEmulationAndWaits() catch unreachable;
    });
    test("21. BrowserContext Permissions, Geolocation, Offline & CDPSession", () => {
        testBrowserContextPermissionsGeoOfflineAndCdp() catch unreachable;
    });
    test("22. Frame Full DOM Actions, Queries, Selectors & Waits", () => {
        testFrameFullActionsAndSelectors() catch unreachable;
    });
    test("23. Network Request, Response & Route Full API Surface", () => {
        testNetworkRequestResponseRouteFullApi() catch unreachable;
    });
}
