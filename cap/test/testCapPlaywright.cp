// Source: cp/cap/test/testCapPlaywright.cp
// Comprehensive Verification Suite for packages/playwright (cap:cdp backed)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { CdpFrameScanner, CdpBrowserLauncher } from "cap:cdp";
import {
    PlaywrightError,
    RouteAction,
    FulfillOptions,
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
    Page,
    Cookie,
    StorageState,
    BrowserContextOptions,
    BrowserContext,
    Browser,
    LaunchOptions,
    ConnectOverCdpOptions,
    chromium,
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
}
