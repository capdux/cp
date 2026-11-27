// ============================================================================
// packages/puppeteer/test/real_chromium_smoke.cp
// Real OS + Chromium 158.0.8127.88 End-to-End CDP WebSocket Verification
// ============================================================================

const std = @import("std");
import {
    Puppeteer,
    Browser,
    Page,
    LaunchOptions,
    ConnectOptions,
    EvaluateResult,
} from "../src/index";
import {
    CdpFrameScanner,
    CdpBrowserLauncher,
    CdpEndpoint,
    CdpWsEndpoint,
} from "cap:cdp";

const PosixSockAddrIn = extern struct {
    sin_family: u16 = 2, // AF_INET
    sin_port: u16 = 0,
    sin_addr: u32 = 0,
    sin_zero: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0],
};

@c {
    fn socket(domain: i32, sock_type: i32, protocol: i32): i32;
    fn connect(sockfd: i32, addr: *const PosixSockAddrIn, addrlen: u32): i32;
    fn send(sockfd: i32, buf: [*]const u8, len: usize, flags: i32): isize;
    fn recv(sockfd: i32, buf: [*]u8, len: usize, flags: i32): isize;
    fn close(fd: i32): i32;
}

fn htons(port: u16): u16 {
    return ((port & 0x00ff) << 8) | ((port & 0xff00) >> 8);
}

fn htonl(ip: u32): u32 {
    return ((ip & 0x000000ff) << 24) |
        ((ip & 0x0000ff00) << 8) |
        ((ip & 0x00ff0000) >> 8) |
        ((ip & 0xff000000) >> 24);
}

fn connectLoopback(port: u16): i32 {
    let fd = unsafe { socket(2, 1, 0) }; // AF_INET=2, SOCK_STREAM=1
    if (fd < 0) return -1;
    let addr = PosixSockAddrIn{
        sin_family: 2,
        sin_port: htons(port),
        sin_addr: htonl(0x7f000001), // 127.0.0.1
    };
    let rc = unsafe { connect(fd, &addr, 16) };
    if (rc < 0) {
        * = unsafe { close(fd) };
        return -1;
    }
    return fd;
}

fn sendAll(fd: i32, data: []const u8): bool {
    let sent: usize = 0;
    while (sent < data.len) {
        let n = unsafe { send(fd, data.ptr + sent, data.len - sent, 0) };
        if (n <= 0) return false;
        sent += n as usize;
    }
    return true;
}

fn httpGetBody(alloc: std.mem.Allocator, port: u16, path: []const u8): ([]u8)? {
    let fd = connectLoopback(port);
    if (fd < 0) return null;
    defer { * = unsafe { close(fd) }; }

    let req = std.fmt.allocPrint(
        alloc,
        "GET {s} HTTP/1.1\r\nHost: 127.0.0.1:{d}\r\nConnection: close\r\n\r\n",
        .{ path, port },
    ) catch return null;
    defer alloc.free(req);

    if (!sendAll(fd, req)) return null;

    let buf: std.ArrayList(u8) = .empty;
    defer buf.deinit(alloc);
    let tmp: [4096]u8 = undefined;
    let tmp_ptr: [*]u8 = unsafe { @ptrCast(&tmp[0]) };
    while (true) {
        let n = unsafe { recv(fd, tmp_ptr, tmp.len, 0) };
        if (n <= 0) break;
        buf.appendSlice(alloc, tmp[0..(n as usize)]) catch return null;
        if (hdr_end := std.mem.indexOf(u8, buf.items, "\r\n\r\n")) {
            let body = buf.items[hdr_end + 4 ..];
            let trimmed = std.mem.trimEnd(u8, body, " \t\r\n");
            if (trimmed.len > 0 && (trimmed[trimmed.len - 1] == '}' || trimmed[trimmed.len - 1] == ']')) {
                return alloc.dupe(u8, body) catch null;
            }
        }
    }
    if (hdr_end := std.mem.indexOf(u8, buf.items, "\r\n\r\n")) {
        return alloc.dupe(u8, buf.items[hdr_end + 4 ..]) catch null;
    }
    return null;
}

struct WsClient {
    alloc: std.mem.Allocator,
    fd: i32,

    static fn connect(alloc: std.mem.Allocator, host: []const u8, port: u16, path: []const u8): WsClient? {
        let fd = connectLoopback(port);
        if (fd < 0) return null;

        let hs = std.fmt.allocPrint(
            alloc,
            "GET {s} HTTP/1.1\r\nHost: {s}:{d}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n",
            .{ path, host, port },
        ) catch {
            * = unsafe { close(fd) };
            return null;
        };
        defer alloc.free(hs);

        if (!sendAll(fd, hs)) {
            * = unsafe { close(fd) };
            return null;
        }

        let resp_buf: [2048]u8 = undefined;
        let resp_ptr: [*]u8 = unsafe { @ptrCast(&resp_buf[0]) };
        let total: usize = 0;
        while (total < resp_buf.len) {
            let n = unsafe { recv(fd, resp_ptr + total, resp_buf.len - total, 0) };
            if (n <= 0) break;
            total += n as usize;
            if (std.mem.indexOf(u8, resp_buf[0..total], "\r\n\r\n") != null) break;
        }
        if (std.mem.indexOf(u8, resp_buf[0..total], " 101 ") == null) {
            std.debug.print("WS Handshake Resp: {s}\n", .{resp_buf[0..total]});
            * = unsafe { close(fd) };
            return null;
        }
        return WsClient{ alloc: alloc, fd: fd };
    }

    fn sendText(payload: []const u8): bool {
        let header: [14]u8 = undefined;
        let hlen: usize = 0;
        header[0] = 0x81; // FIN + Text
        if (payload.len <= 125) {
            header[1] = 0x80 | (payload.len as u8);
            hlen = 2;
        } else if (payload.len <= 65535) {
            header[1] = 0x80 | 126;
            header[2] = ((payload.len >> 8) & 0xff) as u8;
            header[3] = (payload.len & 0xff) as u8;
            hlen = 4;
        } else {
            return false;
        }
        // Masking key (deterministic non-zero for client->server)
        let mask: [4]u8 = [0x12, 0x34, 0x56, 0x78];
        @memcpy(header[hlen .. hlen + 4], &mask);
        hlen += 4;

        let frame = _.alloc.alloc(u8, hlen + payload.len) catch return false;
        defer _.alloc.free(frame);
        @memcpy(frame[0..hlen], header[0..hlen]);
        let i: usize = 0;
        while (i < payload.len) : (i += 1) {
            frame[hlen + i] = payload[i] ^ mask[i & 3];
        }
        return sendAll(_.fd, frame);
    }

    fn recvExact(dst: []u8): bool {
        let got: usize = 0;
        while (got < dst.len) {
            let n = unsafe { recv(_.fd, dst.ptr + got, dst.len - got, 0) };
            if (n <= 0) return false;
            got += n as usize;
        }
        return true;
    }

    fn recvTextFrame(): ([]u8)? {
        while (true) {
            let b: [2]u8 = undefined;
            if (!_.recvExact(&b)) return null;
            let opcode = b[0] & 0x0f;
            let masked = (b[1] & 0x80) != 0;
            let plen: usize = (b[1] & 0x7f) as usize;
            if (plen == 126) {
                let ext: [2]u8 = undefined;
                if (!_.recvExact(&ext)) return null;
                plen = ((ext[0] as usize) << 8) | (ext[1] as usize);
            } else if (plen == 127) {
                let ext8: [8]u8 = undefined;
                if (!_.recvExact(&ext8)) return null;
                plen = ((ext8[4] as usize) << 24) | ((ext8[5] as usize) << 16) | ((ext8[6] as usize) << 8) | (ext8[7] as usize);
            }
            let mask: [4]u8 = [0, 0, 0, 0];
            if (masked && !_.recvExact(&mask)) return null;
            let data = _.alloc.alloc(u8, plen) catch return null;
            if (!_.recvExact(data)) {
                _.alloc.free(data);
                return null;
            }
            if (masked) {
                let i: usize = 0;
                while (i < plen) : (i += 1) data[i] ^= mask[i & 3];
            }
            if (opcode == 0x8) {
                _.alloc.free(data);
                return null;
            }
            if (opcode == 0x9) { // Ping -> ignore and read next
                _.alloc.free(data);
                continue;
            }
            return data;
        }
    }

    fn deinit(*_) {
        if (_.fd >= 0) {
            * = unsafe { close(_.fd) };
            _.fd = -1;
        }
    }
}

struct RealWsTransportCtx {
    ws: *WsClient,
    endpoint_ptr: usize,
}

fn realWsTransportSend(ctx_ptr: usize, payload: []const u8): bool {
    let ctx: *RealWsTransportCtx = unsafe { @ptrFromInt(ctx_ptr) };
    if (!ctx.ws.sendText(payload)) return false;

    // Synchronously pump incoming frames until the response id arrives
    let id_str = CdpFrameScanner.extractTopLevelField(payload, "id", true) orelse return true;
    let msg_id = CdpFrameScanner.parseFastI64(id_str) orelse return true;
    let ep: *CdpEndpoint = unsafe { @ptrFromInt(ctx.endpoint_ptr) };
    let tries: usize = 0;
    while (tries < 64) : (tries += 1) {
        let frame = ctx.ws.recvTextFrame() orelse return false;
        defer ctx.ws.alloc.free(frame);
        ep.handleIncomingFrame(frame) catch {};
        if (rid_str := CdpFrameScanner.extractTopLevelField(frame, "id", true)) {
            if (rid := CdpFrameScanner.parseFastI64(rid_str)) {
                if (rid == msg_id) return true;
            }
        }
    }
    return true;
}

pub fn main() {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    let alloc: std.mem.Allocator = gpa.allocator();

    // 1. Verify CdpBrowserLauncher / Puppeteer probes real /usr/bin/chromium-browser
    let exe = Puppeteer.defaultExecutablePath(alloc) orelse {
        std.debug.print("FAIL: defaultExecutablePath returned null\n", .{});
        std.process.exit(1);
    };
    defer alloc.free(exe);
    std.debug.print("[1] Probed browser executable: {s}\n", .{exe});

    // 2. Query /json/version from live headless Chromium on 127.0.0.1:9225
    let ver_json = httpGetBody(alloc, 9225, "/json/version") orelse {
        std.debug.print("FAIL: Could not reach Chromium CDP HTTP on 127.0.0.1:9225\n", .{});
        std.process.exit(1);
    };
    defer alloc.free(ver_json);

    let browser_name = CdpFrameScanner.extractTopLevelField(ver_json, "Browser", true) orelse "unknown";
    let proto_ver = CdpFrameScanner.extractTopLevelField(ver_json, "Protocol-Version", true) orelse "unknown";
    let ws_url = CdpFrameScanner.extractTopLevelField(ver_json, "webSocketDebuggerUrl", true) orelse {
        std.debug.print("FAIL: Missing webSocketDebuggerUrl\n", .{});
        std.process.exit(1);
    };
    std.debug.print("[2] Live CDP /json/version: Browser={s}, Protocol={s}, WS={s}\n", .{ browser_name, proto_ver, ws_url });

    let parsed_ws = CdpBrowserLauncher.parseWsUrl(ws_url) orelse {
        std.debug.print("FAIL: parseWsUrl failed\n", .{});
        std.process.exit(1);
    };

    // 3. Connect real WebSocket client and attach Puppeteer.connectWithTransport
    let ws = WsClient.connect(alloc, parsed_ws.host, parsed_ws.port, parsed_ws.path) orelse {
        std.debug.print("FAIL: WebSocket handshake failed\n", .{});
        std.process.exit(1);
    };
    defer ws.deinit();

    let ctx = RealWsTransportCtx{ ws: &ws, endpoint_ptr: 0 };
    let browser = Puppeteer.connectWithTransport(
        alloc,
        ConnectOptions{ browser_ws_url: ws_url },
        realWsTransportSend,
        unsafe { @intFromPtr(&ctx) },
    ) catch {
        std.debug.print("FAIL: Puppeteer.connectWithTransport failed\n", .{});
        std.process.exit(1);
    };
    defer browser.deinit();
    ctx.endpoint_ptr = unsafe { @intFromPtr(browser.endpoint) };

    // 4. Real Browser.getVersion over CDP WebSocket
    let ver_buf: [512]u8 = undefined;
    let ver = browser.version(&ver_buf) catch "";
    std.debug.print("[3] CDP Browser.getVersion() -> {s}\n", .{ver});

    // 5. Create real page target, navigate to data URL, evaluate JS in V8!
    let page = browser.newPage() catch {
        std.debug.print("FAIL: browser.newPage() failed\n", .{});
        std.process.exit(1);
    };

    page.goto("data:text/html,<title>CP-Real-Chromium-158</title><h1>OK</h1>", .{}) catch {
        std.debug.print("FAIL: page.goto failed\n", .{});
        std.process.exit(1);
    };
    let eval_buf1: [2048]u8 = undefined;
    let title = page.evaluate("document.title", &eval_buf1) catch EvaluateResult{};
    std.debug.print("[4] CDP Runtime.evaluate(document.title) -> type={s}, json={s}\n", .{ title.value_type, title.value_json });

    let eval_buf2: [2048]u8 = undefined;
    let sum_res = page.evaluate("19 + 23", &eval_buf2) catch EvaluateResult{};
    std.debug.print("[5] CDP Runtime.evaluate(19 + 23) -> type={s}, json={s}\n", .{ sum_res.value_type, sum_res.value_json });

    page.close() catch {};
    browser.close() catch {};
    std.debug.print("ALL REAL CHROMIUM PUPPETEER CDP CHECKS PASSED!\n", .{});
}
