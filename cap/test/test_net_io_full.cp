// cap/test/test_net_io_full.cp
// ============================================================================
// Full Regression Suite for CP Standard Library:
//   cap:net, cap:ws, cap:router, cap:io, cap:sys
// Offline-only, zero external network dependency.
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    Ipv4Addr,
    Ipv6Addr,
    SocketAddress,
    SafeSocket,
    SocketKind,
    SocketCloseCode,
    SendStatus,
    ListenConfig,
    TcpServer,
    UdpSocket,
    HttpConnectTunnel,
    Socks5AddrType,
    Socks5Codec,
    LIBUS_LISTEN_REUSE_PORT,
    LIBUS_LISTEN_REUSE_ADDR,
} from "cap:net";

import {
    Opcode,
    CloseCode,
    FrameHeader,
    FrameBuilder,
    Mask,
} from "cap:ws";

import {
    Router,
    ParamMap,
    RouteFormatter,
} from "cap:router";

import {
    SeekOrigin,
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    copyStream,
    copyN,
    MaxBuf,
    MaxBufKind,
    Pipe,
} from "cap:io";

import {
    FD,
    File,
    Stat,
    stat,
    fstat,
    copyFile,
    Signal,
    fromNumber,
    fromName,
    getTempDir,
    makeTempFile,
    makeTempDir,
    removeFile,
    removeDir,
} from "cap:sys";

// ============================================================================
// 1. cap:net — Address, Socket, TCP Corking, UDP, HTTP CONNECT & SOCKS5 Proxy
// ============================================================================
fn testNetAddressAndProxy(): !void {
    // IPv4 parse & classification
    const priv_ip = Ipv4Addr.parse("10.24.0.88");
    expect_eq(priv_ip != null, true);
    expect_eq(priv_ip.?.isPrivate(), true);
    expect_eq(priv_ip.?.isLoopback(), false);
    expect_eq(priv_ip.?.toU32(), 0x0A180058);

    const roundtrip = Ipv4Addr.fromU32(0xC0A80105);
    expect_eq(roundtrip.octets[0], 192);
    expect_eq(roundtrip.octets[1], 168);
    expect_eq(roundtrip.octets[2], 1);
    expect_eq(roundtrip.octets[3], 5);
    expect_eq(roundtrip.isPrivate(), true);

    const mcast = Ipv4Addr.parse("224.0.0.251");
    expect_eq(mcast != null, true);
    expect_eq(mcast.?.isMulticast(), true);

    expect_eq(Ipv4Addr.parse("999.0.0.1") == null, true);
    expect_eq(Ipv4Addr.parse("127.0.0") == null, true);

    // IPv6 & SocketAddress
    const mapped = Ipv6Addr.fromIpv4Mapped(Ipv4Addr.loopback());
    expect_eq(mapped.isIpv4Mapped(), true);
    expect_eq(mapped.isLoopback(), false);
    expect_eq(Ipv6Addr.loopback().isLoopback(), true);

    const sa4 = SocketAddress.loopbackV4(8443);
    expect_eq(sa4.isIPv6(), false);
    expect_eq(sa4.socklen(), 16);
    expect_eq(sa4.port, 8443);

    const sa6 = SocketAddress.loopbackV6(8443);
    expect_eq(sa6.isIPv6(), true);
    expect_eq(sa6.socklen(), 28);

    // HTTP CONNECT proxy tunnel codec
    let connect_buf: [256]u8 = [0 as u8; 256];
    const req_len = HttpConnectTunnel.buildConnectRequest(connect_buf[0..256], "api.cp-lang.org", 443);
    const req_str = connect_buf[0..req_len];
    expect_eq(std.mem.startsWith(u8, req_str, "CONNECT api.cp-lang.org:443 HTTP/1.1\r\n"), true);
    expect_eq(std.mem.endsWith(u8, req_str, "\r\n\r\n"), true);

    const code200 = HttpConnectTunnel.parseConnectResponse("HTTP/1.1 200 Connection Established\r\n\r\n");
    expect_eq(code200 != null, true);
    expect_eq(code200.?, 200);
    expect_eq(HttpConnectTunnel.isTunnelEstablished("HTTP/1.1 200 Connection Established\r\n\r\n"), true);
    expect_eq(HttpConnectTunnel.isTunnelEstablished("HTTP/1.1 407 Proxy Authentication Required\r\n\r\n"), false);
    expect_eq(HttpConnectTunnel.parseConnectResponse("INVALID") == null, true);

    // SOCKS5 wire codec
    let s5_buf: [64]u8 = [0 as u8; 64];
    const greet_len = Socks5Codec.encodeGreetingNoAuth(s5_buf[0..64]);
    expect_eq(greet_len, 3);
    expect_eq(s5_buf[0], 0x05);
    expect_eq(s5_buf[1], 0x01);
    expect_eq(s5_buf[2], 0x00);

    let ok_greet: [2]u8 = [0x05, 0x00];
    let bad_greet: [2]u8 = [0x05, 0xFF];
    expect_eq(Socks5Codec.verifyGreetingReply(ok_greet[0..2]), true);
    expect_eq(Socks5Codec.verifyGreetingReply(bad_greet[0..2]), false);

    let dst_ip: [4]u8 = [127, 0, 0, 1];
    const v4_len = Socks5Codec.encodeConnectIpv4(s5_buf[0..64], dst_ip, 8080);
    expect_eq(v4_len, 10);
    expect_eq(s5_buf[3], @intFromEnum(Socks5AddrType.Ipv4));
    expect_eq(s5_buf[8], 0x1F); // 8080 >> 8 = 31 (0x1F)
    expect_eq(s5_buf[9], 0x90); // 8080 & 0xFF = 144 (0x90)

    const dom_len = Socks5Codec.encodeConnectDomain(s5_buf[0..64], "cp.dev", 443);
    expect_eq(dom_len, 4 + 1 + 6 + 2);
    expect_eq(s5_buf[3], @intFromEnum(Socks5AddrType.Domain));
    expect_eq(s5_buf[4], 6);
    expect_eq(std.mem.eql(u8, s5_buf[5..11], "cp.dev"), true);

    let ok_reply: [4]u8 = [0x05, 0x00, 0x00, 0x01];
    let err_reply: [4]u8 = [0x05, 0x04, 0x00, 0x01];
    expect_eq(Socks5Codec.verifyConnectReply(ok_reply[0..4]), true);
    expect_eq(Socks5Codec.verifyConnectReply(err_reply[0..4]), false);
}

fn testNetTcpAndUdp(): !void {
    // SafeSocket long-timeout promotion (>240s -> minutes)
    // Note: CP compiler automatically injects RAII defer release() for SafeSocket/TcpServer/TcpStream/UdpSocket
    let sock = SafeSocket.create(42, SocketKind.established_tcp);
    sock.setNoDelay(true);
    expect_eq(sock.nodelay, true);
    expect_eq(sock.setKeepAlive(true, 30), true);
    sock.setTimeout(360);
    expect_eq(sock.timeout_sec, 0);
    expect_eq(sock.long_timeout_min, 6);
    expect_eq(sock.recordWrite(128), SendStatus.success);
    sock.shutdown();
    expect_eq(sock.recordWrite(16), SendStatus.dropped);

    // TcpServer + TcpStream corking / uncorking
    const bind_addr = SocketAddress.loopbackV4(9090);
    const cfg = ListenConfig.defaultForPort(9090);
    expect_eq((cfg.toLibusOptions() & LIBUS_LISTEN_REUSE_PORT) != 0, true);
    expect_eq((cfg.toLibusOptions() & LIBUS_LISTEN_REUSE_ADDR) != 0, true);

    let srv = TcpServer.bind(bind_addr, cfg);
    expect_eq(srv.localPort(), 9090);

    const peer = SocketAddress.parseV4("192.168.10.5", 50000).?;
    const stream_opt = srv.acceptConnection(peer);
    expect_eq(stream_opt != null, true);
    let stream = stream_opt.?;

    stream.cork();
    expect_eq(stream.write("HEAD "), 5);
    expect_eq(stream.write("BODY"), 4);
    expect_eq(stream.send_buf.pendingLen(), 9);
    const flushed = stream.uncork();
    expect_eq(flushed, 9);
    expect_eq(stream.sock.stats.bytes_written, 9);

    expect_eq(stream.feedIncoming("PING"), 4);
    let rx: [8]u8 = [0 as u8; 8];
    const rlen = stream.readAvailable(rx[0..8]);
    expect_eq(rlen, 4);
    expect_eq(std.mem.eql(u8, rx[0..4], "PING"), true);

    // UdpSocket bind, peer connect & datagram counters
    let udp = UdpSocket.bind(SocketAddress.loopbackV4(5353));
    expect_eq(udp.boundPort(), 5353);
    expect_eq(udp.setUnicastTTL(64), true);
    expect_eq(udp.setUnicastTTL(0), false);
    expect_eq(udp.connectPeer(peer), true);
    expect_eq(udp.send("dgram"), 5);
    expect_eq(udp.packets_sent, 1);
    expect_eq(udp.bytes_sent, 5);
    const pkt = udp.simulateReceive(12, peer);
    expect_eq(pkt.payload_len, 12);
    expect_eq(pkt.truncated, false);
}

// ============================================================================
// 2. cap:ws — RFC 6455 Opcodes, Headers, Builder & Masking Roundtrip
// ============================================================================
fn testWebSocketCodec(): !void {
    expect_eq(Opcode.Text.isData(), true);
    expect_eq(Opcode.Text.isControl(), false);
    expect_eq(Opcode.Binary.isData(), true);
    expect_eq(Opcode.Ping.isControl(), true);
    expect_eq(Opcode.Pong.isControl(), true);
    expect_eq(Opcode.Close.isControl(), true);

    // Masked text frame roundtrip
    let wire: [256]u8 = undefined;
    const msg = "RFC6455 zero-copy frame!";
    let key: [4]u8 = [0x37, 0xFA, 0x21, 0x3D];
    const wire_len = try FrameBuilder.buildText(&wire, msg, key);
    expect_eq(wire_len, 2 + 4 + msg.len);

    const hdr = try FrameHeader.decode(wire[0..wire_len]);
    expect_eq(hdr.fin, true);
    expect_eq(hdr.opcode, Opcode.Text);
    expect_eq(hdr.masked, true);
    expect_eq(hdr.payload_len, msg.len);
    expect_eq(hdr.header_len, 6);

    let payload: [64]u8 = [0 as u8; 64];
    @memcpy(payload[0..msg.len], wire[hdr.header_len..wire_len]);
    Mask.apply(payload[0..msg.len], hdr.mask_key);
    expect_eq(std.mem.eql(u8, payload[0..msg.len], msg), true);

    // 16-bit extended payload length header (len = 400)
    const ext_hdr = FrameHeader{
        .fin = true,
        .opcode = .Binary,
        .masked = true,
        .payload_len = 400,
        .mask_key = key,
    };
    let hbuf: [16]u8 = undefined;
    const hlen = try ext_hdr.encode(&hbuf);
    expect_eq(hlen, 8);
    const decoded_ext = try FrameHeader.decode(hbuf[0..hlen]);
    expect_eq(decoded_ext.payload_len, 400);
    expect_eq(decoded_ext.opcode, Opcode.Binary);

    // Control frames: Ping, Pong, Close
    let ctrl: [64]u8 = undefined;
    const ping_len = try FrameBuilder.buildPing(&ctrl, "hb", null);
    const ping_hdr = try FrameHeader.decode(ctrl[0..ping_len]);
    expect_eq(ping_hdr.opcode, Opcode.Ping);
    expect_eq(ping_hdr.payload_len, 2);

    const pong_len = try FrameBuilder.buildPong(&ctrl, "hb", null);
    const pong_hdr = try FrameHeader.decode(ctrl[0..pong_len]);
    expect_eq(pong_hdr.opcode, Opcode.Pong);

    const close_len = try FrameBuilder.buildClose(&ctrl, CloseCode.NormalClosure, "bye", null);
    const close_hdr = try FrameHeader.decode(ctrl[0..close_len]);
    expect_eq(close_hdr.opcode, Opcode.Close);
    expect_eq(close_hdr.payload_len, 2 + 3);
    expect_eq(std.mem.readInt(u16, ctrl[close_hdr.header_len .. close_hdr.header_len + 2][0..2], .big), 1000);
}

// ============================================================================
// 3. cap:router — Radix-Tree Matching, Params, Wildcards & Reverse Formatter
// ============================================================================
fn testRadixRouterAndFormatter(): !void {
    let router = Router(string).init(std.heap.page_allocator);
    router.get("/", "home");
    router.get("/users/me", "current_user");
    router.get("/users/:id", "user_by_id");
    router.post("/users/:id/posts/:postId", "create_post");
    router.get("/org/[orgId]/repo/[repoId]", "repo_detail");
    router.get("/static/*filepath", "serve_static");
    router.get("/docs/[...docpath]", "serve_docs");
    router.all("/health", "healthz");

    // Static takes precedence over dynamic param
    const m_static = router.lookup("GET", "/users/me");
    expect_eq(m_static != null, true);
    expect_eq(std.mem.eql(u8, m_static.?.handler, "current_user"), true);

    // Single & multiple :param extraction
    const m_dyn = router.lookup("GET", "/users/u42");
    expect_eq(m_dyn != null, true);
    expect_eq(std.mem.eql(u8, m_dyn.?.handler, "user_by_id"), true);
    expect_eq(std.mem.eql(u8, m_dyn.?.params.get("id").?, "u42"), true);
    expect_eq(m_dyn.?.params.has("id"), true);
    expect_eq(m_dyn.?.params.count(), 1);

    const m_post = router.lookup("POST", "/users/alice/posts/p7");
    expect_eq(m_post != null, true);
    expect_eq(std.mem.eql(u8, m_post.?.handler, "create_post"), true);
    expect_eq(std.mem.eql(u8, m_post.?.params.get("id").?, "alice"), true);
    expect_eq(std.mem.eql(u8, m_post.?.params.get("postId").?, "p7"), true);

    // Bracket [param] and wildcards
    const m_br = router.lookup("GET", "/org/cp/repo/core");
    expect_eq(m_br != null, true);
    expect_eq(std.mem.eql(u8, m_br.?.params.get("orgId").?, "cp"), true);
    expect_eq(std.mem.eql(u8, m_br.?.params.get("repoId").?, "core"), true);

    const m_wild = router.lookup("GET", "/static/js/app/main.js");
    expect_eq(m_wild != null, true);
    expect_eq(std.mem.eql(u8, m_wild.?.params.get("filepath").?, "js/app/main.js"), true);

    const m_doc = router.lookup("GET", "/docs/v2/net/guide");
    expect_eq(m_doc != null, true);
    expect_eq(std.mem.eql(u8, m_doc.?.params.get("docpath").?, "v2/net/guide"), true);

    // Method isolation & ALL fallback
    expect_eq(router.lookup("DELETE", "/users/u42") == null, true);
    const m_all = router.lookup("PUT", "/health");
    expect_eq(m_all != null, true);
    expect_eq(std.mem.eql(u8, m_all.?.handler, "healthz"), true);

    // Reverse formatting with RouteFormatter
    let names: [4][]const u8 = undefined;
    const pcount = RouteFormatter.extractParamNames("/users/:id/posts/:postId", names[0..4]);
    expect_eq(pcount, 2);
    expect_eq(std.mem.eql(u8, names[0], "id"), true);
    expect_eq(std.mem.eql(u8, names[1], "postId"), true);

    let pm = ParamMap.init();
    pm.append("id", "99");
    pm.append("postId", "hello");
    let url_buf: [128]u8 = [0 as u8; 128];
    const ulen = RouteFormatter.formatPath("/users/:id/posts/:postId", &pm, url_buf[0..128]) catch 0;
    expect_eq(std.mem.eql(u8, url_buf[0..ulen], "/users/99/posts/hello"), true);
}

// ============================================================================
// 4. cap:io — Buffer Streams, BufferedReader/Writer, copyStream, MaxBuf, Pipe
// ============================================================================
fn testIoStreamsAndPipe(): !void {
    const text = "The quick brown fox jumps over the lazy dog.";
    let r = new BufferReader(text[0..text.len]);
    expect_eq(r.remaining(), text.len);

    // Partial read & seek
    let chunk: [10]u8 = [0 as u8; 10];
    const n1 = r.read(chunk[0..10]) catch 0;
    expect_eq(n1, 10);
    expect_eq(std.mem.eql(u8, chunk[0..10], "The quick "), true);

    const s0 = r.seek(0, SeekOrigin.Start) catch 999;
    expect_eq(s0, 0);
    const s_end = r.seek(-4, SeekOrigin.End) catch 999;
    expect_eq(s_end, text.len - 4);
    r.reset();

    // copyStream & copyN
    let w_full = new BufferWriter();
    const copied_all = copyStream(r, w_full) catch 0;
    expect_eq(copied_all, text.len);
    expect_eq(std.mem.eql(u8, w_full.bytes(), text), true);

    r.reset();
    let w_part = new BufferWriter();
    const copied_9 = copyN(r, w_part, 9) catch 0;
    expect_eq(copied_9, 9);
    expect_eq(std.mem.eql(u8, w_part.bytes(), "The quick"), true);

    // BufferedReader & BufferedWriter
    let br_src = new BufferReader("ABCDEF");
    let br = new BufferedReader<BufferReader>(br_src);
    expect_eq(br.readByte() catch 0, 65); // 'A'
    expect_eq(br.readByte() catch 0, 66); // 'B'
    let rem_buf: [4]u8 = [0 as u8; 4];
    expect_eq(br.read(rem_buf[0..4]) catch 0, 4);
    expect_eq(std.mem.eql(u8, rem_buf[0..4], "CDEF"), true);

    let bw_dst = new BufferWriter();
    let bw = new BufferedWriter<BufferWriter>(bw_dst);
    expect_eq(bw.write("hello ") catch 0, 6);
    expect_eq(bw.write("buffered") catch 0, 8);
    expect_eq(bw_dst.len(), 0); // Still in 256B cache before flush
    bw.flush() catch {};
    expect_eq(std.mem.eql(u8, bw_dst.bytes(), "hello buffered"), true);

    // MaxBuf quota guard
    let mb = MaxBuf.withKind(16, MaxBufKind.Stdout);
    expect_eq(mb.onReadBytes(10) catch false, true);
    expect_eq(mb.remaining(), 6);
    expect_eq(mb.onReadBytes(6) catch false, true);
    expect_eq(mb.isExceeded(), false);
    expect_eq(mb.onReadBytes(1) catch false, false);
    expect_eq(mb.isExceeded(), true);
    mb.reset();
    expect_eq(mb.remaining(), 16);

    // Native OS anonymous Pipe IPC
    let pipe = Pipe.create() catch unreachable;
    const ipc_msg = "IPC pipe payload";
    const pw = pipe.writer.write(ipc_msg) catch 0;
    expect_eq(pw, ipc_msg.len);

    let pbuf: [32]u8 = [0 as u8; 32];
    const pr = pipe.reader.read(pbuf[0..32]) catch 0;
    expect_eq(pr, ipc_msg.len);
    expect_eq(std.mem.eql(u8, pbuf[0..pr], ipc_msg), true);

    pipe.writer.close();
    pipe.reader.close();
    expect_eq(pipe.writer.isClosed(), true);
    expect_eq(pipe.reader.isClosed(), true);
}

// ============================================================================
// 5. cap:sys — Temp Files/Dirs, Positional IO, Stat, copyFile & Signals
// ============================================================================
fn testSysFilesStatCopyAndSignals(): !void {
    // Temp dir & temp file (including suffix validation)
    const tmp_root = getTempDir();
    expect_eq(tmp_root.len > 0, true);
    const st_root = try stat(tmp_root);
    expect_eq(st_root.isDir(), true);

    let src_buf: [1024]u8 = undefined;
    let src_file = try makeTempFile("cap_net_io_src", ".dat", &src_buf);
    const src_len = std.mem.indexOfScalar(u8, &src_buf, 0) orelse src_buf.len;
    const src_path = src_buf[0..src_len];
    expect_eq(std.mem.endsWith(u8, src_path, ".dat"), true);

    // writeAll + pwrite + pread + seekTo + seekFromEnd + fstat
    try src_file.writeAll("0123456789ABCDEF");
    const pw_n = try src_file.pwrite("WXYZ", 4);
    expect_eq(pw_n, 4);

    let rbuf: [32]u8 = undefined;
    const pr_n = try src_file.pread(rbuf[0..16], 0);
    expect_eq(pr_n, 16);
    expect_eq(std.mem.eql(u8, rbuf[0..16], "0123WXYZ89ABCDEF"), true);

    expect_eq(try src_file.seekTo(8), 8);
    expect_eq(try src_file.read(rbuf[0..4]), 4);
    expect_eq(std.mem.eql(u8, rbuf[0..4], "89AB"), true);
    expect_eq(try src_file.seekFromEnd(0), 16);

    const st_fd = try src_file.stat();
    expect_eq(st_fd.size, 16);
    expect_eq(st_fd.isFile(), true);
    expect_eq(st_fd.isDir(), false);
    src_file.close();

    // makeTempDir + copyFile + stat verification + cleanup
    let dir_buf: [1024]u8 = undefined;
    const sub_dir = try makeTempDir("cap_net_io_dir", &dir_buf);
    const st_sub = try stat(sub_dir);
    expect_eq(st_sub.isDir(), true);

    let dst_buf: [1024]u8 = undefined;
    const dst_path = try std.fmt.bufPrint(&dst_buf, "{s}/copied.dat", .{sub_dir});
    try copyFile(src_path, dst_path);

    const st_dst = try stat(dst_path);
    expect_eq(st_dst.size, 16);
    expect_eq(st_dst.isFile(), true);

    let dst_file = try File.openReadOnly(dst_path);
    const dst_read = try dst_file.read(rbuf[0..16]);
    expect_eq(dst_read, 16);
    expect_eq(std.mem.eql(u8, rbuf[0..16], "0123WXYZ89ABCDEF"), true);
    dst_file.close();

    try removeFile(dst_path);
    try removeDir(sub_dir);
    try removeFile(src_path);

    // POSIX Signals
    expect_eq(std.mem.eql(u8, Signal.SIGTERM.label(), "SIGTERM"), true);
    expect_eq(std.mem.eql(u8, Signal.SIGINT.label(), "SIGINT"), true);
    expect_eq(Signal.SIGINT.toExitCode(), 130);
    expect_eq(Signal.SIGTERM.toExitCode(), 143);
    expect_eq(std.mem.eql(u8, Signal.SIGPIPE.description(), "Broken pipe"), true);
    expect_eq(fromNumber(15).?, Signal.SIGTERM);
    expect_eq(fromNumber(16) == null, true);
    expect_eq(fromName("TERM").?, Signal.SIGTERM);
    expect_eq(fromName("SIGUSR1").?, Signal.SIGUSR1);
    expect_eq(fromName("UNKNOWN_SIG") == null, true);
}

fn main() {
    test("1. cap:net Address, HTTP CONNECT & SOCKS5 Proxy Codecs", () => {
        try testNetAddressAndProxy();
    });
    test("2. cap:net SafeSocket, TcpServer/TcpStream Corking & UdpSocket", () => {
        try testNetTcpAndUdp();
    });
    test("3. cap:ws RFC 6455 Frames, Extended Length, Control Frames & Masking", () => {
        try testWebSocketCodec();
    });
    test("4. cap:router Radix-Tree Matching, Params, Wildcards & RouteFormatter", () => {
        try testRadixRouterAndFormatter();
    });
    test("5. cap:io Buffer Streams, BufferedReader/Writer, MaxBuf & OS Pipe", () => {
        try testIoStreamsAndPipe();
    });
    test("6. cap:sys Temp Files/Dirs, Positional IO, Stat, copyFile & Signals", () => {
        try testSysFilesStatCopyAndSignals();
    });
}
