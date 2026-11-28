// ============================================================================
// cap/test/testCapPuppeteerManagers.cp — Production Event State Machine Tests
// Covers NetworkManager, FrameManager, ExecutionContext/IsolatedWorld,
// TargetManager & AccessibilityTreeBuilder with DebugAllocator zero-leak check.
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CdpEndpoint,
    CdpEvent,
    CdpMessage,
    CdpFrameScanner,
    AXNodeSummary,
} from "cap:cdp";
import {
    PuppeteerError,
    EvaluateResult,
    WaitUntil,
    ResourceType,
    HttpHeader,
    NetworkConditions,
    Credentials,
    ContinueRequestOverrides,
    CDPSession,
    HTTPRequest,
    InterceptResolutionAction,
    NetworkSimdScanner,
    InterceptDecision,
    NetworkManager,
    FrameSimdScanner,
    ExecutionContext,
    IsolatedWorld,
    FrameManager,
    TargetSimdScanner,
    ManagedTargetKind,
    TargetManager,
    SerializedAXNode,
    AccessibilityTreeBuilder,
} from "../../packages/puppeteer/src/index.cp";

struct MockTransport {
    endpoint: (*CdpEndpoint)? = null,
    send_count: usize = 0,
    last_method_buf: [128]u8 = undefined,
    last_method_len: usize = 0,
    last_payload_buf: [2048]u8 = undefined,
    last_payload_len: usize = 0,

    fn lastMethod(): []const u8 {
        return _.last_method_buf[0.._.last_method_len];
    }

    fn lastPayload(): []const u8 {
        return _.last_payload_buf[0.._.last_payload_len];
    }

    fn onSend(*_, payload: []const u8): bool {
        _.send_count += 1;
        const plen = if (payload.len <= 2048) payload.len else 2048;
        if (plen > 0) @memcpy(_.last_payload_buf[0..plen], payload[0..plen]);
        _.last_payload_len = plen;

        const id_str = CdpFrameScanner.extractTopLevelField(payload, "id", false) orelse "1";
        const method = CdpFrameScanner.extractTopLevelField(payload, "method", true) orelse "";
        const mlen = if (method.len <= 128) method.len else 128;
        if (mlen > 0) @memcpy(_.last_method_buf[0..mlen], method[0..mlen]);
        _.last_method_len = mlen;

        if (ep := _.endpoint) {
            let rbuf: [1024]u8 = undefined;
            let result_json: []const u8 = "{}";
            if (std.mem.eql(u8, method, "Runtime.evaluate")) {
                if (std.mem.indexOf(u8, payload, "throw") != null) {
                    result_json = "{\"exceptionDetails\":{\"text\":\"ReferenceError: boom\"}}";
                } else {
                    result_json = "{\"result\":{\"type\":\"string\",\"value\":\"ctx-eval-ok\"}}";
                }
            }
            const resp = std.fmt.bufPrint(&rbuf, "{{\"id\":{s},\"result\":{s}}}", .{ id_str, result_json }) catch return true;
            const _ign_msg = ep.handleIncomingFrame(resp) catch CdpMessage{};
            * = _ign_msg;
        }
        return true;
    }
}

fn mockSendCallback(ctx: usize, payload: []const u8): bool {
    unsafe {
        const srv: *MockTransport! = ctx as *MockTransport!;
        return srv.onSend(payload);
    }
}

struct EventCounter {
    count: usize = 0,
    last_len: usize = 0,
}

fn countEventHandler(event_name: []const u8, payload: []const u8, ctx: usize) {
    const _ign_ev = event_name;
    * = _ign_ev;
    unsafe {
        const counter: *EventCounter! = ctx as *EventCounter!;
        counter.count += 1;
        counter.last_len = payload.len;
    }
}

fn interceptorContinueLow(req: *HTTPRequest, ctx: usize): InterceptDecision {
    const _ign_r = req;
    const _ign_c = ctx;
    * = _ign_r;
    * = _ign_c;
    return InterceptDecision.continueWith(ContinueRequestOverrides{ method: "POST" }, 10);
}

fn interceptorRespondMid(req: *HTTPRequest, ctx: usize): InterceptDecision {
    const _ign_r = req;
    const _ign_c = ctx;
    * = _ign_r;
    * = _ign_c;
    return InterceptDecision.respond(200, "application/json", "{\"ok\":true}", 50);
}

fn interceptorAbortEqual(req: *HTTPRequest, ctx: usize): InterceptDecision {
    const _ign_r = req;
    const _ign_c = ctx;
    * = _ign_r;
    * = _ign_c;
    return InterceptDecision.abort("BlockedByClient", 50);
}

fn interceptorRespondHigh(req: *HTTPRequest, ctx: usize): InterceptDecision {
    const _ign_r = req;
    const _ign_c = ctx;
    * = _ign_r;
    * = _ign_c;
    return InterceptDecision.respond(201, "text/plain", "high-prio", 100);
}

fn main() {
    test("1. NetworkManager: lifecycle, redirect chain, cache, failure & DebugAllocator zero-leak", () => {
        let gpa: std.heap.DebugAllocator(.{}) = .{};
        {
            const alloc: std.mem.Allocator = gpa.allocator();
            let ep = CdpEndpoint.init(alloc);

            let srv = MockTransport{ endpoint: *ep };
            let srv_addr: usize = 0;
            unsafe {
                const p: *MockTransport! = *srv!;
                srv_addr = p as usize;
            }
            ep.setTransport(mockSendCallback, srv_addr);
            ep.markConnected();

            let sess = CDPSession.init(*ep, "sess-net-1", "page", "target-1", "");
            let nm = NetworkManager.init(alloc, *sess);

            let req_counter = EventCounter{};
            let resp_counter = EventCounter{};
            let rc_addr: usize = 0;
            let rsc_addr: usize = 0;
            unsafe {
                const p1: *EventCounter! = *req_counter!;
                const p2: *EventCounter! = *resp_counter!;
                rc_addr = p1 as usize;
                rsc_addr = p2 as usize;
            }
            expect_eq(nm.on("request", countEventHandler, rc_addr), true);
            expect_eq(nm.on("response", countEventHandler, rsc_addr), true);

            // 1a. Initial requestWillBeSent
            nm.handleCdpEvent(CdpEvent{
                method: "Network.requestWillBeSent",
                params_json: "{\"requestId\":\"req-1\",\"loaderId\":\"req-1\",\"frameId\":\"frame-main\",\"type\":\"Document\",\"request\":{\"url\":\"https://example.com/old\",\"method\":\"GET\"}}",
            }) catch unreachable;
            expect_eq(nm.requestCount(), 1 as usize);
            expect_eq(nm.inFlightCount(), 1 as usize);
            expect_eq(req_counter.count, 1 as usize);

            // 1b. Redirect via requestWillBeSent with redirectResponse
            nm.handleCdpEvent(CdpEvent{
                method: "Network.requestWillBeSent",
                params_json: "{\"requestId\":\"req-1\",\"frameId\":\"frame-main\",\"type\":\"Document\",\"request\":{\"url\":\"https://example.com/final\",\"method\":\"GET\"},\"redirectResponse\":{\"url\":\"https://example.com/old\",\"status\":302,\"statusText\":\"Found\"}}",
            }) catch unreachable;

            if (req := nm.getRequestById("req-1")) {
                expect_eq(std.mem.eql(u8, req.url(), "https://example.com/final"), true);
                expect_eq(req.redirectChain().len, 1 as usize);
                expect_eq(std.mem.eql(u8, req.redirectChain()[0], "https://example.com/old"), true);
                expect_eq(req.resourceType() == ResourceType.Document, true);
                expect_eq(req.isNavigationRequest(), true);
            } else {
                expect_eq(false, true);
            }

            // 1c. Response received + bidirectional link + security details
            nm.handleCdpEvent(CdpEvent{
                method: "Network.responseReceived",
                params_json: "{\"requestId\":\"req-1\",\"response\":{\"url\":\"https://example.com/final\",\"status\":200,\"statusText\":\"OK\",\"remoteIPAddress\":\"93.184.216.34\",\"remotePort\":443,\"encodedDataLength\":512,\"securityDetails\":{\"protocol\":\"TLS 1.3\",\"subjectName\":\"example.com\",\"issuer\":\"DigiCert\"}}}",
            }) catch unreachable;

            if (resp := nm.getResponseById("req-1")) {
                expect_eq(resp.status(), 200 as u16);
                expect_eq(std.mem.eql(u8, resp.remoteAddress().ip, "93.184.216.34"), true);
                expect_eq(resp.remoteAddress().port, 443 as u16);
                expect_eq(resp.request() != null, true);
                expect_eq(resp.securityDetails() != null, true);
            } else {
                expect_eq(false, true);
            }

            // 1d. Served from cache + loading finished
            nm.handleCdpEvent(CdpEvent{
                method: "Network.requestServedFromCache",
                params_json: "{\"requestId\":\"req-1\"}",
            }) catch unreachable;
            nm.handleCdpEvent(CdpEvent{
                method: "Network.loadingFinished",
                params_json: "{\"requestId\":\"req-1\",\"encodedDataLength\":1024}",
            }) catch unreachable;
            expect_eq(nm.inFlightCount(), 0 as usize);
            if (resp := nm.getResponseById("req-1")) {
                expect_eq(resp.fromCache(), true);
                expect_eq(resp.encodedDataLength(), 1024 as usize);
            }

            // 1e. Second request that fails
            nm.handleCdpEvent(CdpEvent{
                method: "Network.requestWillBeSent",
                params_json: "{\"requestId\":\"req-2\",\"frameId\":\"frame-main\",\"type\":\"XHR\",\"request\":{\"url\":\"https://api.example.com/fail\",\"method\":\"POST\",\"postData\":\"hello\"}}",
            }) catch unreachable;
            expect_eq(nm.inFlightCount(), 1 as usize);
            nm.handleCdpEvent(CdpEvent{
                method: "Network.loadingFailed",
                params_json: "{\"requestId\":\"req-2\",\"errorText\":\"net::ERR_CONNECTION_REFUSED\"}",
            }) catch unreachable;
            expect_eq(nm.inFlightCount(), 0 as usize);
            if (req2 := nm.getRequestById("req-2")) {
                expect_eq(req2.hasPostData(), true);
                if (fail := req2.failure()) {
                    expect_eq(std.mem.eql(u8, fail, "net::ERR_CONNECTION_REFUSED"), true);
                } else {
                    expect_eq(false, true);
                }
            }

            // 1f. 16-lane SIMD scanner check
            expect_eq(NetworkSimdScanner.eqlIgnoreCaseSimd("Content-Type-Header-Value", "content-type-header-value"), true);
            expect_eq(NetworkSimdScanner.indexOfByteSimd("https://example.com/path?a=1", '?') != null, true);
        }
            nm.deinit();
            ep.deinit();
        expect_eq(gpa.deinit() == .ok, true);
    });

    test("2. NetworkManager: multi-interceptor priority arbitration, Fetch auth, headers & offline", () => {
        let gpa: std.heap.DebugAllocator(.{}) = .{};
        {
            const alloc: std.mem.Allocator = gpa.allocator();
            let ep = CdpEndpoint.init(alloc);

            let srv = MockTransport{ endpoint: *ep };
            let srv_addr: usize = 0;
            unsafe {
                const p: *MockTransport! = *srv!;
                srv_addr = p as usize;
            }
            ep.setTransport(mockSendCallback, srv_addr);
            ep.markConnected();
            ep.createSession("sess-net-2", "page", "target-2", "") catch unreachable;

            let sess = CDPSession.init(*ep, "sess-net-2", "page", "target-2", "");
            let nm = NetworkManager.init(alloc, *sess);

            // Enable interception & extra headers & offline/cache
            nm.setRequestInterception(true) catch unreachable;
            const hdrs: [2]HttpHeader = [
                HttpHeader{ name: "X-Agent", value: "CP-Puppeteer" },
                HttpHeader{ name: "Accept-Language", value: "en-US" },
            ];
            nm.setExtraHTTPHeaders(&hdrs) catch unreachable;
            expect_eq(nm.extraHTTPHeaders().len, 2 as usize);
            expect_eq(std.mem.eql(u8, srv.lastMethod(), "Network.setExtraHTTPHeaders"), true);

            nm.setOfflineMode(true) catch unreachable;
            expect_eq(nm.isOffline(), true);
            nm.emulateNetworkConditions(NetworkConditions{ offline: false, latency: 25.0, download_throughput: 100000.0, upload_throughput: 50000.0 }) catch unreachable;
            expect_eq(nm.isOffline(), false);
            nm.setCacheEnabled(false) catch unreachable;
            expect_eq(nm.isCacheEnabled(), false);

            // Multi-interceptor arbitration: Continue(10) vs Respond(50) vs Abort(50) => Abort wins at equal priority!
            expect_eq(nm.addInterceptor(interceptorContinueLow, 10, 0), true);
            expect_eq(nm.addInterceptor(interceptorRespondMid, 50, 0), true);
            expect_eq(nm.addInterceptor(interceptorAbortEqual, 50, 0), true);

            nm.handleCdpEvent(CdpEvent{
                method: "Fetch.requestPaused",
                params_json: "{\"requestId\":\"interception-1\",\"networkId\":\"net-10\",\"resourceType\":\"Script\",\"request\":{\"url\":\"https://ads.example.com/track.js\",\"method\":\"GET\"}}",
            }) catch unreachable;
            expect_eq(std.mem.eql(u8, srv.lastMethod(), "Fetch.failRequest"), true);
            if (req := nm.getRequestById("net-10")) {
                expect_eq(req.interceptResolutionState().action == InterceptResolutionAction.Abort, true);
            }

            // Now add Respond(100) => beats Abort(50)
            expect_eq(nm.addInterceptor(interceptorRespondHigh, 100, 0), true);
            nm.handleCdpEvent(CdpEvent{
                method: "Fetch.requestPaused",
                params_json: "{\"requestId\":\"interception-2\",\"networkId\":\"net-11\",\"resourceType\":\"Fetch\",\"request\":{\"url\":\"https://api.example.com/data\",\"method\":\"GET\"}}",
            }) catch unreachable;
            expect_eq(std.mem.eql(u8, srv.lastMethod(), "Fetch.fulfillRequest"), true);
            if (req := nm.getRequestById("net-11")) {
                expect_eq(req.interceptResolutionState().action == InterceptResolutionAction.Respond, true);
            }

            // Authenticate + Fetch.authRequired (1st attempt -> ProvideCredentials, 2nd attempt -> CancelAuth)
            nm.authenticate(Credentials{ username: "alice", password: "secret" }) catch unreachable;
            nm.handleCdpEvent(CdpEvent{
                method: "Fetch.authRequired",
                params_json: "{\"requestId\":\"net-11\",\"authChallenge\":{\"source\":\"Server\",\"origin\":\"https://api.example.com\",\"scheme\":\"Basic\"}}",
            }) catch unreachable;
            expect_eq(std.mem.eql(u8, srv.lastMethod(), "Fetch.continueWithAuth"), true);
            expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "ProvideCredentials") != null, true);

            nm.handleCdpEvent(CdpEvent{
                method: "Fetch.authRequired",
                params_json: "{\"requestId\":\"net-11\",\"authChallenge\":{\"source\":\"Server\"}}",
            }) catch unreachable;
            expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "CancelAuth") != null, true);
        }
            nm.deinit();
            ep.deinit();
        expect_eq(gpa.deinit() == .ok, true);
    });

    test("3. FrameManager, ExecutionContext & IsolatedWorld: frame tree, lifecycle & safe evaluate", () => {
        let gpa: std.heap.DebugAllocator(.{}) = .{};
        {
            const alloc: std.mem.Allocator = gpa.allocator();
            let ep = CdpEndpoint.init(alloc);

            let srv = MockTransport{ endpoint: *ep };
            let srv_addr: usize = 0;
            unsafe {
                const p: *MockTransport! = *srv!;
                srv_addr = p as usize;
            }
            ep.setTransport(mockSendCallback, srv_addr);
            ep.markConnected();
            ep.createSession("sess-frame-1", "page", "target-frame", "") catch unreachable;

            let sess = CDPSession.init(*ep, "sess-frame-1", "page", "target-frame", "");
            let fm = FrameManager.init(alloc, *sess);

            // Main frame navigated
            fm.handleCdpEvent(CdpEvent{
                method: "Page.frameNavigated",
                params_json: "{\"frame\":{\"id\":\"F-MAIN\",\"loaderId\":\"L-1\",\"url\":\"https://app.example.com/home\",\"name\":\"\"}}",
            }) catch unreachable;
            expect_eq(fm.frameCount(), 1 as usize);
            if (mf := fm.mainFrame()) {
                expect_eq(std.mem.eql(u8, mf.id(), "F-MAIN"), true);
                expect_eq(std.mem.eql(u8, mf.url(), "https://app.example.com/home"), true);
            } else {
                expect_eq(false, true);
            }

            // Attach child iframe and grandchild iframe
            fm.handleCdpEvent(CdpEvent{
                method: "Page.frameAttached",
                params_json: "{\"frameId\":\"F-CHILD-1\",\"parentFrameId\":\"F-MAIN\"}",
            }) catch unreachable;
            fm.handleCdpEvent(CdpEvent{
                method: "Page.frameAttached",
                params_json: "{\"frameId\":\"F-GRAND-1\",\"parentFrameId\":\"F-CHILD-1\"}",
            }) catch unreachable;
            expect_eq(fm.frameCount(), 3 as usize);
            expect_eq(fm.activeFrameCount(), 3 as usize);
            if (mf := fm.mainFrame()) {
                expect_eq(mf.frame.childFrames().len, 1 as usize);
            }

            // ExecutionContextCreated (main world + isolated utility world)
            fm.handleCdpEvent(CdpEvent{
                method: "Runtime.executionContextCreated",
                params_json: "{\"context\":{\"id\":101,\"origin\":\"https://app.example.com\",\"name\":\"\",\"uniqueId\":\"u-101\",\"auxData\":{\"frameId\":\"F-MAIN\",\"isDefault\":true}}}",
            }) catch unreachable;
            fm.handleCdpEvent(CdpEvent{
                method: "Runtime.executionContextCreated",
                params_json: "{\"context\":{\"id\":102,\"origin\":\"https://app.example.com\",\"name\":\"__puppeteer_utility_world__\",\"uniqueId\":\"u-102\",\"auxData\":{\"frameId\":\"F-MAIN\",\"isDefault\":false}}}",
            }) catch unreachable;
            expect_eq(fm.contextCount(), 2 as usize);
            expect_eq(fm.activeContextCount(), 2 as usize);

            // Safe evaluate in main world & isolated world
            let eval_buf: [256]u8 = undefined;
            if (mf := fm.mainFrame()) {
                expect_eq(mf.main_world.hasContext(), true);
                expect_eq(mf.puppeteer_world.hasContext(), true);
                const r1: EvaluateResult = mf.main_world.evaluate("1 + 1", &eval_buf) catch unreachable;
                expect_eq(r1.has_exception, false);
                expect_eq(std.mem.eql(u8, r1.value_json, "ctx-eval-ok"), true);
                expect_eq(std.mem.indexOf(u8, srv.lastPayload(), "\"contextId\":101") != null, true);
            }

            // SPA hash navigation & lifecycle events
            fm.handleCdpEvent(CdpEvent{
                method: "Page.navigatedWithinDocument",
                params_json: "{\"frameId\":\"F-MAIN\",\"url\":\"https://app.example.com/home#section-2\"}",
            }) catch unreachable;
            if (mf := fm.mainFrame()) {
                expect_eq(std.mem.eql(u8, FrameSimdScanner.stripFragment(mf.url()), "https://app.example.com/home"), true);
            }

            fm.handleCdpEvent(CdpEvent{
                method: "Page.lifecycleEvent",
                params_json: "{\"frameId\":\"F-MAIN\",\"loaderId\":\"L-1\",\"name\":\"DOMContentLoaded\"}",
            }) catch unreachable;
            expect_eq(fm.isLifecycleSatisfied("F-MAIN", WaitUntil.DomContentLoaded), true);
            expect_eq(fm.isLifecycleSatisfied("F-MAIN", WaitUntil.Load), false);

            fm.handleCdpEvent(CdpEvent{
                method: "Page.lifecycleEvent",
                params_json: "{\"frameId\":\"F-MAIN\",\"loaderId\":\"L-1\",\"name\":\"load\"}",
            }) catch unreachable;
            fm.handleCdpEvent(CdpEvent{
                method: "Page.lifecycleEvent",
                params_json: "{\"frameId\":\"F-MAIN\",\"loaderId\":\"L-1\",\"name\":\"networkIdle\"}",
            }) catch unreachable;
            expect_eq(fm.isLifecycleSatisfied("F-MAIN", WaitUntil.Load), true);
            expect_eq(fm.isLifecycleSatisfied("F-MAIN", WaitUntil.NetworkIdle0), true);
            expect_eq(fm.isLifecycleSatisfied("F-MAIN", WaitUntil.NetworkIdle2), true);

            // Detach F-CHILD-1 -> recursively detaches F-GRAND-1 too
            fm.handleCdpEvent(CdpEvent{
                method: "Page.frameDetached",
                params_json: "{\"frameId\":\"F-CHILD-1\",\"reason\":\"remove\"}",
            }) catch unreachable;
            expect_eq(fm.activeFrameCount(), 1 as usize);

            // Destroy context 101 & clear all contexts
            fm.handleCdpEvent(CdpEvent{
                method: "Runtime.executionContextDestroyed",
                params_json: "{\"executionContextId\":101}",
            }) catch unreachable;
            expect_eq(fm.activeContextCount(), 1 as usize);

            fm.handleCdpEvent(CdpEvent{
                method: "Runtime.executionContextsCleared",
                params_json: "{}",
            }) catch unreachable;
            expect_eq(fm.activeContextCount(), 0 as usize);
        }
            fm.deinit();
            ep.deinit();
        expect_eq(gpa.deinit() == .ok, true);
    });

    test("4. TargetManager: setAutoAttach, Page / Worker / ServiceWorker / OOPIF iframe tracking", () => {
        let gpa: std.heap.DebugAllocator(.{}) = .{};
        {
            const alloc: std.mem.Allocator = gpa.allocator();
            let ep = CdpEndpoint.init(alloc);

            let srv = MockTransport{ endpoint: *ep };
            let srv_addr: usize = 0;
            unsafe {
                const p: *MockTransport! = *srv!;
                srv_addr = p as usize;
            }
            ep.setTransport(mockSendCallback, srv_addr);
            ep.markConnected();

            let sess = CDPSession.init(*ep, "", "browser", "browser", "");
            let tm = TargetManager.init(alloc, *sess);

            tm.setAutoAttach(true, false, true) catch unreachable;
            expect_eq(tm.isAutoAttachEnabled(), true);
            expect_eq(std.mem.eql(u8, srv.lastMethod(), "Target.setAutoAttach"), true);

            tm.setDiscoverTargets(true) catch unreachable;
            expect_eq(std.mem.eql(u8, srv.lastMethod(), "Target.setDiscoverTargets"), true);

            // Replay targetCreated & attachedToTarget for Page, OOPIF iframe, Worker, ServiceWorker
            tm.handleCdpEvent(CdpEvent{
                method: "Target.targetCreated",
                params_json: "{\"targetInfo\":{\"targetId\":\"T-PAGE\",\"type\":\"page\",\"title\":\"Home\",\"url\":\"https://example.com\",\"attached\":false}}",
            }) catch unreachable;
            tm.handleCdpEvent(CdpEvent{
                method: "Target.attachedToTarget",
                params_json: "{\"sessionId\":\"S-PAGE\",\"targetInfo\":{\"targetId\":\"T-PAGE\",\"type\":\"page\",\"title\":\"Home\",\"url\":\"https://example.com\",\"attached\":true},\"waitingForDebugger\":false}",
            }) catch unreachable;
            tm.handleCdpEvent(CdpEvent{
                method: "Target.attachedToTarget",
                params_json: "{\"sessionId\":\"S-OOPIF\",\"targetInfo\":{\"targetId\":\"T-OOPIF\",\"type\":\"iframe\",\"title\":\"CrossOrigin\",\"url\":\"https://cdn.other.com/frame\",\"attached\":true},\"waitingForDebugger\":false}",
            }) catch unreachable;
            tm.handleCdpEvent(CdpEvent{
                method: "Target.attachedToTarget",
                params_json: "{\"sessionId\":\"S-WORKER\",\"targetInfo\":{\"targetId\":\"T-WORKER\",\"type\":\"worker\",\"title\":\"Worker\",\"url\":\"https://example.com/w.js\",\"attached\":true},\"waitingForDebugger\":false}",
            }) catch unreachable;
            tm.handleCdpEvent(CdpEvent{
                method: "Target.attachedToTarget",
                params_json: "{\"sessionId\":\"S-SW\",\"targetInfo\":{\"targetId\":\"T-SW\",\"type\":\"service_worker\",\"title\":\"SW\",\"url\":\"https://example.com/sw.js\",\"attached\":true},\"waitingForDebugger\":false}",
            }) catch unreachable;

            expect_eq(tm.targetCount(), 4 as usize);
            expect_eq(tm.attachedCount(), 4 as usize);
            expect_eq(tm.countByKind(ManagedTargetKind.Page), 1 as usize);
            expect_eq(tm.countByKind(ManagedTargetKind.Iframe), 1 as usize);
            expect_eq(tm.countByKind(ManagedTargetKind.Worker), 1 as usize);
            expect_eq(tm.countByKind(ManagedTargetKind.ServiceWorker), 1 as usize);

            if (oopif := tm.targetBySessionId("S-OOPIF")) {
                expect_eq(oopif.isOopif(), true);
                expect_eq(std.mem.eql(u8, oopif.id(), "T-OOPIF"), true);
            } else {
                expect_eq(false, true);
            }

            // Target info changed
            tm.handleCdpEvent(CdpEvent{
                method: "Target.targetInfoChanged",
                params_json: "{\"targetInfo\":{\"targetId\":\"T-PAGE\",\"type\":\"page\",\"title\":\"Dashboard\",\"url\":\"https://example.com/dash\",\"attached\":true}}",
            }) catch unreachable;
            if (pg := tm.targetById("T-PAGE")) {
                expect_eq(std.mem.eql(u8, pg.info.title, "Dashboard"), true);
            }

            // Detach & destroy worker
            tm.handleCdpEvent(CdpEvent{
                method: "Target.detachedFromTarget",
                params_json: "{\"sessionId\":\"S-WORKER\",\"targetId\":\"T-WORKER\"}",
            }) catch unreachable;
            expect_eq(tm.attachedCount(), 3 as usize);

            tm.handleCdpEvent(CdpEvent{
                method: "Target.targetDestroyed",
                params_json: "{\"targetId\":\"T-WORKER\"}",
            }) catch unreachable;
            expect_eq(tm.activeTargetCount(), 3 as usize);
            expect_eq(tm.countByKind(ManagedTargetKind.Worker), 0 as usize);
        }
            tm.deinit();
            ep.deinit();
        expect_eq(gpa.deinit() == .ok, true);
    });

    test("5. AccessibilityTreeBuilder: recursive SerializedAXNode tree, interestingOnly & ARIA normalization", () => {
        let gpa: std.heap.DebugAllocator(.{}) = .{};
        {
            const alloc: std.mem.Allocator = gpa.allocator();
            let builder = AccessibilityTreeBuilder.init(alloc);

            const ax_nodes_json =
                "[" ++
                "{\"nodeId\":\"1\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"RootWebArea\"},\"name\":{\"type\":\"computedString\",\"value\":\"Demo Page\"},\"childIds\":[\"2\",\"6\"],\"backendDOMNodeId\":10}," ++
                "{\"nodeId\":\"2\",\"parentId\":\"1\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"GenericContainer\"},\"name\":{\"type\":\"computedString\",\"value\":\"\"},\"childIds\":[\"3\",\"4\",\"5\"],\"backendDOMNodeId\":11}," ++
                "{\"nodeId\":\"3\",\"parentId\":\"2\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"Heading\"},\"name\":{\"type\":\"computedString\",\"value\":\"Welcome\"},\"properties\":[{\"name\":\"level\",\"value\":{\"type\":\"integer\",\"value\":1}}],\"backendDOMNodeId\":12}," ++
                "{\"nodeId\":\"4\",\"parentId\":\"2\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"Button\"},\"name\":{\"type\":\"computedString\",\"value\":\"Submit\"},\"properties\":[{\"name\":\"focused\",\"value\":{\"type\":\"boolean\",\"value\":true}},{\"name\":\"disabled\",\"value\":{\"type\":\"boolean\",\"value\":false}}],\"backendDOMNodeId\":13}," ++
                "{\"nodeId\":\"5\",\"parentId\":\"2\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"CheckBox\"},\"name\":{\"type\":\"computedString\",\"value\":\"Accept\"},\"properties\":[{\"name\":\"checked\",\"value\":{\"type\":\"tristate\",\"value\":\"true\"}}],\"backendDOMNodeId\":14}," ++
                "{\"nodeId\":\"6\",\"parentId\":\"1\",\"ignored\":true,\"role\":{\"type\":\"role\",\"value\":\"GenericContainer\"},\"name\":{\"type\":\"computedString\",\"value\":\"Hidden\"},\"backendDOMNodeId\":15}" ++
                "]";

            const parsed = builder.parseAXNodesJson(ax_nodes_json) catch unreachable;
            expect_eq(parsed, 6 as usize);

            // interestingOnly = true: GenericContainer(2) flattened, ignored(6) dropped
            const interesting_tree = (builder.buildTree(true) catch unreachable).?;

            expect_eq(std.mem.eql(u8, interesting_tree.role, "WebArea"), true);
            expect_eq(std.mem.eql(u8, interesting_tree.name, "Demo Page"), true);
            expect_eq(interesting_tree.childSlice().len, 3 as usize);
            expect_eq(AccessibilityTreeBuilder.countTreeNodes(interesting_tree), 4 as usize);

            const h1 = interesting_tree.childSlice()[0];
            expect_eq(std.mem.eql(u8, h1.role, "heading"), true);
            expect_eq(h1.has_level, true);
            expect_eq(h1.level, 1 as i32);

            const btn = interesting_tree.childSlice()[1];
            expect_eq(std.mem.eql(u8, btn.role, "button"), true);
            expect_eq(btn.focused, true);
            expect_eq(btn.disabled, false);

            const cb = interesting_tree.childSlice()[2];
            expect_eq(std.mem.eql(u8, cb.role, "checkbox"), true);
            expect_eq(std.mem.eql(u8, cb.checked, "true"), true);

            // interestingOnly = false: retains all 6 nodes
            const full_tree = (builder.buildTree(false) catch unreachable).?;
            expect_eq(AccessibilityTreeBuilder.countTreeNodes(full_tree), 6 as usize);

            // Direct AXNodeSummary builder + SIMD whitespace helper
            let b2 = AccessibilityTreeBuilder.init(alloc);
            b2.addSummaryNode(AXNodeSummary{ node_id: "r1", role: "RootWebArea", name: "App", ignored: false }, "") catch unreachable;
            b2.addSummaryNode(AXNodeSummary{ node_id: "c1", role: "Link", name: "Docs", ignored: false }, "r1") catch unreachable;
            const t2 = (b2.buildTree(true) catch unreachable).?;
            expect_eq(AccessibilityTreeBuilder.countTreeNodes(t2), 2 as usize);
            expect_eq(TargetSimdScanner.hasNonWhitespaceSimd("                \t\n "), false);
            expect_eq(TargetSimdScanner.hasNonWhitespaceSimd("                x"), true);
        }
            AccessibilityTreeBuilder.destroyTree(alloc, t2);
            b2.deinit();
            AccessibilityTreeBuilder.destroyTree(alloc, full_tree);
            AccessibilityTreeBuilder.destroyTree(alloc, interesting_tree);
            builder.deinit();
        expect_eq(gpa.deinit() == .ok, true);
    });
}
