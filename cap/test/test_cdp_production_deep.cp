// Source: cp/cap/test/test_cdp_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:cdp & cap.cdp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CdpError,
    CdpMessageKind,
    CdpRequest,
    CdpResponse,
    CdpEvent,
    CdpMessage,
    CdpFrameScanner,
    CdpConnectionState,
    CdpSession,
    CdpEndpoint,
    CdpBrowserLaunchOptions,
    CdpWsEndpoint,
    CdpBrowserLauncher,
    PageNavigateParams,
    PageCaptureScreenshotParams,
    PagePrintToPdfParams,
    PageDomain,
    RuntimeEvaluateParams,
    RuntimeDomain,
    DomGetDocumentParams,
    DomQuerySelectorParams,
    DomDomain,
    InputMouseEventParams,
    InputKeyEventParams,
    InputDomain,
    NetworkEnableParams,
    NetworkDomain,
    AccessibilityGetFullAXTreeParams,
    AXNodeSummary,
    AccessibilityDomain,
} from "cap:cdp";

struct MockTransport {
    last_len: usize = 0,
    send_count: usize = 0,
    should_fail: bool = false,
    last_buf: [1024]u8 = undefined,

    fn record(*_, payload: []const u8): bool {
        if (_.should_fail) return false;
        const copy_len = if (payload.len <= 1024) payload.len else 1024;
        @memcpy(_.last_buf[0..copy_len], payload[0..copy_len]);
        _.last_len = copy_len;
        _.send_count += 1;
        return true;
    }

    fn lastPayload(): []const u8 {
        return _.last_buf[0.._.last_len];
    }
}

fn mockSendFn(ctx: usize, payload: []const u8): bool {
    const mt = unsafe { ctx as *MockTransport! };
    return mt.record(payload);
}

struct EventCollector {
    count: usize = 0,
    last_method: [128]u8 = undefined,
    last_method_len: usize = 0,
    last_session: [128]u8 = undefined,
    last_session_len: usize = 0,

    fn push(*_, ev: CdpEvent) {
        _.count += 1;
        const mlen = if (ev.method.len <= 128) ev.method.len else 128;
        @memcpy(_.last_method[0..mlen], ev.method[0..mlen]);
        _.last_method_len = mlen;

        const slen = if (ev.session_id.len <= 128) ev.session_id.len else 128;
        @memcpy(_.last_session[0..slen], ev.session_id[0..slen]);
        _.last_session_len = slen;
    }
}

fn onEventCollect(ctx: usize, ev: CdpEvent) {
    const ec = unsafe { ctx as *EventCollector! };
    ec.push(ev);
}

fn testRequestEncoding(): !void {
    let buf: [512]u8 = undefined;
    const req1 = CdpRequest{
        id: 1,
        method: "Browser.getVersion",
        params_json: "{}",
        session_id: "",
    };
    const n1 = CdpFrameScanner.encodeRequest(req1, &buf)?;
    const s1 = buf[0..n1];
    expect_eq(std.mem.eql(u8, s1, "{\"id\":1,\"method\":\"Browser.getVersion\",\"params\":{}}"), true);

    const req2 = CdpRequest{
        id: 42,
        method: "Page.navigate",
        params_json: "{\"url\":\"https://example.com\"}",
        session_id: "SESS-ABC-99",
    };
    const n2 = CdpFrameScanner.encodeRequest(req2, &buf)?;
    const s2 = buf[0..n2];
    expect_eq(
        std.mem.eql(u8, s2, "{\"id\":42,\"method\":\"Page.navigate\",\"params\":{\"url\":\"https://example.com\"},\"sessionId\":\"SESS-ABC-99\"}"),
        true,
    );

    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        const alloced = CdpFrameScanner.encodeRequestAlloc(alloc, req2)?;
        defer alloc.free(alloced);
        expect_eq(std.mem.eql(u8, alloced, s2), true);
    }
    expect_eq(gpa.deinit(), std.heap.Check.ok);
}

fn testSimdFrameScanner(): !void {
    // >32 bytes payload to exercise multiple 16-lane SIMD vector iterations
    const payload = "{\"id\":123456,\"sessionId\":\"SESSION_SIMD_001\",\"result\":{\"frameTree\":{\"frame\":{\"id\":\"F1\",\"url\":\"https://cp-lang.dev/docs\\\"quoted\\\"\"}}}}";

    const id_raw = CdpFrameScanner.extractTopLevelField(payload, "id", false) orelse return error.TestFailed;
    expect_eq(CdpFrameScanner.parseFastI64(id_raw) orelse 0, 123456);

    const sid = CdpFrameScanner.extractTopLevelField(payload, "sessionId", true) orelse return error.TestFailed;
    expect_eq(std.mem.eql(u8, sid, "SESSION_SIMD_001"), true);

    const res = CdpFrameScanner.extractTopLevelField(payload, "result", false) orelse return error.TestFailed;
    expect_eq(res[0], '{');
    expect_eq(res[res.len - 1], '}');

    const ft = CdpFrameScanner.extractTopLevelField(res, "frameTree", false) orelse return error.TestFailed;
    const fr = CdpFrameScanner.extractTopLevelField(ft, "frame", false) orelse return error.TestFailed;
    const fid = CdpFrameScanner.extractTopLevelField(fr, "id", true) orelse return error.TestFailed;
    expect_eq(std.mem.eql(u8, fid, "F1"), true);
}

fn testResponseAndErrorDecoding(): !void {
    const ok_frame = "  { \"id\": 7, \"sessionId\": \"TAB-1\", \"result\": {\"nodeId\": 88, \"ok\": true} } ";
    const msg_ok = CdpFrameScanner.decodeMessage(ok_frame)?;
    expect_eq(msg_ok.isResponse(), true);
    expect_eq(msg_ok.isError(), false);
    expect_eq(msg_ok.id, 7);
    expect_eq(std.mem.eql(u8, msg_ok.session_id, "TAB-1"), true);
    const node_raw = CdpFrameScanner.extractTopLevelField(msg_ok.result_json, "nodeId", false) orelse "";
    expect_eq(CdpFrameScanner.parseFastI64(node_raw) orelse 0, 88);

    const err_frame = "{\"id\":9,\"sessionId\":\"TAB-2\",\"error\":{\"code\":-32000,\"message\":\"Session with given id not found.\"}}";
    const msg_err = CdpFrameScanner.decodeMessage(err_frame)?;
    expect_eq(msg_err.isResponse(), true);
    expect_eq(msg_err.isError(), true);
    expect_eq(msg_err.id, 9);
    expect_eq(msg_err.error_code, -32000);
    expect_eq(std.mem.eql(u8, msg_err.error_message, "Session with given id not found."), true);
    expect_eq(std.mem.eql(u8, msg_err.session_id, "TAB-2"), true);
}

fn testEventDecodingAndInvalidFrames(): !void {
    const ev_frame = "{\"method\":\"Page.loadEventFired\",\"params\":{\"timestamp\":12345.67},\"sessionId\":\"PAGE-7\"}";
    const ev_msg = CdpFrameScanner.decodeMessage(ev_frame)?;
    expect_eq(ev_msg.isEvent(), true);
    expect_eq(std.mem.eql(u8, ev_msg.method, "Page.loadEventFired"), true);
    expect_eq(std.mem.eql(u8, ev_msg.session_id, "PAGE-7"), true);

    let saw_invalid = false;
    * = CdpFrameScanner.decodeMessage("not json") catch {
        saw_invalid = true;
    };
    expect_eq(saw_invalid, true);

    let saw_missing_method = false;
    * = CdpFrameScanner.decodeMessage("{\"params\":{}}") catch {
        saw_missing_method = true;
    };
    expect_eq(saw_missing_method, true);
}

fn testEndpointStateMachineAndCorrelation(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let ep = CdpEndpoint.init(alloc);
        defer ep.deinit();

        expect_eq(ep.state, CdpConnectionState.Disconnected);
        let buf: [512]u8 = undefined;

        // Sending while Disconnected must return NotConnected
        let not_conn_err = false;
        * = ep.sendCommand("", "Browser.getVersion", "{}", &buf) catch {
            not_conn_err = true;
        };
        expect_eq(not_conn_err, true);

        let mt = MockTransport{};
        ep.setTransport(mockSendFn, unsafe { @intFromPtr(&mt) });
        ep.beginConnect();
        expect_eq(ep.state, CdpConnectionState.Connecting);
        ep.markConnected();
        expect_eq(ep.state, CdpConnectionState.Connected);

        const id1 = ep.sendCommand("", "Browser.getVersion", "{}", &buf)?;
        const id2 = ep.sendCommand("", "Target.getTargets", "{}", &buf)?;
        expect_eq(id1, 1);
        expect_eq(id2, 2);
        expect_eq(ep.pendingCount(), 2);
        expect_eq(mt.send_count, 2);

        // Correlate response for id2 out-of-order
        * = ep.handleIncomingFrame("{\"id\":2,\"result\":{\"targetInfos\":[]}}")?;
        expect_eq(ep.takeResponse(1) == null, true);

        const r2 = ep.takeResponse(2) orelse return error.TestFailed;
        expect_eq(r2.id, 2);
        expect_eq(r2.is_error, false);
        expect_eq(ep.pendingCount(), 1);

        // Correlate error response for id1
        * = ep.handleIncomingFrame("{\"id\":1,\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}")?;
        const r1 = ep.takeResponse(1) orelse return error.TestFailed;
        expect_eq(r1.is_error, true);
        expect_eq(r1.error_code, -32601);
        expect_eq(ep.pendingCount(), 0);
    }
    expect_eq(gpa.deinit(), std.heap.Check.ok);
}

fn testMultiTargetSessionMultiplexing(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let ep = CdpEndpoint.init(alloc);
        defer ep.deinit();
        ep.markConnected();

        ep.createSession("S1", "page", "T1", "")?;
        ep.createSession("S2", "iframe", "T2", "S1")?;
        expect_eq(ep.sessionCount(), 2);
        expect_eq(ep.hasActiveSession("S1"), true);
        expect_eq(ep.hasActiveSession("S2"), true);

        let buf: [512]u8 = undefined;
        const id_s1 = ep.sendCommand("S1", "Page.reload", "{}", &buf)?;
        const id_s2 = ep.sendCommand("S2", "Runtime.evaluate", "{\"expression\":\"1+1\"}", &buf)?;
        expect_eq(id_s1, 1);
        expect_eq(id_s2, 2);

        // Unknown session must fail with SessionNotFound
        let not_found = false;
        * = ep.sendCommand("S_UNKNOWN", "Page.reload", "{}", &buf) catch {
            not_found = true;
        };
        expect_eq(not_found, true);

        // Target.detachedFromTarget auto-detaches S2
        * = ep.handleIncomingFrame("{\"method\":\"Target.detachedFromTarget\",\"params\":{\"sessionId\":\"S2\"}}")?;
        expect_eq(ep.hasActiveSession("S2"), false);
        expect_eq(ep.hasActiveSession("S1"), true);

        let closed_err = false;
        * = ep.sendCommand("S2", "Page.reload", "{}", &buf) catch {
            closed_err = true;
        };
        expect_eq(closed_err, true);

        // Endpoint close detaches all sessions
        ep.close();
        expect_eq(ep.hasActiveSession("S1"), false);
        expect_eq(ep.hasActiveSession(""), false);
    }
    expect_eq(gpa.deinit(), std.heap.Check.ok);
}

fn testEventDispatchFiltering(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let ep = CdpEndpoint.init(alloc);
        defer ep.deinit();
        ep.markConnected();

        let all_collector = EventCollector{};
        let page_s1_collector = EventCollector{};

        ep.onEvent("*", "*", onEventCollect, unsafe { @intFromPtr(&all_collector) })?;
        ep.onEvent("Page.loadEventFired", "S1", onEventCollect, unsafe { @intFromPtr(&page_s1_collector) })?;

        * = ep.handleIncomingFrame("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"1\"},\"sessionId\":\"S1\"}")?;
        * = ep.handleIncomingFrame("{\"method\":\"Page.loadEventFired\",\"params\":{\"timestamp\":100},\"sessionId\":\"S2\"}")?;
        * = ep.handleIncomingFrame("{\"method\":\"Page.loadEventFired\",\"params\":{\"timestamp\":200},\"sessionId\":\"S1\"}")?;

        expect_eq(all_collector.count, 3);
        expect_eq(page_s1_collector.count, 1);
        expect_eq(std.mem.eql(u8, page_s1_collector.last_method[0..page_s1_collector.last_method_len], "Page.loadEventFired"), true);
        expect_eq(std.mem.eql(u8, page_s1_collector.last_session[0..page_s1_collector.last_session_len], "S1"), true);
    }
    expect_eq(gpa.deinit(), std.heap.Check.ok);
}

fn testBrowserLauncherArgsAndProbing(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        const names = CdpBrowserLauncher.defaultCandidateNames();
        expect_eq(names.len, 5);
        expect_eq(std.mem.eql(u8, names[0], "chromium"), true);
        expect_eq(std.mem.eql(u8, names[4], "capsurf"), true);

        const opts = CdpBrowserLaunchOptions{
            headless: true,
            user_data_dir: "/tmp/cp-cdp-profile",
            viewport_width: 1920,
            viewport_height: 1080,
            remote_debugging_port: 9222,
            ignore_https_errors: true,
            args: &.{ "--disable-gpu" },
        };
        const built = CdpBrowserLauncher.buildLaunchArgs(alloc, opts)?;
        defer CdpBrowserLauncher.freeLaunchArgs(alloc, built);

        let has_headless = false;
        let has_port = false;
        let has_win = false;
        let has_udd = false;
        let has_gpu = false;
        for (a of built) {
            if (std.mem.eql(u8, a, "--headless=new")) has_headless = true;
            if (std.mem.eql(u8, a, "--remote-debugging-port=9222")) has_port = true;
            if (std.mem.eql(u8, a, "--window-size=1920,1080")) has_win = true;
            if (std.mem.eql(u8, a, "--user-data-dir=/tmp/cp-cdp-profile")) has_udd = true;
            if (std.mem.eql(u8, a, "--disable-gpu")) has_gpu = true;
        }
        expect_eq(has_headless, true);
        expect_eq(has_port, true);
        expect_eq(has_win, true);
        expect_eq(has_udd, true);
        expect_eq(has_gpu, true);

        if (found := CdpBrowserLauncher.probeBrowserExecutable(alloc, "")) {
            alloc.free(found);
        }
    }
    expect_eq(gpa.deinit(), std.heap.Check.ok);
}

fn testDevToolsWsUrlParser(): !void {
    const stderr_log = "[1128/024500.123:INFO] Starting...\nDevTools listening on ws://127.0.0.1:9222/devtools/browser/9f8e7d6c-1234-5678-9abc-def012345678\r\n";
    const ep1 = CdpBrowserLauncher.parseDevToolsWsUrl(stderr_log) orelse return error.TestFailed;
    expect_eq(std.mem.eql(u8, ep1.host, "127.0.0.1"), true);
    expect_eq(ep1.port, 9222);
    expect_eq(std.mem.eql(u8, ep1.path, "/devtools/browser/9f8e7d6c-1234-5678-9abc-def012345678"), true);
    expect_eq(ep1.is_tls, false);

    const ep2 = CdpBrowserLauncher.parseWsUrl("wss://[::1]:9443/devtools/page/42") orelse return error.TestFailed;
    expect_eq(std.mem.eql(u8, ep2.host, "[::1]"), true);
    expect_eq(ep2.port, 9443);
    expect_eq(std.mem.eql(u8, ep2.path, "/devtools/page/42"), true);
    expect_eq(ep2.is_tls, true);

    expect_eq(CdpBrowserLauncher.parseWsUrl("http://localhost:9222") == null, true);
}

fn testCdpDomainBuildersAndAXTree(): !void {
    let pbuf: [512]u8 = undefined;
    let fbuf: [1024]u8 = undefined;

    const nav_req = PageDomain.navigate(10, "S1", PageNavigateParams{ url: "https://cp-lang.dev" }, &pbuf)?;
    expect_eq(std.mem.eql(u8, nav_req.method, "Page.navigate"), true);
    const nav_len = CdpFrameScanner.encodeRequest(nav_req, &fbuf)?;
    const nav_dec = CdpFrameScanner.decodeMessage(fbuf[0..nav_len])?;
    expect_eq(nav_dec.id, 10);

    const ss_req = PageDomain.captureScreenshot(11, "S1", PageCaptureScreenshotParams{ format: "jpeg", quality: 90 }, &pbuf)?;
    expect_eq(std.mem.eql(u8, ss_req.method, "Page.captureScreenshot"), true);

    const pdf_req = PageDomain.printToPDF(12, "S1", PagePrintToPdfParams{ landscape: true }, &pbuf)?;
    expect_eq(std.mem.eql(u8, pdf_req.method, "Page.printToPDF"), true);

    const rt_req = RuntimeDomain.evaluate(13, "S1", RuntimeEvaluateParams{ expression: "document.title" }, &pbuf)?;
    expect_eq(std.mem.eql(u8, rt_req.method, "Runtime.evaluate"), true);

    const doc_req = DomDomain.getDocument(14, "S1", DomGetDocumentParams{ depth: 2 }, &pbuf)?;
    expect_eq(std.mem.eql(u8, doc_req.method, "DOM.getDocument"), true);

    const qs_req = DomDomain.querySelector(15, "S1", DomQuerySelectorParams{ node_id: 3, selector: "#submit-btn" }, &pbuf)?;
    expect_eq(std.mem.eql(u8, qs_req.method, "DOM.querySelector"), true);

    const mouse_req = InputDomain.dispatchMouseEvent(16, "S1", InputMouseEventParams{ x: 100.5, y: 200.5 }, &pbuf)?;
    expect_eq(std.mem.eql(u8, mouse_req.method, "Input.dispatchMouseEvent"), true);

    const key_req = InputDomain.dispatchKeyEvent(17, "S1", InputKeyEventParams{ key: "Enter", code: "Enter" }, &pbuf)?;
    expect_eq(std.mem.eql(u8, key_req.method, "Input.dispatchKeyEvent"), true);

    const net_req = NetworkDomain.enable(18, "S1", NetworkEnableParams{}, &pbuf)?;
    expect_eq(std.mem.eql(u8, net_req.method, "Network.enable"), true);

    const ax_req = AccessibilityDomain.getFullAXTree(19, "S1", AccessibilityGetFullAXTreeParams{ depth: 4 }, &pbuf)?;
    expect_eq(std.mem.eql(u8, ax_req.method, "Accessibility.getFullAXTree"), true);

    // Parse AXNode JSON for AI Agent perception
    const ax_node_json = "{\"nodeId\":\"ax-42\",\"ignored\":false,\"role\":{\"type\":\"role\",\"value\":\"button\"},\"name\":{\"type\":\"computedString\",\"value\":\"Submit Order\"},\"value\":{\"type\":\"string\",\"value\":\"ready\"},\"backendDOMNodeId\":777}";
    const summary = AccessibilityDomain.parseAXNodeSummary(ax_node_json);
    expect_eq(std.mem.eql(u8, summary.node_id, "ax-42"), true);
    expect_eq(summary.ignored, false);
    expect_eq(std.mem.eql(u8, summary.role, "button"), true);
    expect_eq(std.mem.eql(u8, summary.name, "Submit Order"), true);
    expect_eq(std.mem.eql(u8, summary.value, "ready"), true);
    expect_eq(summary.backend_dom_node_id, 777);
}

fn testCapCdpNamespaceAndSession(): !void {
    const sess = cap.cdp.CdpSession.init("S-NS", "page", "T-NS", "");
    expect_eq(sess.isRoot(), false);
    let buf: [256]u8 = undefined;
    const n = sess.encodeCommand(5, "Page.reload", "{}", &buf)?;
    const decoded = cap.cdp.CdpFrameScanner.decodeMessage(buf[0..n])?;
    expect_eq(decoded.id, 5);
    expect_eq(std.mem.eql(u8, decoded.session_id, "S-NS"), true);
}

fn main() {
    test("1. JSON-RPC Request Encoding (Stack & Alloc)", () => {
        testRequestEncoding() catch unreachable;
    });
    test("2. 16-Lane SIMD Frame Scanner & Top-Level Extractor", () => {
        testSimdFrameScanner() catch unreachable;
    });
    test("3. JSON-RPC Response & Error Response Decoding", () => {
        testResponseAndErrorDecoding() catch unreachable;
    });
    test("4. CDP Event Decoding & Malformed Frame Rejection", () => {
        testEventDecodingAndInvalidFrames() catch unreachable;
    });
    test("5. CdpEndpoint State Machine & Out-of-Order Correlation", () => {
        testEndpointStateMachineAndCorrelation() catch unreachable;
    });
    test("6. Multi-Target sessionId Multiplexing & Auto-Detach", () => {
        testMultiTargetSessionMultiplexing() catch unreachable;
    });
    test("7. CdpEndpoint Event Callback Dispatch & Filtering", () => {
        testEventDispatchFiltering() catch unreachable;
    });
    test("8. Browser Launch Options, Args Builder & Probing", () => {
        testBrowserLauncherArgsAndProbing() catch unreachable;
    });
    test("9. DevTools WebSocket URL Parser (ws:// & wss://)", () => {
        testDevToolsWsUrlParser() catch unreachable;
    });
    test("10. CDP Domain Builders & Accessibility AXNode Parser", () => {
        testCdpDomainBuildersAndAXTree() catch unreachable;
    });
    test("11. cap.cdp Namespace & CdpSession Encode", () => {
        testCapCdpNamespaceAndSession() catch unreachable;
    });
}
