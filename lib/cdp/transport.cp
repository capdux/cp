// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/node/NodeWebSocketTransport.ts & _ref/playwright/packages/playwright-core/src/server/chromium/crConnection.ts | Synced: 2026-11-28
// ============================================================================
// lib/cdp/transport.cp — Built-In Native POSIX TCP + RFC 6455 WS Transport
// Enables zero-glue one-line Browser.launch() & connectOverCDP()
// ============================================================================

const std = @import("std");
import { CdpError, CdpFrameScanner } from "./protocol";
import { CdpEndpoint } from "./session";
import { CdpBrowserLaunchOptions, CdpWsEndpoint, CdpBrowserLauncher } from "./launcher";

const CdpSockAddrIn = extern struct {
    sin_family: u16 = 2,
    sin_port: u16 = 0,
    sin_addr: u32 = 0,
    sin_zero: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0],
};

@c {
    fn socket(domain: i32, sock_type: i32, protocol: i32): i32;
    fn connect(sockfd: i32, addr: *const CdpSockAddrIn, addrlen: u32): i32;
    fn send(sockfd: i32, buf: [*]const u8, len: usize, flags: i32): isize;
    fn recv(sockfd: i32, buf: [*]u8, len: usize, flags: i32): isize;
    fn fork(): i32;
    fn execvp(file: [*:0]const u8, argv: [*:null]const ([*:0]const u8)?): i32;
    fn _exit(status: i32);
    fn kill(pid: i32, sig: i32): i32;
    fn waitpid(pid: i32, status: *i32, options: i32): i32;
    fn usleep(usec: u32): i32;
}

fn cdpHtons(port: u16): u16 {
    return ((port & 0x00ff) << 8) | ((port & 0xff00) >> 8);
}

fn cdpHtonl(ip: u32): u32 {
    return ((ip & 0x000000ff) << 24) |
        ((ip & 0x0000ff00) << 8) |
        ((ip & 0x00ff0000) >> 8) |
        ((ip & 0xff000000) >> 24);
}

fn cdpConnectLoopback(port: u16): i32 {
    let fd = unsafe { socket(2, 1, 0) };
    if (fd < 0) return -1;
    let addr = CdpSockAddrIn{
        sin_family: 2,
        sin_port: cdpHtons(port),
        sin_addr: cdpHtonl(0x7f000001),
    };
    if (unsafe { connect(fd, &addr, 16) } < 0) {
        * = std.c.close(fd);
        return -1;
    }
    return fd;
}

fn cdpSendAll(fd: i32, data: []const u8): bool {
    let sent: usize = 0;
    while (sent < data.len) {
        let n = unsafe { send(fd, data.ptr + sent, data.len - sent, 0) };
        if (n <= 0) return false;
        sent += n as usize;
    }
    return true;
}

/// Native POSIX TCP + RFC 6455 WebSocket Transport owning optional browser child PID.
pub struct CdpNativeWsTransport {
    alloc: std.mem.Allocator,
    fd: i32 = -1,
    child_pid: i32 = -1,
    endpoint: *CdpEndpoint = undefined,
    has_endpoint: bool = false,
    ws_url_buf: [512]u8 = undefined,
    ws_url_len: usize = 0,

    pub static fn init(alloc: std.mem.Allocator): CdpNativeWsTransport {
        return CdpNativeWsTransport{
            alloc: alloc,
            fd: -1,
            child_pid: -1,
            has_endpoint: false,
            ws_url_len: 0,
        };
    }

    pub fn wsUrl(): []const u8 {
        return _.ws_url_buf[0.._.ws_url_len];
    }

    pub fn isConnected(): bool {
        return _.fd >= 0;
    }

    pub fn attachEndpoint(*_, ep: *CdpEndpoint) {
        _.endpoint = ep;
        _.has_endpoint = true;
        ep.setTransport(CdpNativeWsTransport.transportSendCb, unsafe { @intFromPtr(_) });
        ep.markConnected();
    }

    /// HTTP GET /json/version on loopback port to discover live WebSocket debugger URL.
    pub static fn fetchWsUrlFromPort(alloc: std.mem.Allocator, port: u16): ([]u8)? {
        let fd = cdpConnectLoopback(port);
        if (fd < 0) return null;
        defer { * = std.c.close(fd); }

        let req = std.fmt.allocPrint(
            alloc,
            "GET /json/version HTTP/1.1\r\nHost: 127.0.0.1:{d}\r\nConnection: close\r\n\r\n",
            .{port},
        ) catch return null;
        defer alloc.free(req);
        if (!cdpSendAll(fd, req)) return null;

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
                    if (ws := CdpFrameScanner.extractTopLevelField(body, "webSocketDebuggerUrl", true)) {
                        return alloc.dupe(u8, ws) catch null;
                    }
                    return null;
                }
            }
        }
        return null;
    }

    /// Connect RFC 6455 WebSocket to an existing `ws://host:port/path` endpoint.
    pub fn connectWs(*_, ws_url: []const u8): void!CdpError {
        const parsed = CdpBrowserLauncher.parseWsUrl(ws_url) orelse return CdpError.InvalidFrame;
        let fd = cdpConnectLoopback(parsed.port);
        if (fd < 0) return CdpError.TransportDisconnected;

        let hs = std.fmt.allocPrint(
            _.alloc,
            "GET {s} HTTP/1.1\r\nHost: {s}:{d}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n",
            .{ parsed.path, parsed.host, parsed.port },
        ) catch {
            * = std.c.close(fd);
            return CdpError.OutOfMemory;
        };
        defer _.alloc.free(hs);

        if (!cdpSendAll(fd, hs)) {
            * = std.c.close(fd);
            return CdpError.TransportSendFailed;
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
            * = std.c.close(fd);
            return CdpError.TransportDisconnected;
        }

        _.fd = fd;
        const wn = @min(ws_url.len, _.ws_url_buf.len);
        if (wn > 0) @memcpy(_.ws_url_buf[0..wn], ws_url[0..wn]);
        _.ws_url_len = wn;
    }

    /// Spawn Chromium/Chrome binary with CDP debugging port and connect WebSocket automatically.
    pub fn launchAndConnect(*_, opts: CdpBrowserLaunchOptions): void!CdpError {
        const exe = CdpBrowserLauncher.probeBrowserExecutable(_.alloc, opts.executable_path) orelse
            return CdpError.ProtocolError;
        defer _.alloc.free(exe);

        const port = if (opts.remote_debugging_port == 0) 9229 else opts.remote_debugging_port;
        const launch_opts = CdpBrowserLaunchOptions{
            headless: opts.headless,
            executable_path: opts.executable_path,
            user_data_dir: opts.user_data_dir,
            args: opts.args,
            viewport_width: opts.viewport_width,
            viewport_height: opts.viewport_height,
            timeout_ms: opts.timeout_ms,
            remote_debugging_port: port,
            ignore_https_errors: opts.ignore_https_errors,
        };
        const args = CdpBrowserLauncher.buildLaunchArgs(_.alloc, launch_opts)?;
        defer CdpBrowserLauncher.freeLaunchArgs(_.alloc, args);

        // Build null-terminated argv for execvp: [exe, args..., "--remote-allow-origins=*", null]
        let c_strs: std.ArrayList(([*:0]const u8)?) = .empty;
        defer c_strs.deinit(_.alloc);
        let owned_z: std.ArrayList([:0]u8) = .empty;
        defer {
            for (z of owned_z.items) { _.alloc.free(z); }
            owned_z.deinit(_.alloc);
        }

        const exe_z = _.alloc.dupeZ(u8, exe) catch return CdpError.OutOfMemory;
        owned_z.append(_.alloc, exe_z) catch return CdpError.OutOfMemory;
        c_strs.append(_.alloc, exe_z.ptr) catch return CdpError.OutOfMemory;

        for (a of args) {
            const az = _.alloc.dupeZ(u8, a) catch return CdpError.OutOfMemory;
            owned_z.append(_.alloc, az) catch return CdpError.OutOfMemory;
            c_strs.append(_.alloc, az.ptr) catch return CdpError.OutOfMemory;
        }
        const allow_z = _.alloc.dupeZ(u8, "--remote-allow-origins=*") catch return CdpError.OutOfMemory;
        owned_z.append(_.alloc, allow_z) catch return CdpError.OutOfMemory;
        c_strs.append(_.alloc, allow_z.ptr) catch return CdpError.OutOfMemory;
        c_strs.append(_.alloc, null) catch return CdpError.OutOfMemory;

        const pid = unsafe { fork() };
        if (pid < 0) return CdpError.TransportSendFailed;
        if (pid == 0) {
            const argv_ptr: [*:null]const ([*:0]const u8)? = unsafe { @ptrCast(c_strs.items.ptr) };
            * = unsafe { execvp(exe_z.ptr, argv_ptr) };
            unsafe { _exit(127) }
        }
        _.child_pid = pid;

        // Poll /json/version up to 5 seconds (100 * 50ms)
        let tries: usize = 0;
        let discovered_ws: ([]u8)? = null;
        while (tries < 100) : (tries += 1) {
            if (ws := CdpNativeWsTransport.fetchWsUrlFromPort(_.alloc, port)) {
                discovered_ws = ws;
                break;
            }
            * = unsafe { usleep(50000) };
        }
        const ws_str = discovered_ws orelse {
            _.close();
            return CdpError.Timeout;
        };
        defer _.alloc.free(ws_str);

        _.connectWs(ws_str)?;
    }

    pub fn sendTextFrame(payload: []const u8): bool {
        if (_.fd < 0) return false;
        let header: [14]u8 = undefined;
        let hlen: usize = 0;
        header[0] = 0x81;
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
        return cdpSendAll(_.fd, frame);
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

    pub fn recvTextFrame(): ([]u8)? {
        if (_.fd < 0) return null;
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
            if (opcode == 0x9) {
                _.alloc.free(data);
                continue;
            }
            return data;
        }
    }

    pub static fn transportSendCb(ctx_ptr: usize, payload: []const u8): bool {
        let self_ptr: *CdpNativeWsTransport = unsafe { @ptrFromInt(ctx_ptr) };
        if (!self_ptr.sendTextFrame(payload)) return false;
        if (!self_ptr.has_endpoint) return true;

        let id_str = CdpFrameScanner.extractTopLevelField(payload, "id", true) orelse return true;
        let msg_id = CdpFrameScanner.parseFastI64(id_str) orelse return true;
        let tries: usize = 0;
        while (tries < 64) : (tries += 1) {
            let frame = self_ptr.recvTextFrame() orelse return false;
            defer self_ptr.alloc.free(frame);
            self_ptr.endpoint.handleIncomingFrame(frame) catch {};
            if (rid_str := CdpFrameScanner.extractTopLevelField(frame, "id", true)) {
                if (rid := CdpFrameScanner.parseFastI64(rid_str)) {
                    if (rid == msg_id) return true;
                }
            }
        }
        return true;
    }

    pub fn close(*_) {
        if (_.fd >= 0) {
            * = std.c.close(_.fd);
            _.fd = -1;
        }
        if (_.child_pid > 0) {
            * = unsafe { kill(_.child_pid, 9) };
            let st: i32 = 0;
            * = unsafe { waitpid(_.child_pid, &st, 0) };
            _.child_pid = -1;
        }
    }

    pub fn deinit(*_) {
        _.close();
    }
}
