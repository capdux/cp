// cap/test/test_net_production_deep.cp
// ============================================================================
// Deep Production Test Suite for Upgraded cap:net
// Covers:
//   1. IPv4/IPv6 CIDR parsing, contains/overlaps, IpClass & CidrAllowlist
//   2. Real loopback TCP socket & socketpair sockopts + pollSingleFd readiness
//   3. SOCKS5 Greeting/RFC1929 Auth/IPv4+Domain+IPv6 Connect + HTTP CONNECT
//   4. ConnectionPool reuse hit, TTL/max_lifetime eviction, caps & PoolStats
//   5. cap:net unified exports & full backward compatibility
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    // Legacy & existing exports
    LIBUS_LISTEN_REUSE_ADDR,
    LIBUS_LISTEN_REUSE_PORT,
    SocketCloseCode,
    SocketKind,
    SendStatus,
    AddressFamily,
    Ipv4Addr,
    Ipv6Addr,
    SocketAddress,
    SocketState,
    SafeSocket,
    STREAM_CORK_BUFFER_SIZE,
    StreamBuffer,
    TcpStream,
    ListenConfig,
    TcpServer,
    UdpPacket,
    UdpSocket,
    HttpConnectTunnel,
    // New production modules
    IpKind,
    IpAddr,
    IpAddrCodec,
    Ipv4Cidr,
    Ipv6Cidr,
    IpClass,
    CidrAllowlist,
    PollEvent,
    SocketOptions,
    setNoDelay,
    getNoDelay,
    setKeepAlive,
    getKeepAlive,
    setReuseAddr,
    getReuseAddr,
    setReusePort,
    getReusePort,
    setRecvBufSize,
    getRecvBufSize,
    setSendBufSize,
    getSendBufSize,
    setRecvTimeoutMs,
    setSendTimeoutMs,
    setNonBlocking,
    isNonBlocking,
    getSocketError,
    pollSingleFd,
    pollManyFds,
    createTcpSocket,
    createSocketPair,
    closeSocketFd,
    writeFd,
    readFd,
    Socks5AddrType,
    Socks5AuthMethod,
    Socks5ReplyCode,
    Socks5MethodSelection,
    Socks5ConnectReply,
    Socks5Codec,
    HttpConnectCodec,
    PoolCloseFn,
    ConnPoolConfig,
    PoolEntry,
    PoolStats,
    ConnectionPool
} from "cap:net";

let g_closed_fds_count: usize = 0;
let g_last_closed_fd: i32 = -1;

fn recordClosedFd(fd: i32) {
    g_closed_fds_count += 1;
    g_last_closed_fd = fd;
}

test("1. IPv4/IPv6 CIDR parsing, contains/overlaps, IpClass & CidrAllowlist") {
    // IPv4 CIDR parsing, normalization, broadcast & contains
    const c4 = Ipv4Cidr.parse("192.168.1.42/24").?;
    expect_eq(c4.prefixLen() as i64, 24);
    const net4: Ipv4Addr = c4.networkAddress();
    const bcast4: Ipv4Addr = c4.broadcastAddress();
    expect_eq(net4.toU32() as i64, 0xC0A80100);
    expect_eq(bcast4.toU32() as i64, 0xC0A801FF);
    expect_eq(c4.contains(Ipv4Addr.parse("192.168.1.254").?), true);
    expect_eq(c4.contains(Ipv4Addr.parse("192.168.2.1").?), false);

    let fmt_buf: [64]u8 = [0; 64];
    const f4_len = c4.format(fmt_buf[0..64]);
    expect_eq(std.mem.eql(u8, fmt_buf[0..f4_len], "192.168.1.0/24"), true);

    // IPv4 overlaps & invalid inputs
    const c4_10 = Ipv4Cidr.parse("10.0.0.0/8").?;
    const c4_10_sub = Ipv4Cidr.parse("10.20.30.0/24").?;
    const c4_172 = Ipv4Cidr.parse("172.16.0.0/12").?;
    expect_eq(c4_10.overlaps(c4_10_sub), true);
    expect_eq(c4_10_sub.overlaps(c4_10), true);
    expect_eq(c4_10.overlaps(c4_172), false);
    expect_eq(Ipv4Cidr.parse("192.168.1.0/33") == null, true);
    expect_eq(Ipv4Cidr.parse("192.168.1.0/04") == null, true);

    // IPv6 parsing, formatting & CIDR
    const c6 = Ipv6Cidr.parse("fe80::1234/10").?;
    expect_eq(c6.prefixLen() as i64, 10);
    const f6_len = c6.format(fmt_buf[0..64]);
    expect_eq(std.mem.eql(u8, fmt_buf[0..f6_len], "fe80::/10"), true);
    expect_eq(c6.contains(IpAddrCodec.parseV6("fe80::dead:beef").?), true);
    expect_eq(c6.contains(IpAddrCodec.parseV6("2001:db8::1").?), false);
    expect_eq(c6.overlaps(Ipv6Cidr.parse("fe80::/64").?), true);

    const mapped_v6 = IpAddrCodec.parseV6("::ffff:192.0.2.128").?;
    expect_eq(mapped_v6.isIpv4Mapped(), true);
    expect_eq(Ipv6Cidr.parse("fe80::/129") == null, true);

    // IpClass classification
    expect_eq(IpClass.isLoopback(IpAddr.parse("127.0.0.1").?), true);
    expect_eq(IpClass.isLoopback(IpAddr.parse("::1").?), true);
    expect_eq(IpClass.isPrivate(IpAddr.parse("10.1.2.3").?), true);
    expect_eq(IpClass.isPrivate(IpAddr.parse("172.16.0.1").?), true);
    expect_eq(IpClass.isPrivate(IpAddr.parse("172.32.0.1").?), false);
    expect_eq(IpClass.isPrivate(IpAddr.parse("192.168.99.1").?), true);
    expect_eq(IpClass.isPrivate(IpAddr.parse("fd12:3456:789a::1").?), true);
    expect_eq(IpClass.isLinkLocal(IpAddr.parse("169.254.169.254").?), true);
    expect_eq(IpClass.isLinkLocal(IpAddr.parse("fe80::1").?), true);
    expect_eq(IpClass.isCgnat(IpAddr.parse("100.64.0.1").?), true);
    expect_eq(IpClass.isCgnat(IpAddr.parse("100.127.255.254").?), true);
    expect_eq(IpClass.isCgnat(IpAddr.parse("100.128.0.1").?), false);
    expect_eq(IpClass.isMulticast(IpAddr.parse("224.0.0.251").?), true);
    expect_eq(IpClass.isMulticast(IpAddr.parse("ff02::1").?), true);
    expect_eq(IpClass.isGlobalUnicast(IpAddr.parse("8.8.8.8").?), true);
    expect_eq(IpClass.isGlobalUnicast(IpAddr.parse("2606:4700:4700::1111").?), true);
    expect_eq(IpClass.isGlobalUnicast(IpAddr.parse("10.0.0.1").?), false);
    expect_eq(IpClass.classify(IpAddr.parse("100.64.1.1").?) == IpKind.Cgnat, true);

    // CidrAllowlist & SSRF outbound guard
    let allow = CidrAllowlist.init();
    defer allow.deinit();
    expect_eq(allow.addRule("10.10.0.0/16"), true);
    expect_eq(allow.addRule("fc00::/7"), true);
    expect_eq(allow.addRule("192.168.50.5"), true);
    expect_eq(allow.count() as i64, 3);

    expect_eq(allow.matchesStr("10.10.42.7"), true);
    expect_eq(allow.matchesStr("10.11.0.1"), false);
    expect_eq(allow.matchesStr("fd00::1"), true);
    expect_eq(allow.matchesStr("192.168.50.5"), true);

    expect_eq(allow.isAllowedOutboundStr("8.8.4.4"), true);
    expect_eq(allow.isAllowedOutboundStr("169.254.169.254"), false);
    expect_eq(allow.isAllowedOutboundStr("10.10.42.7"), true);
    expect_eq(allow.isAllowedOutboundStr("192.168.1.1"), false);
}

test("2. Real TCP socket & socketpair sockopts + pollSingleFd I/O readiness") {
    const tcp_fd = createTcpSocket(false).?;
    defer closeSocketFd(tcp_fd);

    expect_eq(setNoDelay(tcp_fd, true), true);
    expect_eq(getNoDelay(tcp_fd).?, true);
    expect_eq(setNoDelay(tcp_fd, false), true);
    expect_eq(getNoDelay(tcp_fd).?, false);

    expect_eq(setReuseAddr(tcp_fd, true), true);
    expect_eq(getReuseAddr(tcp_fd).?, true);

    expect_eq(setReusePort(tcp_fd, true), true);
    expect_eq(getReusePort(tcp_fd).?, true);

    expect_eq(setKeepAlive(tcp_fd, true), true);
    expect_eq(getKeepAlive(tcp_fd).?, true);

    expect_eq(setRecvBufSize(tcp_fd, 65536), true);
    expect_eq(getRecvBufSize(tcp_fd) != null, true);
    expect_eq(setSendBufSize(tcp_fd, 65536), true);
    expect_eq(getSendBufSize(tcp_fd) != null, true);

    expect_eq(setRecvTimeoutMs(tcp_fd, 250), true);
    expect_eq(setSendTimeoutMs(tcp_fd, 250), true);

    expect_eq(setNonBlocking(tcp_fd, true), true);
    expect_eq(isNonBlocking(tcp_fd).?, true);
    expect_eq(setNonBlocking(tcp_fd, false), true);
    expect_eq(isNonBlocking(tcp_fd).?, false);

    expect_eq(getSocketError(tcp_fd) as i64, 0);

    let batch_opts = SocketOptions {
        no_delay: true,
        reuse_addr: true,
        non_blocking: true,
    };
    expect_eq(batch_opts.apply(tcp_fd), true);
    expect_eq(getNoDelay(tcp_fd).?, true);
    expect_eq(isNonBlocking(tcp_fd).?, true);

    // Real socketpair readiness + write/read + pollSingleFd & pollManyFds
    const pair = createSocketPair(true).?;
    const fd_a = pair[0];
    const fd_b = pair[1];
    defer closeSocketFd(fd_a);
    defer closeSocketFd(fd_b);

    const pre_ev = pollSingleFd(fd_b, PollEvent.IN, 0);
    expect_eq(pre_ev.ready, false);
    expect_eq(pre_ev.isReadable(), false);

    const out_ev = pollSingleFd(fd_a, PollEvent.OUT, 100);
    expect_eq(out_ev.ready, true);
    expect_eq(out_ev.isWritable(), true);

    const wn = writeFd(fd_a, "CPNET_OK");
    expect_eq(wn as i64, 8);

    const in_ev = pollSingleFd(fd_b, PollEvent.IN, 100);
    expect_eq(in_ev.ready, true);
    expect_eq(in_ev.isReadable(), true);

    let rbuf: [16]u8 = [0; 16];
    const rn = readFd(fd_b, rbuf[0..16]);
    expect_eq(rn as i64, 8);
    expect_eq(std.mem.eql(u8, rbuf[0..8], "CPNET_OK"), true);

    const post_ev = pollSingleFd(fd_b, PollEvent.IN, 0);
    expect_eq(post_ev.ready, false);

    // Multi-FD poll check
    let pfd_a: std.os.linux.pollfd = undefined;
    pfd_a.fd = fd_a;
    pfd_a.events = @bitCast(PollEvent.OUT);
    pfd_a.revents = 0;
    let pfd_b: std.os.linux.pollfd = undefined;
    pfd_b.fd = fd_b;
    pfd_b.events = @bitCast(PollEvent.IN);
    pfd_b.revents = 0;
    let pfds: [2]std.os.linux.pollfd = [pfd_a, pfd_b];
    const ready_n = pollManyFds(pfds[0..2], 50);
    expect_eq(ready_n as i64, 1);
}

test("3. SOCKS5 Greeting, RFC 1929 Auth, IPv4/Domain/IPv6 Connect & HTTP CONNECT") {
    let buf: [256]u8 = [0; 256];

    // Greeting with multiple methods & method selection decode
    let methods: [2]Socks5AuthMethod = [Socks5AuthMethod.NoAuth, Socks5AuthMethod.UserPass];
    const glen = Socks5Codec.encodeGreeting(buf[0..256], methods[0..2]);
    expect_eq(glen as i64, 4);
    expect_eq(buf[0] as i64, 0x05);
    expect_eq(buf[1] as i64, 0x02);
    expect_eq(buf[2] as i64, 0x00);
    expect_eq(buf[3] as i64, 0x02);

    let sel_up: [2]u8 = [0x05, 0x02];
    const decoded_sel = Socks5Codec.decodeMethodSelection(sel_up[0..2]).?;
    expect_eq(decoded_sel.method == Socks5AuthMethod.UserPass, true);

    let sel_none: [2]u8 = [0x05, 0xFF];
    expect_eq(Socks5Codec.decodeMethodSelection(sel_none[0..2]).?.method == Socks5AuthMethod.NoAcceptable, true);

    // RFC 1929 User/Pass Auth
    const ulen = Socks5Codec.encodeUserPassAuth(buf[0..256], "alice", "s3cr3t");
    expect_eq(ulen as i64, 14);
    expect_eq(buf[0] as i64, 0x01);
    expect_eq(buf[1] as i64, 5);
    expect_eq(buf[7] as i64, 6);
    let auth_ok: [2]u8 = [0x01, 0x00];
    let auth_fail: [2]u8 = [0x01, 0x01];
    expect_eq(Socks5Codec.decodeUserPassReply(auth_ok[0..2]).?, true);
    expect_eq(Socks5Codec.decodeUserPassReply(auth_fail[0..2]).?, false);

    // Connect Request: IPv4, Domain, IPv6
    const c_v4 = Socks5Codec.encodeConnectRequest(buf[0..256], "127.0.0.1", 8080);
    expect_eq(c_v4 as i64, 10);
    expect_eq(buf[3] as i64, 0x01);
    expect_eq(buf[8] as i64, 0x1F);
    expect_eq(buf[9] as i64, 0x90);

    const c_dom = Socks5Codec.encodeConnectRequest(buf[0..256], "cp-lang.org", 443);
    expect_eq(c_dom as i64, 18);
    expect_eq(buf[3] as i64, 0x03);
    expect_eq(buf[4] as i64, 11);

    const c_v6 = Socks5Codec.encodeConnectRequest(buf[0..256], "2001:db8::1", 8443);
    expect_eq(c_v6 as i64, 22);
    expect_eq(buf[3] as i64, 0x04);
    expect_eq(buf[4] as i64, 0x20);
    expect_eq(buf[5] as i64, 0x01);
    expect_eq(buf[19] as i64, 0x01);
    expect_eq(buf[20] as i64, 0x20);
    expect_eq(buf[21] as i64, 0xFB);

    // Connect Reply roundtrip decode (IPv4, Domain, IPv6, Refused)
    let rep_v4: [10]u8 = [0x05, 0x00, 0x00, 0x01, 10, 0, 0, 5, 0x1F, 0x90];
    const dec_v4 = Socks5Codec.decodeConnectReply(rep_v4[0..10]).?;
    expect_eq(dec_v4.succeeded(), true);
    expect_eq(dec_v4.addr_type == Socks5AddrType.Ipv4, true);
    expect_eq(dec_v4.ipv4[0] as i64, 10);
    expect_eq(dec_v4.ipv4[3] as i64, 5);
    expect_eq(dec_v4.port as i64, 8080);

    let rep_dom: [13]u8 = [0x05, 0x00, 0x00, 0x03, 6, 'c', 'p', '.', 'd', 'e', 'v', 0x01, 0xBB];
    const dec_dom = Socks5Codec.decodeConnectReply(rep_dom[0..13]).?;
    expect_eq(dec_dom.succeeded(), true);
    expect_eq(dec_dom.addr_type == Socks5AddrType.Domain, true);
    expect_eq(std.mem.eql(u8, dec_dom.domain(), "cp.dev"), true);
    expect_eq(dec_dom.port as i64, 443);

    buf[1] = Socks5ReplyCode.ConnectionRefused as u8;
    const dec_v6 = Socks5Codec.decodeConnectReply(buf[0..22]).?;
    expect_eq(dec_v6.succeeded(), false);
    expect_eq(dec_v6.reply == Socks5ReplyCode.ConnectionRefused, true);
    expect_eq(dec_v6.addr_type == Socks5AddrType.Ipv6, true);
    expect_eq(dec_v6.port as i64, 8443);

    // HttpConnectCodec with & without Proxy-Authorization + IPv6 bracketing
    const h1 = HttpConnectCodec.encodeRequest(buf[0..256], "api.cp-lang.org", 443, "Basic YWxpY2U6czNjcjN0");
    expect_eq(std.mem.startsWith(u8, buf[0..h1], "CONNECT api.cp-lang.org:443 HTTP/1.1\r\n"), true);
    expect_eq(std.mem.indexOf(u8, buf[0..h1], "Proxy-Authorization: Basic YWxpY2U6czNjcjN0\r\n") != null, true);

    const h2 = HttpConnectCodec.encodeRequest(buf[0..256], "::1", 8443, null);
    expect_eq(std.mem.startsWith(u8, buf[0..h2], "CONNECT [::1]:8443 HTTP/1.1\r\n"), true);

    expect_eq(HttpConnectCodec.parseResponseStatus("HTTP/1.1 200 Connection Established\r\n\r\n").? as i64, 200);
    expect_eq(HttpConnectCodec.parseResponseStatus("HTTP/1.0 407 Proxy Auth Required\r\n\r\n").? as i64, 407);
    expect_eq(HttpConnectCodec.parseResponseStatus("HTTP/2.0 200 OK") == null, true);
}

test("4. ConnectionPool reuse hit, TTL/max_lifetime eviction, caps & PoolStats") {
    g_closed_fds_count = 0;
    g_last_closed_fd = -1;

    let cfg = ConnPoolConfig {
        max_idle_per_host: 2,
        max_total_idle: 3,
        idle_timeout_ms: 1000,
        max_lifetime_ms: 5000,
        close_on_evict: false,
    };
    let pool = ConnectionPool.init(cfg);
    pool.setCloseCallback(recordClosedFd);
    defer pool.deinit();

    // Empty miss
    expect_eq(pool.acquire("api.cp", 443, 100) == null, true);
    expect_eq(pool.stats().misses as i64, 1);

    // Release 2 connections and reuse newest (LIFO)
    expect_eq(pool.releaseFd("api.cp", 443, 101, 100), true);
    expect_eq(pool.releaseFd("api.cp", 443, 102, 200), true);
    expect_eq(pool.stats().idle_count as i64, 2);

    expect_eq(pool.acquire("api.cp", 443, 300).? as i64, 102);
    expect_eq(pool.stats().hits as i64, 1);
    expect_eq(pool.stats().idle_count as i64, 1);

    // Per-host cap eviction (max_idle_per_host = 2)
    expect_eq(pool.releaseFd("api.cp", 443, 103, 310), true);
    expect_eq(pool.releaseFd("api.cp", 443, 104, 320), true);
    expect_eq(pool.hostIdleCount("api.cp", 443) as i64, 2);
    expect_eq(g_closed_fds_count as i64, 1);
    expect_eq(g_last_closed_fd as i64, 101);

    // Global cap eviction (max_total_idle = 3)
    expect_eq(pool.releaseFd("db.cp", 5432, 201, 330), true);
    expect_eq(pool.stats().idle_count as i64, 3);
    expect_eq(pool.releaseFd("db.cp", 5432, 202, 340), true);
    expect_eq(pool.stats().idle_count as i64, 3);
    expect_eq(g_closed_fds_count as i64, 2);
    expect_eq(g_last_closed_fd as i64, 103);

    // Idle timeout eviction during acquire
    expect_eq(pool.acquire("api.cp", 443, 2000) == null, true);
    expect_eq(g_closed_fds_count as i64, 3);
    expect_eq(g_last_closed_fd as i64, 104);

    // Sweep remaining expired entries
    const swept = pool.evictExpired(2000);
    expect_eq(swept as i64, 2);
    expect_eq(pool.stats().idle_count as i64, 0);
    expect_eq(g_closed_fds_count as i64, 5);

    // Max lifetime eviction even within idle timeout
    expect_eq(pool.releaseWithCreated("long.cp", 80, 301, 100, 4900), true);
    expect_eq(pool.acquire("long.cp", 80, 5150) == null, true);
    expect_eq(g_closed_fds_count as i64, 6);
    expect_eq(g_last_closed_fd as i64, 301);

    const st = pool.stats();
    expect_eq(st.hits as i64, 1);
    expect_eq(st.misses as i64, 3);
    expect_eq(st.evictions as i64, 6);
}

test("5. cap:net unified aggregate exports & backward compatibility") {
    const sa4 = SocketAddress.loopbackV4(9443);
    expect_eq(sa4.family == AddressFamily.INET, true);
    expect_eq(sa4.socklen() as i64, 16);

    let sock = SafeSocket.create(55, SocketKind.established_tcp);
    sock.setTimeout(300);
    expect_eq(sock.long_timeout_min as i64, 5);
    sock.close(SocketCloseCode.normal);
    sock.~init();

    let stream = TcpStream.init(56, sa4, SocketAddress.loopbackV4(50000));
    stream.cork();
    expect_eq(stream.write("abc") as i64, 3);
    expect_eq(stream.uncork() as i64, 3);
    expect_eq(stream.sock.recordWrite(1) == SendStatus.success, true);
    stream.~init();

    let cfg = ListenConfig.defaultForPort(9443);
    expect_eq((cfg.toLibusOptions() & (LIBUS_LISTEN_REUSE_PORT | LIBUS_LISTEN_REUSE_ADDR)) != 0, true);
    let srv = TcpServer.bind(sa4, cfg);
    expect_eq(srv.localPort() as i64, 9443);
    srv.~init();

    let udp = UdpSocket.bind(SocketAddress.loopbackV4(5355));
    const pkt: UdpPacket = udp.simulateReceive(12, sa4);
    expect_eq(pkt.payload_len as i64, 12);
    udp.~init();

    expect_eq(HttpConnectTunnel.isTunnelEstablished("HTTP/1.1 200 Connection Established\r\n\r\n"), true);
    let s5_buf: [16]u8 = [0; 16];
    expect_eq(Socks5Codec.encodeGreetingNoAuth(s5_buf[0..16]) as i64, 3);
    expect_eq(STREAM_CORK_BUFFER_SIZE as i64, 16384);
    expect_eq(SocketState.Closed as i64, 3);
}
