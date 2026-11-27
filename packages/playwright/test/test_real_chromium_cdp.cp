// ============================================================================
// packages/playwright/test/test_real_chromium_cdp.cp
// Real Chromium (158.0.8127.88) End-to-End CDP Verification over TCP/HTTP/WS
// ============================================================================

const std = @import("std");
import {
    CdpBrowserLauncher,
    CdpFrameScanner,
    CdpResponse,
} from "cap:cdp";
import {
    chromium,
    Browser,
} from "../src/index";

fn expectTrue(cond: bool, label: []const u8): void! {
    if (!cond) {
        std.debug.print("[FAIL] {s}\n", .{label});
        return error.AssertionFailed;
    }
}

type SockAddrIn = std.c.sockaddr.in;

fn connectLoopbackTcp(port: u16): i32! {
    const fd = std.c.socket(std.c.AF.INET, std.c.SOCK.STREAM, 0);
    if (fd < 0) return error.SocketCreateFailed;

    let addr = SockAddrIn{
        family: std.c.AF.INET,
        port: std.mem.nativeToBig(u16, port),
        addr: std.mem.nativeToBig(u32, 0x7f000001),
    };
    const rc = std.c.connect(fd, unsafe { @ptrCast(&addr) }, @sizeOf(SockAddrIn));
    if (rc < 0) {
        const _rc_close = std.c.close(fd);
        * = _rc_close;
        return error.ConnectFailed;
    }
    return fd;
}

fn writeAllFd(fd: i32, data: []const u8): bool {
    let sent: usize = 0;
    while (sent < data.len) {
        const n = std.c.write(fd, data[sent..data.len].ptr, data.len - sent);
        if (n <= 0) return false;
        sent += @as(usize, @intCast(n));
    }
    return true;
}

fn readExactFd(fd: i32, out: []u8): bool {
    let got: usize = 0;
    while (got < out.len) {
        const n = std.c.read(fd, out[got..out.len].ptr, out.len - got);
        if (n <= 0) return false;
        got += @as(usize, @intCast(n));
    }
    return true;
}

fn httpGetLoopback(port: u16, path: []const u8, out_buf: []u8): ([]const u8)! {
    const fd = connectLoopbackTcp(port)?;
    defer * = std.c.close(fd);

    let req_buf: [512]u8 = undefined;
    const req = std.fmt.bufPrint(
        &req_buf,
        "GET {s} HTTP/1.1\r\nHost: 127.0.0.1:{d}\r\nConnection: close\r\n\r\n",
        .{ path, port },
    ) catch return error.BufferTooSmall;
    if (!writeAllFd(fd, req)) return error.WriteFailed;

    let total: usize = 0;
    let expected_end: usize? = null;
    while (total < out_buf.len) {
        const n = std.c.read(fd, out_buf[total..out_buf.len].ptr, out_buf.len - total);
        if (n <= 0) break;
        total += @as(usize, @intCast(n));
        if (expected_end == null) {
            if (hdr_pos := std.mem.indexOf(u8, out_buf[0..total], "\r\n\r\n")) {
                const hdr = out_buf[0..hdr_pos];
                let cl_len: usize = 0;
                if (cl_pos := std.mem.indexOf(u8, hdr, "Content-Length:")) {
                    const after_cl = std.mem.trimStart(u8, hdr[cl_pos + 15 ..], " \t");
                    const line_end = std.mem.indexOfScalar(u8, after_cl, '\r') ?? after_cl.len;
                    cl_len = std.fmt.parseInt(usize, after_cl[0..line_end], 10) catch 0;
                }
                expected_end = hdr_pos + 4 + cl_len;
            }
        }
        if (end_pos := expected_end) {
            if (total >= end_pos) break;
        }
    }
    const resp = out_buf[0..total];
    const hdr_end = std.mem.indexOf(u8, resp, "\r\n\r\n") ?? return error.InvalidHttpResponse;
    return resp[hdr_end + 4 ..];
}

fn buildMaskedWsFrame(out: []u8, opcode: u8, payload: []const u8, mask: [4]u8): usize! {
    let offset: usize = 2;
    out[0] = 0x80 | opcode;
    if (payload.len <= 125) {
        out[1] = 0x80 | @as(u8, @truncate(payload.len));
    } else if (payload.len <= 0xFFFF) {
        out[1] = 0x80 | 126;
        std.mem.writeInt(u16, out[2..4], @as(u16, @truncate(payload.len)), .big);
        offset = 4;
    } else {
        return error.FrameTooLarge;
    }
    if (out.len < offset + 4 + payload.len) return error.BufferTooSmall;
    @memcpy(out[offset .. offset + 4], &mask);
    offset += 4;
    let i: usize = 0;
    while (i < payload.len) : (i += 1) {
        out[offset + i] = payload[i] ^ mask[i % 4];
    }
    return offset + payload.len;
}

struct WsTcpTransport {
    fd: i32 = -1,
    recv_buf: [65536]u8 = undefined,

    fn sendPayload(*_, payload: []const u8): bool {
        if (_.fd < 0) return false;
        let frame_buf: [16384]u8 = undefined;
        const mask: [4]u8 = [0x37, 0xfa, 0x21, 0x3d];
        const frame_len = buildMaskedWsFrame(&frame_buf, 0x1, payload, mask) catch return false;
        return writeAllFd(_.fd, frame_buf[0..frame_len]);
    }

    fn readNextTextFrame(*_): ([]const u8)! {
        while (true) {
            let hdr2: [2]u8 = undefined;
            if (!readExactFd(_.fd, &hdr2)) return error.ReadFailed;

            const b0 = hdr2[0];
            const b1 = hdr2[1];
            const opcode_raw: u4 = @as(u4, @truncate(b0 & 0x0f));
            const masked = (b1 & 0x80) != 0;
            let payload_len: usize = @as(usize, b1 & 0x7f);

            if (payload_len == 126) {
                let ext2: [2]u8 = undefined;
                if (!readExactFd(_.fd, &ext2)) return error.ReadFailed;
                payload_len = std.mem.readInt(u16, &ext2, .big);
            } else if (payload_len == 127) {
                let ext8: [8]u8 = undefined;
                if (!readExactFd(_.fd, &ext8)) return error.ReadFailed;
                payload_len = @as(usize, @intCast(std.mem.readInt(u64, &ext8, .big)));
            }

            let mask_key: [4]u8 = [0, 0, 0, 0];
            if (masked) {
                if (!readExactFd(_.fd, &mask_key)) return error.ReadFailed;
            }

            if (payload_len > _.recv_buf.len) return error.FrameTooLarge;
            if (payload_len > 0) {
                if (!readExactFd(_.fd, _.recv_buf[0..payload_len])) return error.ReadFailed;
            }

            if (opcode_raw == 0x9) {
                let pong_buf: [256]u8 = undefined;
                const mask: [4]u8 = [0x11, 0x22, 0x33, 0x44];
                const plen = buildMaskedWsFrame(&pong_buf, 0xA, _.recv_buf[0..payload_len], mask) catch return error.PongFailed;
                if (!writeAllFd(_.fd, pong_buf[0..plen])) return error.PongFailed;
                continue;
            }
            if (opcode_raw == 0x8) {
                return error.WebSocketClosed;
            }
            if (opcode_raw == 0x1) {
                return _.recv_buf[0..payload_len];
            }
        }
    }
}

fn wsTransportSend(ctx: usize, payload: []const u8): bool {
    const tr = unsafe { ctx as *WsTcpTransport! };
    return tr.sendPayload(payload);
}

fn waitForCdpResponse(
    browser: *Browser,
    tr: *WsTcpTransport,
    cmd_id: i64,
): CdpResponse! {
    let spins: usize = 0;
    while (spins < 64) : (spins += 1) {
        if (resp := browser.endpoint.takeResponse(cmd_id)) {
            return resp;
        }
        const raw_json = tr.readNextTextFrame()?;
        * = browser.endpoint.handleIncomingFrame(raw_json) catch return error.CdpDecodeFailed;
        if (resp := browser.endpoint.takeResponse(cmd_id)) {
            return resp;
        }
    }
    return error.CdpTimeout;
}

fn performWsHandshake(ws_fd: i32, path: []const u8, port: u16): void! {
    const client_key = "dGhlIHNhbXBsZSBub25jZQ==";
    let up_req_buf: [512]u8 = undefined;
    const up_req = std.fmt.bufPrint(
        &up_req_buf,
        "GET {s} HTTP/1.1\r\nHost: 127.0.0.1:{d}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: {s}\r\nSec-WebSocket-Version: 13\r\n\r\n",
        .{ path, port, client_key },
    ) catch return error.BufferTooSmall;
    expectTrue(writeAllFd(ws_fd, up_req), "WS Upgrade request sent")?;

    let up_resp_buf: [2048]u8 = undefined;
    let up_len: usize = 0;
    while (up_len < up_resp_buf.len) {
        const n_read = std.c.read(ws_fd, up_resp_buf[up_len..up_resp_buf.len].ptr, up_resp_buf.len - up_len);
        if (n_read <= 0) break;
        up_len += @as(usize, @intCast(n_read));
        if (std.mem.indexOf(u8, up_resp_buf[0..up_len], "\r\n\r\n") != null) break;
    }
    const up_resp = up_resp_buf[0..up_len];
    expectTrue(std.mem.startsWith(u8, up_resp, "HTTP/1.1 101"), "HTTP 101 Switching Protocols")?;
    expectTrue(std.mem.indexOf(u8, up_resp, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != null, "Sec-WebSocket-Accept matches RFC 6455")?;
    std.debug.print("[REAL-CDP] WS Handshake 101 OK Sec-WebSocket-Accept=s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\n", .{});
}

type LinuxTimespec = std.os.linux.timespec;

fn evalTitleWithRetry(browser: *Browser, tr: *WsTcpTransport, session_id: []const u8, cmd_buf: []u8): []const u8! {
    let eval_try: usize = 0;
    while (eval_try < 10) : (eval_try += 1) {
        const id_eval = browser.endpoint.sendCommand(
            session_id,
            "Runtime.evaluate",
            "{\"expression\":\"document.title\",\"returnByValue\":true}",
            cmd_buf,
        )?;
        const resp_eval = waitForCdpResponse(browser, tr, id_eval)?;
        expectTrue(!resp_eval.is_error, "Runtime.evaluate ok")?;
        if (std.mem.indexOf(u8, resp_eval.result_json, "CP Playwright Chromium 158") != null) {
            std.debug.print("[REAL-CDP] Runtime.evaluate(document.title) => {s}\n", .{resp_eval.result_json});
            return "CP Playwright Chromium 158";
        }
        const ts = LinuxTimespec{ sec: 0, nsec: 50_000_000 };
        * = std.os.linux.nanosleep(&ts, null);
    }
    return "";
}

fn runRealCdpTest(alloc: std.mem.Allocator): !void {
    std.debug.print("[REAL-CDP] start\n", .{});

    // 1. Probe real Chromium executable on CentOS Stream 10
    const probed_cdp = CdpBrowserLauncher.probeBrowserExecutable(alloc, "") orelse return error.ProbeFailed;
    defer alloc.free(probed_cdp);
    std.debug.print("[REAL-CDP] CdpBrowserLauncher.probeBrowserExecutable => {s}\n", .{probed_cdp});
    expectTrue(std.mem.eql(u8, probed_cdp, "/usr/bin/chromium-browser"), "probeBrowserExecutable == /usr/bin/chromium-browser")?;

    const probed_pw = chromium.executablePath(alloc, "") orelse return error.PwProbeFailed;
    defer alloc.free(probed_pw);
    std.debug.print("[REAL-CDP] chromium.executablePath() => {s}\n", .{probed_pw});
    expectTrue(std.mem.eql(u8, probed_pw, "/usr/bin/chromium-browser"), "chromium.executablePath == /usr/bin/chromium-browser")?;

    // 2. Real Chromium 158 HTTP /json/version, /json/list & WebSocket CDP E2E
    const port: u16 = 9224;

    // 2a. HTTP GET /json/version
    const ver_buf = alloc.alloc(u8, 8192)?;
    defer alloc.free(ver_buf);
    const ver_body = httpGetLoopback(port, "/json/version", ver_buf)?;
    const browser_ver = CdpFrameScanner.extractTopLevelField(ver_body, "Browser", true) orelse "";
    const proto_ver = CdpFrameScanner.extractTopLevelField(ver_body, "Protocol-Version", true) orelse "";
    const v8_ver = CdpFrameScanner.extractTopLevelField(ver_body, "V8-Version", true) orelse "";
    const ws_url = CdpFrameScanner.extractTopLevelField(ver_body, "webSocketDebuggerUrl", true) orelse "";

    std.debug.print("[REAL-CDP] /json/version Browser={s} Protocol={s} V8={s}\n", .{ browser_ver, proto_ver, v8_ver });
    std.debug.print("[REAL-CDP] /json/version webSocketDebuggerUrl={s}\n", .{ws_url});
    expectTrue(std.mem.indexOf(u8, browser_ver, "158.0.8127.88") != null, "Browser version contains 158.0.8127.88")?;
    expectTrue(proto_ver.len > 0, "Protocol-Version non-empty")?;
    expectTrue(ws_url.len > 0, "webSocketDebuggerUrl non-empty")?;

    // 2b. HTTP GET /json/list
    const list_buf = alloc.alloc(u8, 8192)?;
    defer alloc.free(list_buf);
    const list_body = httpGetLoopback(port, "/json/list", list_buf)?;
    std.debug.print("[REAL-CDP] /json/list bytes={d}\n", .{list_body.len});
    expectTrue(std.mem.indexOf(u8, list_body, "webSocketDebuggerUrl") != null, "/json/list contains webSocketDebuggerUrl")?;

    // 2c. Parse ws:// endpoint and perform RFC 6455 WebSocket handshake
    const parsed_ws = CdpBrowserLauncher.parseWsUrl(ws_url) orelse return error.InvalidWsUrl;
    const ws_fd = connectLoopbackTcp(parsed_ws.port)?;
    defer * = std.c.close(ws_fd);
    performWsHandshake(ws_fd, parsed_ws.path, parsed_ws.port)?;

    // 2d. Connect Playwright Browser via chromium.connectOverCDP with real WS transport
    let tr = alloc.create(WsTcpTransport)?;
    defer alloc.destroy(tr);
    tr.* = WsTcpTransport{ fd: ws_fd };
    let browser = chromium.connectOverCDP(alloc, ws_url, .{
        transport_fn: wsTransportSend,
        transport_ctx: unsafe { @intFromPtr(tr) },
    })?;
    defer browser.deinit();
    expectTrue(browser.isConnected(), "browser.isConnected() == true")?;

    const cmd_buf = alloc.alloc(u8, 4096)?;
    defer alloc.free(cmd_buf);

    // 2e. Browser.getVersion
    const id_ver = browser.endpoint.sendCommand("", "Browser.getVersion", "{}", cmd_buf)?;
    const resp_ver = waitForCdpResponse(&browser, tr, id_ver)?;
    expectTrue(!resp_ver.is_error, "Browser.getVersion ok")?;
    const cdp_product = CdpFrameScanner.extractTopLevelField(resp_ver.result_json, "product", true) orelse "";
    const cdp_js_ver = CdpFrameScanner.extractTopLevelField(resp_ver.result_json, "jsVersion", true) orelse "";
    std.debug.print("[REAL-CDP] Browser.getVersion => product={s} jsVersion={s}\n", .{ cdp_product, cdp_js_ver });
    expectTrue(std.mem.indexOf(u8, cdp_product, "158.0.8127.88") != null, "Browser.getVersion product contains 158.0.8127.88")?;

    // 2f. Target.createTarget
    const id_create = browser.endpoint.sendCommand("", "Target.createTarget", "{\"url\":\"about:blank\"}", cmd_buf)?;
    const resp_create = waitForCdpResponse(&browser, tr, id_create)?;
    expectTrue(!resp_create.is_error, "Target.createTarget ok")?;
    const target_id = CdpFrameScanner.extractTopLevelField(resp_create.result_json, "targetId", true) orelse "";
    std.debug.print("[REAL-CDP] Target.createTarget => targetId={s}\n", .{target_id});
    expectTrue(target_id.len > 0, "targetId non-empty")?;

    // 2g. Target.attachToTarget (flatten: true)
    let attach_params_buf: [256]u8 = undefined;
    const attach_params = std.fmt.bufPrint(&attach_params_buf, "{{\"targetId\":\"{s}\",\"flatten\":true}}", .{target_id}) catch return error.BufferTooSmall;
    const id_attach = browser.endpoint.sendCommand("", "Target.attachToTarget", attach_params, cmd_buf)?;
    const resp_attach = waitForCdpResponse(&browser, tr, id_attach)?;
    expectTrue(!resp_attach.is_error, "Target.attachToTarget ok")?;
    const session_id = CdpFrameScanner.extractTopLevelField(resp_attach.result_json, "sessionId", true) orelse "";
    std.debug.print("[REAL-CDP] Target.attachToTarget => sessionId={s}\n", .{session_id});
    expectTrue(session_id.len > 0, "sessionId non-empty")?;

    browser.endpoint.createSession(session_id, "page", target_id, "")?;

    // 2h. Page.enable + Page.navigate to data:text/html
    const id_enable = browser.endpoint.sendCommand(session_id, "Page.enable", "{}", cmd_buf)?;
    const resp_enable = waitForCdpResponse(&browser, tr, id_enable)?;
    expectTrue(!resp_enable.is_error, "Page.enable ok")?;

    const nav_params = "{\"url\":\"data:text/html,%3Chtml%3E%3Chead%3E%3Ctitle%3ECP%20Playwright%20Chromium%20158%3C%2Ftitle%3E%3C%2Fhead%3E%3Cbody%3Ehello%3C%2Fbody%3E%3C%2Fhtml%3E\"}";
    const id_nav = browser.endpoint.sendCommand(session_id, "Page.navigate", nav_params, cmd_buf)?;
    const resp_nav = waitForCdpResponse(&browser, tr, id_nav)?;
    expectTrue(!resp_nav.is_error, "Page.navigate ok")?;
    const frame_id = CdpFrameScanner.extractTopLevelField(resp_nav.result_json, "frameId", true) orelse "";
    std.debug.print("[REAL-CDP] Page.navigate => frameId={s}\n", .{frame_id});
    expectTrue(frame_id.len > 0, "frameId non-empty")?;

    // 2i. Runtime.evaluate document.title
    const title_val = evalTitleWithRetry(&browser, tr, session_id, cmd_buf)?;
    expectTrue(std.mem.eql(u8, title_val, "CP Playwright Chromium 158"), "document.title == CP Playwright Chromium 158")?;

    // 2j. Target.closeTarget + Browser.close
    let close_t_buf: [256]u8 = undefined;
    const close_t_params = std.fmt.bufPrint(&close_t_buf, "{{\"targetId\":\"{s}\"}}", .{target_id}) catch return error.BufferTooSmall;
    const id_close_t = browser.endpoint.sendCommand("", "Target.closeTarget", close_t_params, cmd_buf)?;
    const resp_close_t = waitForCdpResponse(&browser, tr, id_close_t)?;
    expectTrue(!resp_close_t.is_error, "Target.closeTarget ok")?;
    std.debug.print("[REAL-CDP] Target.closeTarget => {s}\n", .{resp_close_t.result_json});

    const id_close_b = browser.endpoint.sendCommand("", "Browser.close", "{}", cmd_buf)?;
    const resp_close_b = waitForCdpResponse(&browser, tr, id_close_b)?;
    expectTrue(!resp_close_b.is_error, "Browser.close ok")?;
    std.debug.print("[REAL-CDP] Browser.close => {s}\n", .{resp_close_b.result_json});

    let ws_close_buf: [64]u8 = undefined;
    const close_mask: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    const close_payload: [2]u8 = [0x03, 0xe8];
    const ws_close_len = buildMaskedWsFrame(&ws_close_buf, 0x8, &close_payload, close_mask) catch 0;
    if (ws_close_len > 0) {
        if (!writeAllFd(ws_fd, ws_close_buf[0..ws_close_len])) {}
    }
    browser.close();
    std.debug.print("[REAL-CDP] ALL REAL CHROMIUM 158 CDP CHECKS PASSED\n", .{});
}

fn main(): !void {
    runRealCdpTest(std.heap.page_allocator)?;
}
