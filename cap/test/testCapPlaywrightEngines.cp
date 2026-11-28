// Source: cp/cap/test/testCapPlaywrightEngines.cp
// Verification Suite for Playwright Core Internal State Machine Engines:
//   1. FrameManager & DomActionabilityLoop (frame_manager.cp)
//   2. CrNetworkManager (cr_network_manager.cp)
//   3. Snapshotter & HarTracer (snapshotter.cp)
//   4. APIRequestContext redirects, MultipartFormData & ProxySettings (request_context.cp)
// Verified with std.heap.DebugAllocator for zero memory leaks.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { CdpFrameScanner } from "cap:cdp";
import { chromium } from "../../packages/playwright/src/chromium.cp";
import {
    PlaywrightError,
    Route,
    RouteAction,
    FulfillOptions,
    RouteContinueOverrides,
    ResourceTiming,
    Request,
    Response,
} from "../../packages/playwright/src/network.cp";
import { HTTPCredentials } from "../../packages/playwright/src/context.cp";
import {
    ActionabilityState,
    ActionabilityCheckKind,
    ActionabilityOptions,
    ElementBox,
} from "../../packages/playwright/src/actionability.cp";
import {
    FrameLifecycleState,
    FrameManager,
    ActionabilityProbeSnapshot,
    DomActionabilityLoop,
    simdFindByteFrame,
} from "../../packages/playwright/src/frame_manager.cp";
import {
    CrNetworkRequestState,
    CrAuthResponseAction,
    CrAuthChallenge,
    CrNetworkManager,
} from "../../packages/playwright/src/cr_network_manager.cp";
import {
    SNAPSHOTTER_INJECTED_SCRIPT,
    Snapshotter,
    HarTimings,
    HarTracer,
    HarTracerEntry,
    simdHasBinaryBytes,
} from "../../packages/playwright/src/snapshotter.cp";
import {
    ProxySettings,
    MultipartFormData,
    FetchTransportResult,
    APIResponse,
    APIRequestContext,
    simdHasHeaderUnsafeBytes,
} from "../../packages/playwright/src/request_context.cp";

// ----------------------------------------------------------------------------
// Helpers & Mock Transports
// ----------------------------------------------------------------------------

struct EngineMockCdp {
    sent_count: usize = 0,
    seen_fulfill: bool = false,
    seen_continue: bool = false,
    seen_auth: bool = false,
    seen_add_script: bool = false,

    fn record(*_, payload: []const u8): bool {
        _.sent_count += 1;
        if (m := CdpFrameScanner.extractTopLevelField(payload, "method", true)) {
            if (std.mem.eql(u8, m, "Fetch.fulfillRequest")) _.seen_fulfill = true;
            if (std.mem.eql(u8, m, "Fetch.continueRequest")) _.seen_continue = true;
            if (std.mem.eql(u8, m, "Fetch.continueWithAuth")) _.seen_auth = true;
            if (std.mem.eql(u8, m, "Page.addScriptToEvaluateOnNewDocument")) _.seen_add_script = true;
        }
        return true;
    }
}

fn engineMockCdpSend(ctx: usize, payload: []const u8): bool {
    const m = unsafe { ctx as *EngineMockCdp! };
    return m.record(payload);
}

struct ActionabilityScenario {
    calls: u32 = 0,
}

fn animatingThenStableProbe(ctx: usize, selector: []const u8, attempt: u32, scrolled: bool): ActionabilityProbeSnapshot {
    * = selector;
    const sc = unsafe { ctx as *ActionabilityScenario! };
    sc.calls += 1;
    // Attempt 0: detached -> re-locate
    if (attempt == 0) {
        return ActionabilityProbeSnapshot{ attached: false, visible: false };
    }
    // Attempt 1: visible, animating box (x=10)
    if (attempt == 1) {
        return ActionabilityProbeSnapshot{
            attached: true,
            visible: true,
            enabled: true,
            editable: true,
            in_viewport: false,
            hit_target_ok: true,
            box: ElementBox{ x: 10.0, y: 20.0, width: 120.0, height: 40.0 },
        };
    }
    // Attempt 2: animating box moved (x=50)
    if (attempt == 2) {
        return ActionabilityProbeSnapshot{
            attached: true,
            visible: true,
            enabled: true,
            editable: true,
            in_viewport: false,
            hit_target_ok: true,
            box: ElementBox{ x: 50.0, y: 20.0, width: 120.0, height: 40.0 },
        };
    }
    // Attempt 3: box stabilized at x=50, still outside viewport -> triggers auto scrollIntoView
    if (!scrolled) {
        return ActionabilityProbeSnapshot{
            attached: true,
            visible: true,
            enabled: true,
            editable: true,
            in_viewport: false,
            hit_target_ok: true,
            box: ElementBox{ x: 50.0, y: 20.0, width: 120.0, height: 40.0 },
        };
    }
    // After scroll: 1st frame obscured, then hit_target_ok = true and stable for 2 frames!
    if (attempt == 4) {
        return ActionabilityProbeSnapshot{
            attached: true,
            visible: true,
            enabled: true,
            editable: true,
            in_viewport: true,
            hit_target_ok: false,
            box: ElementBox{ x: 50.0, y: 100.0, width: 120.0, height: 40.0 },
            obscured_by: "<div class=\"modal-overlay\"> intercepts pointer events",
        };
    }
    return ActionabilityProbeSnapshot{
        attached: true,
        visible: true,
        enabled: true,
        editable: true,
        in_viewport: true,
        hit_target_ok: true,
        box: ElementBox{ x: 50.0, y: 100.0, width: 120.0, height: 40.0 },
    };
}

fn alwaysObscuredProbe(ctx: usize, selector: []const u8, attempt: u32, scrolled: bool): ActionabilityProbeSnapshot {
    * = ctx;
    * = selector;
    * = attempt;
    * = scrolled;
    return ActionabilityProbeSnapshot{
        attached: true,
        visible: true,
        enabled: true,
        editable: true,
        in_viewport: true,
        hit_target_ok: false,
        box: ElementBox{ x: 0.0, y: 0.0, width: 80.0, height: 30.0 },
        obscured_by: "overlay-blocker",
    };
}

fn fulfillApiRoute(ctx: usize, route: *Route) {
    * = ctx;
    route.fulfill(FulfillOptions{
        status: 201,
        content_type: "application/json",
        body: "{\"ok\":true,\"intercepted\":1}",
        headers_json: "{\"X-Mock\":\"yes\"}",
    }) catch {};
}

fn continueOverrideRoute(ctx: usize, route: *Route) {
    * = ctx;
    route.continueWithOverrides(RouteContinueOverrides{
        url: "https://api.example.com/v2/items",
        method: "PUT",
        headers_json: "{\"X-Auth\":\"token-123\"}",
        post_data: "{\"name\":\"cp\"}",
    }) catch {};
}

struct MockRedirectServer {
    calls: u32 = 0,
    last_method: [16]u8 = undefined,
    last_method_len: usize = 0,
    last_proxy: [64]u8 = undefined,
    last_proxy_len: usize = 0,
    last_ignore_https: bool = false,
}

fn mockRedirectFetch(
    ctx: usize,
    method: []const u8,
    url: []const u8,
    headers_json: []const u8,
    body: []const u8,
    proxy_server: []const u8,
    ignore_https_errors: bool,
): FetchTransportResult {
    * = headers_json;
    * = body;
    const srv = unsafe { ctx as *MockRedirectServer! };
    srv.calls += 1;
    const mn = if (method.len <= 16) method.len else 16;
    @memcpy(srv.last_method[0..mn], method[0..mn]);
    srv.last_method_len = mn;
    const pn = if (proxy_server.len <= 64) proxy_server.len else 64;
    @memcpy(srv.last_proxy[0..pn], proxy_server[0..pn]);
    srv.last_proxy_len = pn;
    srv.last_ignore_https = ignore_https_errors;

    if (std.mem.eql(u8, url, "https://app.example.com/start")) {
        return FetchTransportResult{
            status: 302,
            status_text: "Found",
            headers_json: "{\"Location\":\"/step2\"}",
            body: "",
        };
    }
    if (std.mem.eql(u8, url, "https://app.example.com/step2")) {
        return FetchTransportResult{
            status: 307,
            status_text: "Temporary Redirect",
            headers_json: "{\"Location\":\"https://app.example.com/final\"}",
            body: "",
        };
    }
    if (std.mem.eql(u8, url, "https://app.example.com/loop")) {
        return FetchTransportResult{
            status: 301,
            status_text: "Moved Permanently",
            headers_json: "{\"Location\":\"https://app.example.com/loop\"}",
            body: "",
        };
    }
    return FetchTransportResult{
        status: 200,
        status_text: "OK",
        headers_json: "{\"Content-Type\":\"application/json\"}",
        body: "{\"reached\":\"final\"}",
    };
}

// ----------------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------------

fn testFrameManagerAndActionabilityLoop(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    {
        // SIMD byte finder sanity
        expect_eq(simdFindByteFrame("0123456789abcdef:value", ':').?, 16);
        expect_eq(simdFindByteFrame("clean_frame_identifier_1234", '!') == null, true);

        let fm = FrameManager.init(alloc);
        defer fm.deinit();

        // Main frame navigated via CDP JSON event
        const ok_nav = fm.handleCdpEvent(
            "Page.frameNavigated",
            "{\"frame\":{\"id\":\"main-1\",\"name\":\"root\",\"url\":\"https://app.example.com/\",\"loaderId\":\"L1\"}}",
            1000,
        )?;
        expect_eq(ok_nav, true);
        expect_eq(fm.activeFrameCount(), 1);
        expect_eq(fm.isLifecycleReached("main-1", FrameLifecycleState.Commit), true);

        // Attach nested child & grandchild frames
        expect_eq(fm.handleCdpEvent("Page.frameAttached", "{\"frameId\":\"child-1\",\"parentFrameId\":\"main-1\"}", 1010)?, true);
        expect_eq(fm.handleCdpEvent("Page.frameAttached", "{\"frameId\":\"grand-1\",\"parentFrameId\":\"child-1\"}", 1020)?, true);
        expect_eq(fm.activeFrameCount(), 3);

        // Cross-frame navigation on child-1
        const nav_child = fm.onFrameNavigated("child-1", "main-1", "checkout", "https://pay.example.com/embed", "L2", 1050)?;
        expect_eq(nav_child.navigation_count, 1);
        expect_eq(fm.findFrameByName("checkout") != null, true);
        expect_eq(fm.findFrameByUrl("https://pay.example.com/*") != null, true);

        const kids = fm.childFrameIds(alloc, "main-1")?;
        defer alloc.free(kids);
        expect_eq(kids.len, 1);
        expect_eq(std.mem.eql(u8, kids[0], "child-1"), true);

        // Lifecycle transitions + active request tracking for exact networkidle
        expect_eq(fm.handleCdpEvent("Page.lifecycleEvent", "{\"frameId\":\"main-1\",\"loaderId\":\"L1\",\"name\":\"DOMContentLoaded\"}", 1100)?, true);
        expect_eq(fm.handleCdpEvent("Page.lifecycleEvent", "{\"frameId\":\"main-1\",\"loaderId\":\"L1\",\"name\":\"load\"}", 1200)?, true);
        expect_eq(fm.isLifecycleReached("main-1", FrameLifecycleState.Load), true);

        fm.onRequestStarted("main-1", 1210);
        expect_eq(fm.checkNetworkIdle("main-1", 2000), false);
        fm.onRequestFinished("main-1", 1300);
        // Before 500ms quiet window -> not idle yet
        expect_eq(fm.checkNetworkIdle("main-1", 1600), false);
        // After 500ms quiet window -> exact NetworkIdle reached!
        expect_eq(fm.checkNetworkIdle("main-1", 1850), true);
        expect_eq(fm.isLifecycleReached("main-1", FrameLifecycleState.NetworkIdle), true);

        // Detaching child-1 recursively detaches grand-1
        const detached = fm.onFrameDetached("child-1");
        expect_eq(detached, 2);
        expect_eq(fm.activeFrameCount(), 1);

        // DomActionabilityLoop: full retry loop (detached -> animating -> stable -> scroll -> obscured -> done)
        let sc = ActionabilityScenario{};
        const checks: [4]ActionabilityCheckKind = [
            ActionabilityCheckKind.Visible,
            ActionabilityCheckKind.Stable,
            ActionabilityCheckKind.Enabled,
            ActionabilityCheckKind.Editable,
        ];
        const rep = DomActionabilityLoop.run(
            "#submit-btn",
            &checks,
            ActionabilityOptions{ timeout_ms: 5000, scroll_into_view: true, check_hit_target: true },
            animatingThenStableProbe,
            unsafe { @intFromPtr(&sc) },
        )?;
        expect_eq(rep.state == ActionabilityState.Done, true);
        expect_eq(rep.scrolled, true);
        expect_eq(rep.hit_target_ok, true);
        expect_eq(rep.attempts >= 6, true);

        // Occluded element times out with descriptive error
        const fail_rep = DomActionabilityLoop.run(
            "#blocked-btn",
            &checks,
            ActionabilityOptions{ timeout_ms: 100, scroll_into_view: false, check_hit_target: true },
            alwaysObscuredProbe,
            0,
        )?;
        expect_eq(fail_rep.state == ActionabilityState.Timeout, true);
        expect_eq(std.mem.eql(u8, fail_rep.error_message, "overlay-blocker"), true);
    }

    expect_eq(gpa.deinit() == .ok, true);
}

fn testCrNetworkManager(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    {
        let mock_cdp = EngineMockCdp{};
        let browser = chromium.launch(alloc, .{
            headless: true,
            transport_fn: engineMockCdpSend,
            transport_ctx: unsafe { @intFromPtr(&mock_cdp) },
        })?;
        defer browser.deinit();

        const page = browser.newPage()?;
        let nm = CrNetworkManager.initWithEndpoint(alloc, page.endpoint, page.session_id);
        defer nm.deinit();

        nm.enableInterception()?;
        nm.addRoute("**/api/v1/*", fulfillApiRoute, 0)?;
        nm.addRoute("**/v1/items*", continueOverrideRoute, 0)?;
        nm.setHttpCredentials(HTTPCredentials{
            username: "admin",
            password: "secret",
            origin: "https://secure.example.com",
        })?;

        // 1. Network.requestWillBeSent + redirect chain
        expect_eq(nm.handleCdpEvent(
            "Network.requestWillBeSent",
            "{\"requestId\":\"req-1\",\"frameId\":\"main-1\",\"type\":\"XHR\",\"request\":{\"url\":\"https://api.example.com/old\",\"method\":\"GET\",\"headers\":{}}}",
        )?, true);
        expect_eq(nm.handleCdpEvent(
            "Network.requestWillBeSent",
            "{\"requestId\":\"req-1\",\"frameId\":\"main-1\",\"type\":\"XHR\",\"redirectResponse\":{\"status\":302},\"request\":{\"url\":\"https://api.example.com/api/v1/users\",\"method\":\"GET\",\"headers\":{}}}",
        )?, true);

        const req_ptr = nm.findRequest("req-1").?;
        expect_eq(req_ptr.redirect_count, 1);
        expect_eq(std.mem.eql(u8, req_ptr.redirected_from_id, "req-1#r0"), true);

        // 2. Fetch.requestPaused -> matches fulfillApiRoute -> Fetch.fulfillRequest with Base64 body
        expect_eq(nm.handleCdpEvent(
            "Fetch.requestPaused",
            "{\"requestId\":\"interception-1\",\"networkId\":\"req-1\",\"resourceType\":\"XHR\",\"request\":{\"url\":\"https://api.example.com/api/v1/users\",\"method\":\"GET\",\"headers\":{}}}",
        )?, true);
        expect_eq(nm.fulfilled_count, 1);
        expect_eq(mock_cdp.seen_fulfill, true);
        expect_eq(std.mem.eql(u8, nm.last_dispatched_method, "Fetch.fulfillRequest"), true);
        // Base64 of "{\"ok\":true,\"intercepted\":1}" is "eyJvayI6dHJ1ZSwiaW50ZXJjZXB0ZWQiOjF9"
        expect_eq(std.mem.indexOf(u8, nm.last_dispatched_params, "eyJvayI6dHJ1ZSwiaW50ZXJjZXB0ZWQiOjF9") != null, true);

        // 3. Fetch.requestPaused -> matches continueOverrideRoute -> Fetch.continueRequest with method/header override
        const act2 = nm.onFetchRequestPaused(
            "interception-2",
            "req-2",
            "https://api.example.com/v1/items",
            "POST",
            "Fetch",
            "{}",
            "",
        )?;
        expect_eq(act2 == RouteAction.Continued, true);
        expect_eq(mock_cdp.seen_continue, true);
        expect_eq(std.mem.indexOf(u8, nm.last_dispatched_params, "\"method\":\"PUT\"") != null, true);
        expect_eq(std.mem.indexOf(u8, nm.last_dispatched_params, "X-Auth") != null, true);

        // 4. Fetch.authRequired -> auto ProvideCredentials
        expect_eq(nm.handleCdpEvent(
            "Fetch.authRequired",
            "{\"requestId\":\"auth-1\",\"authChallenge\":{\"source\":\"Server\",\"origin\":\"https://secure.example.com\",\"scheme\":\"Basic\",\"realm\":\"Admin\"}}",
        )?, true);
        expect_eq(nm.auth_challenges_handled, 1);
        expect_eq(mock_cdp.seen_auth, true);
        expect_eq(std.mem.indexOf(u8, nm.last_dispatched_params, "ProvideCredentials") != null, true);

        // 5. ResponseReceived + LoadingFinished + LoadingFailed
        expect_eq(nm.handleCdpEvent(
            "Network.responseReceived",
            "{\"requestId\":\"req-1\",\"response\":{\"status\":201,\"statusText\":\"Created\",\"mimeType\":\"application/json\",\"headers\":{\"x-mock\":\"yes\"}}}",
        )?, true);
        expect_eq(nm.handleCdpEvent("Network.loadingFinished", "{\"requestId\":\"req-1\",\"encodedDataLength\":27}")?, true);
        expect_eq(nm.findRequest("req-1").?.state == CrNetworkRequestState.Finished, true);
        expect_eq(nm.findRequest("req-1").?.encoded_data_length, 27);

        const fail_req = nm.onRequestWillBeSent("req-fail", "main-1", "https://bad.local/x", "GET", "Fetch", "{}", "", 0)?;
        expect_eq(fail_req.state == CrNetworkRequestState.Pending, true);
        expect_eq(nm.handleCdpEvent("Network.loadingFailed", "{\"requestId\":\"req-fail\",\"errorText\":\"net::ERR_NAME_NOT_RESOLVED\",\"canceled\":false}")?, true);
        expect_eq(nm.findRequest("req-fail").?.state == CrNetworkRequestState.Failed, true);
    }

    expect_eq(gpa.deinit() == .ok, true);
}

fn testSnapshotterAndHarTracer(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    {
        expect_eq(std.mem.indexOf(u8, SNAPSHOTTER_INJECTED_SCRIPT, "__pwCaptureSnapshot") != null, true);
        expect_eq(simdHasBinaryBytes("plain ascii json text 1234567890"), false);
        expect_eq(simdHasBinaryBytes("0123456789abcdef\x00binary"), true);

        let mock_cdp = EngineMockCdp{};
        let browser = chromium.launch(alloc, .{
            headless: true,
            transport_fn: engineMockCdpSend,
            transport_ctx: unsafe { @intFromPtr(&mock_cdp) },
        })?;
        defer browser.deinit();

        const ctx = browser.newContext(.{})?;
        const page = ctx.newPage()?;
        const tracing = ctx.tracing();
        tracing.start(.{ name: "snap-trace", snapshots: true })?;

        let snap = Snapshotter.init(alloc);
        defer snap.deinit();

        snap.injectIntoEndpoint(page.endpoint, page.session_id)?;
        expect_eq(snap.injected, true);
        expect_eq(mock_cdp.seen_add_script, true);

        // SHA1 + FNV deduplication table
        const sha1_a = snap.addResource("text/css", "body { color: #111; }")?;
        const sha1_b = snap.addResource("text/css", "body { color: #111; }")?;
        const sha1_c = snap.addResource("image/png", "\x89PNG\r\n\x1a\n")?;
        expect_eq(std.mem.eql(u8, sha1_a[0..40], sha1_b[0..40]), true);
        expect_eq(std.mem.eql(u8, sha1_a[0..40], sha1_c[0..40]), false);
        expect_eq(snap.uniqueResourceCount(), 2);
        expect_eq(snap.deduplicatedHits(), 1);

        const cap_snap = snap.captureSnapshot("page-1", "main-1", "https://app.example.com/", "before@1", "call@1", "[\"HTML\",{},[]]", 1280, 720, 1000)?;
        expect_eq(cap_snap.viewport_width, 1280);
        expect_eq(snap.snapshotCount(), 1);

        const exported = snap.exportToTracing(tracing)?;
        expect_eq(exported, 1);
        expect_eq(tracing.eventCount() >= 2, true);

        const zip_bytes = tracing.stop("")?;
        defer alloc.free(zip_bytes);
        expect_eq(zip_bytes.len > 0, true);

        // HarTracer with url_glob filter, exact HarTimings, and Base64 binary body
        let tracer = HarTracer.init(alloc, .{
            url_glob: "https://api.example.com/*",
            base64_binary_bodies: true,
        });
        defer tracer.deinit();

        const timings = HarTimings.fromResourceTiming(ResourceTiming{
            start_time: 100.0,
            domain_lookup_start: 1.0,
            domain_lookup_end: 6.0,
            connect_start: 6.0,
            connect_end: 20.0,
            secure_connection_start: 12.0,
            response_start: 55.0,
            response_end: 70.0,
        });
        expect_eq(timings.dns, 5.0);
        expect_eq(timings.connect, 14.0);
        expect_eq(timings.ssl, 8.0);
        expect_eq(timings.receive, 15.0);

        // Filtered out URL
        const rec_skip = tracer.recordExchange(HarTracerEntry{
            url: "https://cdn.other.org/logo.png",
            method: "GET",
            status: 200,
        })?;
        expect_eq(rec_skip, false);
        expect_eq(tracer.filtered_out_count, 1);

        // Matching binary exchange -> Base64 encoded
        const rec_ok = tracer.recordExchange(HarTracerEntry{
            url: "https://api.example.com/blob",
            method: "GET",
            status: 200,
            status_text: "OK",
            mime_type: "application/octet-stream",
            request_headers_json: "{\"Accept\":\"*/*\"}",
            response_headers_json: "{\"Content-Type\":\"application/octet-stream\"}",
            response_body: "\x00\x01\x02\x03",
            timings: timings,
        })?;
        expect_eq(rec_ok, true);
        expect_eq(tracer.entryCount(), 1);

        const har_json = tracer.toJson(alloc)?;
        defer alloc.free(har_json);
        expect_eq(std.mem.indexOf(u8, har_json, "\"version\":\"1.2\"") != null, true);
        expect_eq(std.mem.indexOf(u8, har_json, "\"encoding\":\"base64\"") != null, true);
        expect_eq(std.mem.indexOf(u8, har_json, "\"AAECAw==\"") != null, true);
        expect_eq(std.mem.indexOf(u8, har_json, "\"timings\"") != null, true);
    }

    expect_eq(gpa.deinit() == .ok, true);
}

fn testRequestContextUpgrades(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    {
        expect_eq(simdHasHeaderUnsafeBytes("normal_field_name_123456"), false);
        expect_eq(simdHasHeaderUnsafeBytes("bad\r\nheader_injection"), true);

        // 1. ProxySettings bypass & Basic Proxy-Authorization
        const proxy = ProxySettings{
            server: "http://proxy.corp.local:8080",
            bypass: "localhost, .internal.corp",
            username: "bot",
            password: "pw",
        };
        expect_eq(proxy.shouldBypass("localhost"), true);
        expect_eq(proxy.shouldBypass("svc.internal.corp"), true);
        expect_eq(proxy.shouldBypass("app.example.com"), false);
        let auth_buf: [128]u8 = undefined;
        const auth_hdr = proxy.buildProxyAuthHeader(&auth_buf)?;
        expect_eq(std.mem.eql(u8, auth_hdr, "Basic Ym90OnB3"), true);

        // 2. MultipartFormData encoding
        let form = MultipartFormData.initWithBoundary(alloc, 0x123456789abcdef0)?;
        defer form.deinit();
        form.addField("title", "hello")?;
        form.addFile("upload", "note.txt", "text/plain", "cp-multipart-body")?;

        let ct_buf: [128]u8 = undefined;
        const ct_hdr = form.contentTypeHeader(&ct_buf)?;
        expect_eq(std.mem.indexOf(u8, ct_hdr, "multipart/form-data; boundary=----PlaywrightCPBoundary123456789abcdef0") != null, true);

        const encoded = form.encode(alloc)?;
        defer alloc.free(encoded);
        expect_eq(std.mem.indexOf(u8, encoded, "name=\"title\"\r\n\r\nhello\r\n") != null, true);
        expect_eq(std.mem.indexOf(u8, encoded, "filename=\"note.txt\"") != null, true);
        expect_eq(std.mem.indexOf(u8, encoded, "cp-multipart-body") != null, true);

        // 3. APIRequestContext max_redirects chain (302 POST->GET + 307 preserve GET) + Proxy + ignore_https_errors
        let srv = MockRedirectServer{};
        let req_ctx = APIRequestContext.init(alloc, .{
            base_url: "https://app.example.com",
            max_redirects: 5,
            ignore_https_errors: true,
            proxy: proxy,
        });
        defer req_ctx.deinit();
        req_ctx.setTransport(mockRedirectFetch, unsafe { @intFromPtr(&srv) });

        const resp = req_ctx.post("/start", .{
            multipart: &form,
        })?;
        expect_eq(resp.status, 200);
        expect_eq(resp.redirect_count, 2);
        expect_eq(resp.redirect_history.len, 2);
        expect_eq(resp.redirect_history[0].status, 302);
        expect_eq(std.mem.eql(u8, resp.redirect_history[0].method, "GET"), true);
        expect_eq(resp.redirect_history[1].status, 307);
        expect_eq(std.mem.eql(u8, resp.url, "https://app.example.com/final"), true);
        expect_eq(std.mem.eql(u8, resp.proxy_used, "http://proxy.corp.local:8080"), true);
        expect_eq(resp.tls_verify_disabled, true);
        expect_eq(srv.last_ignore_https, true);

        // 4. Exceeding max_redirects returns PlaywrightError.ProtocolError
        let hit_limit = false;
        const loop_res = req_ctx.get("/loop", .{ max_redirects: 2 }) catch |e| blk: {
            if (e == PlaywrightError.ProtocolError) hit_limit = true;
            break :blk APIResponse{};
        };
        expect_eq(loop_res.redirect_count, 0);
        expect_eq(hit_limit, true);
    }

    expect_eq(gpa.deinit() == .ok, true);
}

fn main() {
    test("FrameManager lifecycle, networkidle & DomActionabilityLoop", () => {
        testFrameManagerAndActionabilityLoop() catch unreachable;
    });
    test("CrNetworkManager CDP interception, Base64 fulfill, overrides & auth", () => {
        testCrNetworkManager() catch unreachable;
    });
    test("Snapshotter SHA1/FNV dedup, Tracing export & HarTracer 1.2", () => {
        testSnapshotterAndHarTracer() catch unreachable;
    });
    test("APIRequestContext max_redirects, MultipartFormData & ProxySettings", () => {
        testRequestContextUpgrades() catch unreachable;
    });
}
