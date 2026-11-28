// Source: cp/cap/test/testCapPlaywrightDispatchers.cp
// Comprehensive Verification Suite for packages/playwright/src/dispatchers.cp
// Covers:
//   1. RpcObjectKind (14 kinds) & 16-lane SIMD JSON field extraction
//   2. DispatcherScope GUID registry, adopt, and parent-child cascading RAII dispose
//   3. Full RpcDispatcher dispatch() coverage:
//      BrowserType, Browser, BrowserContext, Page, Frame, Clock, FileChooser, Tracing, Locator
//   4. JSON-RPC error codes (-32600, -32601, -32602, -32001)
//   5. DebugAllocator zero-leak verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { CdpFrameScanner } from "cap:cdp";
import {
    RpcObjectKind,
    rpcObjectKindName,
    RpcErrorCode,
    rpcErrorCodeInt,
    simdExtractJsonField,
    simdExtractJsonInt,
    simdExtractJsonBool,
    DispatcherScope,
    RpcDispatcher,
} from "../../packages/playwright/src/dispatchers.cp";

struct MockRpcTransport {
    sent_count: usize = 0,
    last_method: [128]u8 = undefined,
    last_method_len: usize = 0,

    fn record(*_, payload: []const u8): bool {
        _.sent_count += 1;
        if (m := CdpFrameScanner.extractTopLevelField(payload, "method", true)) {
            const n = if (m.len <= 128) m.len else 128;
            @memcpy(_.last_method[0..n], m[0..n]);
            _.last_method_len = n;
        }
        return true;
    }

    fn lastMethod(): []const u8 {
        return _.last_method[0.._.last_method_len];
    }
}

fn mockRpcTransportSend(ctx: usize, payload: []const u8): bool {
    const mt = unsafe { ctx as *MockRpcTransport! };
    return mt.record(payload);
}

fn extractGuidFromField(json: []const u8, field_name: []const u8): ([]const u8)? {
    const res_obj = simdExtractJsonField(json, "result", false) orelse return null;
    const sub_obj = simdExtractJsonField(res_obj, field_name, false) orelse return null;
    return simdExtractJsonField(sub_obj, "guid", true);
}

test("PlaywrightDispatchers: RpcObjectKind names and 16-lane SIMD JSON extraction") {
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Playwright), "Playwright"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.BrowserType), "BrowserType"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Browser), "Browser"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.BrowserContext), "BrowserContext"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Page), "Page"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Frame), "Frame"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Locator), "Locator"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Route), "Route"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Request), "Request"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Response), "Response"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.FileChooser), "FileChooser"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Tracing), "Tracing"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.Clock), "Clock"), true);
    expect_eq(std.mem.eql(u8, rpcObjectKindName(RpcObjectKind.APIRequestContext), "APIRequestContext"), true);

    const payload = "{\"url\":\"https://playwright.dev/docs/intro?a=1&b=2\",\"timeout\":15000,\"headless\":true,\"nested\":{\"k\":\"v\"}}";
    const url = simdExtractJsonField(payload, "url", true) orelse "";
    expect_eq(std.mem.eql(u8, url, "https://playwright.dev/docs/intro?a=1&b=2"), true);
    expect_eq(simdExtractJsonInt(payload, "timeout") orelse 0, 15000);
    expect_eq(simdExtractJsonBool(payload, "headless") orelse false, true);
    const nested = simdExtractJsonField(payload, "nested", false) orelse "";
    expect_eq(std.mem.eql(u8, nested, "{\"k\":\"v\"}"), true);
}

test("PlaywrightDispatchers: DispatcherScope parent-child cascading dispose and DebugAllocator zero-leak") {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();

    {
        let scope = DispatcherScope.init(alloc);
        defer scope.deinit();

        * = scope.register("pw@1", RpcObjectKind.Playwright, "", 0, false) catch unreachable;
        * = scope.register("bt@1", RpcObjectKind.BrowserType, "pw@1", 0, false) catch unreachable;
        * = scope.register("br@1", RpcObjectKind.Browser, "bt@1", 0, false) catch unreachable;
        * = scope.register("ctx@1", RpcObjectKind.BrowserContext, "br@1", 0, false) catch unreachable;
        * = scope.register("ctx@2", RpcObjectKind.BrowserContext, "br@1", 0, false) catch unreachable;
        * = scope.register("page@1", RpcObjectKind.Page, "ctx@1", 0, false) catch unreachable;
        * = scope.register("frame@1", RpcObjectKind.Frame, "page@1", 0, false) catch unreachable;
        * = scope.register("loc@1", RpcObjectKind.Locator, "frame@1", 0, false) catch unreachable;

        expect_eq(scope.count(), 8);
        expect_eq(scope.childCount("br@1"), 2);
        expect_eq(scope.childCount("ctx@1"), 1);

        // Reparent page@1 from ctx@1 to ctx@2
        expect_eq(scope.adopt("ctx@2", "page@1"), true);
        expect_eq(scope.childCount("ctx@1"), 0);
        expect_eq(scope.childCount("ctx@2"), 1);

        // Cascading dispose of ctx@2 must dispose ctx@2 -> page@1 -> frame@1 -> loc@1 (4 nodes)
        const n1 = scope.dispose("ctx@2");
        expect_eq(n1, 4);
        expect_eq(scope.contains("ctx@2"), false);
        expect_eq(scope.contains("page@1"), false);
        expect_eq(scope.contains("frame@1"), false);
        expect_eq(scope.contains("loc@1"), false);
        expect_eq(scope.count(), 4);

        // Disposing already disposed GUID is idempotent (0)
        expect_eq(scope.dispose("ctx@2"), 0);

        // Remaining nodes cascade cleanly on scope.deinit()
    }

    expect_eq(gpa.deinit(), .ok);
}

test("PlaywrightDispatchers: full BrowserType/Browser/Context/Page/Frame/Clock/FileChooser RPC dispatch") {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();

    {
        let mock = MockRpcTransport{};
        let disp = RpcDispatcher.init(alloc) catch unreachable;
        defer disp.deinit();
        disp.setTransport(mockRpcTransportSend, unsafe { @intFromPtr(&mock) });

        // 1. Root Playwright.initialize
        {
            const init_res = disp.dispatch(alloc, "", "initialize", "{}") catch unreachable;
            defer alloc.free(init_res);
            expect_eq(std.mem.indexOf(u8, init_res, "browser-type@chromium") != null, true);
        }

        // 2. BrowserType.launch
        let browser_guid_buf: [64]u8 = undefined;
        let browser_guid: []const u8 = "";
        {
            const launch_res = disp.dispatch(alloc, disp.browser_type_guid, "launch", "{\"headless\":true}") catch unreachable;
            defer alloc.free(launch_res);
            const g = extractGuidFromField(launch_res, "browser") orelse "";
            expect_eq(g.len > 0, true);
            @memcpy(browser_guid_buf[0..g.len], g);
            browser_guid = browser_guid_buf[0..g.len];
            expect_eq(disp.scope.contains(browser_guid), true);
        }

        // 3. BrowserType.connectOverCDP + Browser.close cascade
        {
            const cdp_res = disp.dispatch(
                alloc,
                disp.browser_type_guid,
                "connectOverCDP",
                "{\"endpointURL\":\"ws://127.0.0.1:9222/devtools/browser/abc\"}",
            ) catch unreachable;
            defer alloc.free(cdp_res);
            const cdp_b_guid = extractGuidFromField(cdp_res, "browser") orelse "";
            expect_eq(cdp_b_guid.len > 0, true);
            expect_eq(disp.scope.contains(cdp_b_guid), true);

            const close_cdp = disp.dispatch(alloc, cdp_b_guid, "close", "{}") catch unreachable;
            defer alloc.free(close_cdp);
            expect_eq(std.mem.indexOf(u8, close_cdp, "\"closed\":true") != null, true);
            expect_eq(disp.scope.contains(cdp_b_guid), false);
        }

        // 4. Browser.newContext -> BrowserContext cookies/storageState/tracingStart/tracingStop
        let ctx_guid_buf: [64]u8 = undefined;
        let ctx_guid: []const u8 = "";
        {
            const ctx_res = disp.dispatch(alloc, browser_guid, "newContext", "{\"viewportWidth\":1440,\"viewportHeight\":900}") catch unreachable;
            defer alloc.free(ctx_res);
            const g = extractGuidFromField(ctx_res, "context") orelse "";
            expect_eq(g.len > 0, true);
            @memcpy(ctx_guid_buf[0..g.len], g);
            ctx_guid = ctx_guid_buf[0..g.len];
        }

        {
            const add_c = disp.dispatch(alloc, ctx_guid, "addCookies", "{\"name\":\"sid\",\"value\":\"xyz\",\"domain\":\"example.com\",\"path\":\"/\"}") catch unreachable;
            defer alloc.free(add_c);
            expect_eq(std.mem.indexOf(u8, add_c, "\"added\":1") != null, true);

            const get_c = disp.dispatch(alloc, ctx_guid, "cookies", "{}") catch unreachable;
            defer alloc.free(get_c);
            expect_eq(std.mem.indexOf(u8, get_c, "\"sid\"") != null, true);

            const ss = disp.dispatch(alloc, ctx_guid, "storageState", "{}") catch unreachable;
            defer alloc.free(ss);
            expect_eq(std.mem.indexOf(u8, ss, "\"cookieCount\":1") != null, true);

            const clr = disp.dispatch(alloc, ctx_guid, "clearCookies", "{}") catch unreachable;
            defer alloc.free(clr);
            expect_eq(std.mem.indexOf(u8, clr, "\"cleared\":true") != null, true);

            const ts = disp.dispatch(alloc, ctx_guid, "tracingStart", "{\"name\":\"rpc-trace\",\"screenshots\":true}") catch unreachable;
            defer alloc.free(ts);
            expect_eq(std.mem.indexOf(u8, ts, "\"recording\":true") != null, true);

            const tstop = disp.dispatch(alloc, ctx_guid, "tracingStop", "{}") catch unreachable;
            defer alloc.free(tstop);
            expect_eq(std.mem.indexOf(u8, tstop, "\"recording\":false") != null, true);
        }

        // 5. BrowserContext.newPage -> Page & Frame full methods
        let page_guid_buf: [64]u8 = undefined;
        let page_guid: []const u8 = "";
        let frame_guid_buf: [64]u8 = undefined;
        let frame_guid: []const u8 = "";
        {
            const pg_res = disp.dispatch(alloc, ctx_guid, "newPage", "{}") catch unreachable;
            defer alloc.free(pg_res);
            const pg = extractGuidFromField(pg_res, "page") orelse "";
            const fg = extractGuidFromField(pg_res, "mainFrame") orelse "";
            expect_eq(pg.len > 0, true);
            expect_eq(fg.len > 0, true);
            @memcpy(page_guid_buf[0..pg.len], pg);
            page_guid = page_guid_buf[0..pg.len];
            @memcpy(frame_guid_buf[0..fg.len], fg);
            frame_guid = frame_guid_buf[0..fg.len];
        }

        // Page methods: goto, reload, evaluate, setContent, content, title, screenshot, pdf, click, fill, locator, route
        {
            const r_goto = disp.dispatch(alloc, page_guid, "goto", "{\"url\":\"https://example.com\"}") catch unreachable;
            defer alloc.free(r_goto);
            expect_eq(std.mem.indexOf(u8, r_goto, "https://example.com") != null, true);

            const r_reload = disp.dispatch(alloc, page_guid, "reload", "{}") catch unreachable;
            defer alloc.free(r_reload);
            expect_eq(std.mem.indexOf(u8, r_reload, "\"ok\":true") != null, true);

            const r_eval = disp.dispatch(alloc, page_guid, "evaluate", "{\"expression\":\"1+1\"}") catch unreachable;
            defer alloc.free(r_eval);
            expect_eq(std.mem.indexOf(u8, r_eval, "\"result\"") != null, true);

            const r_setc = disp.dispatch(alloc, page_guid, "setContent", "{\"html\":\"<h1>CP</h1>\"}") catch unreachable;
            defer alloc.free(r_setc);
            const r_cont = disp.dispatch(alloc, page_guid, "content", "{}") catch unreachable;
            defer alloc.free(r_cont);
            expect_eq(std.mem.indexOf(u8, r_cont, "<h1>CP</h1>") != null, true);

            const r_title = disp.dispatch(alloc, page_guid, "title", "{}") catch unreachable;
            defer alloc.free(r_title);
            expect_eq(std.mem.indexOf(u8, r_title, "\"value\"") != null, true);

            const r_shot = disp.dispatch(alloc, page_guid, "screenshot", "{\"type\":\"png\",\"fullPage\":true}") catch unreachable;
            defer alloc.free(r_shot);
            expect_eq(std.mem.indexOf(u8, r_shot, "\"binary\"") != null, true);

            const r_pdf = disp.dispatch(alloc, page_guid, "pdf", "{\"landscape\":true}") catch unreachable;
            defer alloc.free(r_pdf);
            expect_eq(std.mem.indexOf(u8, r_pdf, "\"pdf\"") != null, true);

            const r_click = disp.dispatch(alloc, page_guid, "click", "{\"selector\":\"#submit\"}") catch unreachable;
            defer alloc.free(r_click);
            expect_eq(std.mem.indexOf(u8, r_click, "\"ok\":true") != null, true);

            const r_fill = disp.dispatch(alloc, page_guid, "fill", "{\"selector\":\"#email\",\"value\":\"a@b.com\"}") catch unreachable;
            defer alloc.free(r_fill);
            expect_eq(std.mem.indexOf(u8, r_fill, "\"ok\":true") != null, true);

            const r_loc = disp.dispatch(alloc, page_guid, "locator", "{\"selector\":\".btn\"}") catch unreachable;
            defer alloc.free(r_loc);
            const loc_guid = extractGuidFromField(r_loc, "locator") orelse "";
            expect_eq(loc_guid.len > 0, true);
            expect_eq(disp.scope.contains(loc_guid), true);

            const r_route = disp.dispatch(alloc, page_guid, "route", "{\"url\":\"**/api/**\"}") catch unreachable;
            defer alloc.free(r_route);
            expect_eq(std.mem.indexOf(u8, r_route, "**/api/**") != null, true);
        }

        // Frame methods: goto, evaluate, click, fill, locator, reload
        {
            const f_goto = disp.dispatch(alloc, frame_guid, "goto", "{\"url\":\"https://example.com/sub\"}") catch unreachable;
            defer alloc.free(f_goto);
            expect_eq(std.mem.indexOf(u8, f_goto, "https://example.com/sub") != null, true);

            const f_eval = disp.dispatch(alloc, frame_guid, "evaluate", "{\"expression\":\"document.URL\"}") catch unreachable;
            defer alloc.free(f_eval);
            expect_eq(std.mem.indexOf(u8, f_eval, "\"result\"") != null, true);

            const f_click = disp.dispatch(alloc, frame_guid, "click", "{\"selector\":\"button\"}") catch unreachable;
            defer alloc.free(f_click);
            expect_eq(std.mem.indexOf(u8, f_click, "\"ok\":true") != null, true);

            const f_fill = disp.dispatch(alloc, frame_guid, "fill", "{\"selector\":\"input\",\"value\":\"hi\"}") catch unreachable;
            defer alloc.free(f_fill);
            expect_eq(std.mem.indexOf(u8, f_fill, "\"ok\":true") != null, true);

            const f_loc = disp.dispatch(alloc, frame_guid, "locator", "{\"selector\":\"a\"}") catch unreachable;
            defer alloc.free(f_loc);
            expect_eq((extractGuidFromField(f_loc, "locator") orelse "").len > 0, true);
        }

        // 6. Clock methods: install, fastForward, pauseAt, setFixedTime
        {
            const clk_guid = disp.registerClockForPage(page_guid) catch unreachable;
            const c_inst = disp.dispatch(alloc, clk_guid, "install", "{\"time\":1000}") catch unreachable;
            defer alloc.free(c_inst);
            expect_eq(std.mem.indexOf(u8, c_inst, "\"installed\":true") != null, true);

            const c_ff = disp.dispatch(alloc, clk_guid, "fastForward", "{\"ticksNumber\":500}") catch unreachable;
            defer alloc.free(c_ff);
            expect_eq(std.mem.indexOf(u8, c_ff, "1500") != null, true);

            const c_pause = disp.dispatch(alloc, clk_guid, "pauseAt", "{\"time\":5000}") catch unreachable;
            defer alloc.free(c_pause);
            expect_eq(std.mem.indexOf(u8, c_pause, "\"paused\":true") != null, true);

            const c_fix = disp.dispatch(alloc, clk_guid, "setFixedTime", "{\"time\":9000}") catch unreachable;
            defer alloc.free(c_fix);
            expect_eq(std.mem.indexOf(u8, c_fix, "\"fixed\":true") != null, true);
        }

        // 7. FileChooser.setFiles
        {
            const fc_guid = disp.registerFileChooserForPage(page_guid, "input[type=file]", true) catch unreachable;
            const fc_res = disp.dispatch(alloc, fc_guid, "setFiles", "{\"files\":[\"/tmp/a.txt\",\"/tmp/b.txt\"]}") catch unreachable;
            defer alloc.free(fc_res);
            expect_eq(std.mem.indexOf(u8, fc_res, "\"count\":2") != null, true);
        }

        // 8. Browser.newPage shortcut & wire frame dispatchRpcFrame
        {
            let frame_buf: [256]u8 = undefined;
            const req_frame = std.fmt.bufPrint(
                &frame_buf,
                "{{\"id\":42,\"guid\":\"{s}\",\"method\":\"newPage\",\"params\":{{}}}}",
                .{browser_guid},
            ) catch unreachable;
            const wire_res = disp.dispatchRpcFrame(alloc, req_frame) catch unreachable;
            defer alloc.free(wire_res);
            expect_eq(std.mem.indexOf(u8, wire_res, "\"id\":42") != null, true);
            expect_eq(std.mem.indexOf(u8, wire_res, "\"page\"") != null, true);
        }

        // 9. Error codes: unknown guid (-32001), unknown method (-32601), invalid params (-32602), empty method (-32600)
        {
            const e_guid = disp.dispatch(alloc, "page@99999", "goto", "{\"url\":\"https://x.test\"}") catch unreachable;
            defer alloc.free(e_guid);
            expect_eq(std.mem.indexOf(u8, e_guid, "-32001") != null, true);

            const e_meth = disp.dispatch(alloc, page_guid, "nonExistentMethod", "{}") catch unreachable;
            defer alloc.free(e_meth);
            expect_eq(std.mem.indexOf(u8, e_meth, "-32601") != null, true);

            const e_param = disp.dispatch(alloc, page_guid, "goto", "{}") catch unreachable;
            defer alloc.free(e_param);
            expect_eq(std.mem.indexOf(u8, e_param, "-32602") != null, true);

            const e_empty = disp.dispatch(alloc, page_guid, "", "{}") catch unreachable;
            defer alloc.free(e_empty);
            expect_eq(std.mem.indexOf(u8, e_empty, "-32600") != null, true);
        }

        // 10. Closing browser cascades disposal of browser + contexts + pages + frames + clocks + filechoosers + locators
        {
            const close_br = disp.dispatch(alloc, browser_guid, "close", "{}") catch unreachable;
            defer alloc.free(close_br);
            expect_eq(std.mem.indexOf(u8, close_br, "\"closed\":true") != null, true);
            expect_eq(disp.scope.contains(browser_guid), false);
            expect_eq(disp.scope.contains(ctx_guid), false);
            expect_eq(disp.scope.contains(page_guid), false);
            expect_eq(disp.scope.contains(frame_guid), false);
            // Only root Playwright and BrowserType remain
            expect_eq(disp.scope.count(), 2);
        }

        expect_eq(mock.sent_count > 0, true);
    }

    expect_eq(gpa.deinit(), .ok);
}
