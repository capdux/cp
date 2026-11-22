// Source: cp/cap/test/test_socks_production_deep.cp (Production deep tests for cap:socks)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SOCKS_TCP_PORT,
    SOCKS_V4,
    SOCKS_V5,
    SOCKS5_USERPASS_VER,
    AUTH_NONE,
    AUTH_GSSAPI,
    AUTH_USERPASS,
    AUTH_NO_ACCEPTABLE,
    CMD_CONNECT,
    CMD_BIND,
    CMD_UDP_ASSOCIATE,
    ATYP_IPV4,
    ATYP_DOMAIN,
    ATYP_IPV6,
    REP_SUCCEEDED,
    REP_GENERAL_FAILURE,
    REP_NOT_ALLOWED,
    REP_NET_UNREACHABLE,
    REP_HOST_UNREACHABLE,
    REP_CONN_REFUSED,
    REP_TTL_EXPIRED,
    REP_CMD_NOT_SUPPORTED,
    REP_ATYP_NOT_SUPPORTED,
    Socks5Greeting,
    parseSocks5Greeting,
    encodeSocks5Greeting,
    encodeSocks5MethodSelect,
    Socks5UserPass,
    parseSocks5UserPass,
    encodeSocks5UserPass,
    Socks5Addr,
    Socks5Request,
    parseSocks5Request,
    encodeSocks5Request,
    Socks5Reply,
    parseSocks5Reply,
    encodeSocks5Reply,
    Socks5UdpPacket,
    parseSocks5Udp,
    encodeSocks5Udp,
    Socks4Request,
    parseSocks4Request,
} from "../../lib/net/socks_.cp";

test("1. SOCKS5 constants, greeting & method selection round-trip", () => {
    expect_eq(SOCKS_TCP_PORT, 1080 as u16);
    expect_eq(SOCKS_V4, 4 as u8);
    expect_eq(SOCKS_V5, 5 as u8);
    expect_eq(SOCKS5_USERPASS_VER, 1 as u8);
    expect_eq(REP_GENERAL_FAILURE, 1 as u8);
    expect_eq(REP_NOT_ALLOWED, 2 as u8);
    expect_eq(REP_NET_UNREACHABLE, 3 as u8);
    expect_eq(REP_HOST_UNREACHABLE, 4 as u8);
    expect_eq(REP_CONN_REFUSED, 5 as u8);
    expect_eq(REP_TTL_EXPIRED, 6 as u8);
    expect_eq(REP_CMD_NOT_SUPPORTED, 7 as u8);
    expect_eq(REP_ATYP_NOT_SUPPORTED, 8 as u8);

    let buf: [16]u8 = std.mem.zeroes([16]u8);
    const methods: [2]u8 = [AUTH_NONE, AUTH_USERPASS];
    const n = encodeSocks5Greeting(methods[0..], buf[0..]) ?? unreachable;
    expect_eq(n, 4 as usize);
    expect_eq(buf[0], SOCKS_V5);
    expect_eq(buf[1], 2 as u8);

    const g = parseSocks5Greeting(buf[0..n]) ?? unreachable;
    expect_eq(g.methods.len, 2 as usize);
    expect_eq(g.supports(AUTH_NONE), true);
    expect_eq(g.supports(AUTH_USERPASS), true);
    expect_eq(g.supports(AUTH_GSSAPI), false);

    const sel_n = encodeSocks5MethodSelect(AUTH_NO_ACCEPTABLE, buf[0..]) ?? unreachable;
    expect_eq(sel_n, 2 as usize);
    expect_eq(buf[0], SOCKS_V5);
    expect_eq(buf[1], AUTH_NO_ACCEPTABLE);
});

test("2. RFC 1929 username/password subnegotiation round-trip", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const n = encodeSocks5UserPass("alice", "s3cr3t!", buf[0..]) ?? unreachable;
    expect_eq(n, 15 as usize);
    expect_eq(buf[0], SOCKS5_USERPASS_VER);

    const up = parseSocks5UserPass(buf[0..n]) ?? unreachable;
    expect_eq(std.mem.eql(u8, up.username, "alice"), true);
    expect_eq(std.mem.eql(u8, up.password, "s3cr3t!"), true);

    // Empty username or password must be rejected
    expect_eq(encodeSocks5UserPass("", "pass", buf[0..]) == null, true);
    expect_eq(encodeSocks5UserPass("user", "", buf[0..]) == null, true);
});

test("3. SOCKS5 IPv4, Domain, and IPv6 Request & Reply round-trip", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    // IPv4 CONNECT Request
    const v4_raw: [4]u8 = [93, 184, 216, 34];
    const v4_addr = Socks5Addr{ .atyp = ATYP_IPV4, .raw = v4_raw[0..], .port = 80 };
    const n1 = encodeSocks5Request(CMD_CONNECT, v4_addr, buf[0..]) ?? unreachable;
    expect_eq(n1, 10 as usize);
    const req1 = parseSocks5Request(buf[0..n1]) ?? unreachable;
    expect_eq(req1.cmd, CMD_CONNECT);
    expect_eq(req1.addr.atyp, ATYP_IPV4);
    expect_eq(std.mem.eql(u8, req1.addr.raw, v4_raw[0..]), true);
    expect_eq(req1.addr.port, 80 as u16);

    // Domain CONNECT Request
    const dom_addr = Socks5Addr{ .atyp = ATYP_DOMAIN, .raw = "cp-lang.org", .port = 443 };
    const n2 = encodeSocks5Request(CMD_BIND, dom_addr, buf[0..]) ?? unreachable;
    expect_eq(n2, (7 + 11) as usize);
    const req2 = parseSocks5Request(buf[0..n2]) ?? unreachable;
    expect_eq(req2.cmd, CMD_BIND);
    expect_eq(req2.addr.atyp, ATYP_DOMAIN);
    expect_eq(std.mem.eql(u8, req2.addr.raw, "cp-lang.org"), true);
    expect_eq(req2.addr.port, 443 as u16);

    // IPv6 Reply
    const v6_raw: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const v6_addr = Socks5Addr{ .atyp = ATYP_IPV6, .raw = v6_raw[0..], .port = 1080 };
    const n3 = encodeSocks5Reply(REP_SUCCEEDED, v6_addr, buf[0..]) ?? unreachable;
    expect_eq(n3, 22 as usize);
    const rep3 = parseSocks5Reply(buf[0..n3]) ?? unreachable;
    expect_eq(rep3.rep, REP_SUCCEEDED);
    expect_eq(rep3.bnd_addr.atyp, ATYP_IPV6);
    expect_eq(std.mem.eql(u8, rep3.bnd_addr.raw, v6_raw[0..]), true);
    expect_eq(rep3.bnd_addr.port, 1080 as u16);
});

test("4. SOCKS5 UDP Associate datagram encoding and zero-copy payload slicing", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const dst = Socks5Addr{ .atyp = ATYP_DOMAIN, .raw = "dns.google", .port = 53 };
    const payload = "DNS_QUERY_PAYLOAD";

    const n = encodeSocks5Udp(0, dst, payload, buf[0..]) ?? unreachable;
    expect_eq(n, (3 + 1 + 1 + 10 + 2 + payload.len) as usize);

    const pkt = parseSocks5Udp(buf[0..n]) ?? unreachable;
    expect_eq(pkt.frag, 0 as u8);
    expect_eq(pkt.dst_addr.atyp, ATYP_DOMAIN);
    expect_eq(std.mem.eql(u8, pkt.dst_addr.raw, "dns.google"), true);
    expect_eq(pkt.dst_addr.port, 53 as u16);
    expect_eq(std.mem.eql(u8, pkt.payload, payload), true);

    // Empty UDP payload with IPv4 destination
    const ip4: [4]u8 = [127, 0, 0, 1];
    const dst4 = Socks5Addr{ .atyp = ATYP_IPV4, .raw = ip4[0..], .port = 5353 };
    const n2 = encodeSocks5Udp(1, dst4, "", buf[0..]) ?? unreachable;
    const pkt2 = parseSocks5Udp(buf[0..n2]) ?? unreachable;
    expect_eq(pkt2.frag, 1 as u8);
    expect_eq(pkt2.payload.len, 0 as usize);
    expect_eq(pkt2.dst_addr.port, 5353 as u16);
});

test("5. SOCKS4 and SOCKS4a request parsing with NUL-terminated user_id and domain", () => {
    // Classic SOCKS4: VN=4, CD=1, PORT=80 (0x00,0x50), IP=93.184.216.34, USER="fred\0"
    const s4_wire: [13]u8 = [4, 1, 0x00, 0x50, 93, 184, 216, 34, 'f', 'r', 'e', 'd', 0];
    const r4 = parseSocks4Request(s4_wire[0..]) ?? unreachable;
    expect_eq(r4.cmd, CMD_CONNECT);
    expect_eq(r4.port, 80 as u16);
    expect_eq(r4.ip[0], 93 as u8);
    expect_eq(r4.ip[3], 34 as u8);
    expect_eq(std.mem.eql(u8, r4.user_id, "fred"), true);
    expect_eq(r4.isSocks4a(), false);
    expect_eq(r4.domain.len, 0 as usize);

    // SOCKS4a: IP=0.0.0.1, USER="\0", DOMAIN="example.org\0"
    const s4a_wire: [21]u8 = [
        4, 1, 0x01, 0xBB, 0, 0, 0, 1,
        0,
        'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'o', 'r', 'g', 0,
    ];
    const r4a = parseSocks4Request(s4a_wire[0..]) ?? unreachable;
    expect_eq(r4a.cmd, CMD_CONNECT);
    expect_eq(r4a.port, 443 as u16);
    expect_eq(r4a.user_id.len, 0 as usize);
    expect_eq(r4a.isSocks4a(), true);
    expect_eq(std.mem.eql(u8, r4a.domain, "example.org"), true);

    // IP=0.0.0.0 is NOT SOCKS4a
    const s4_zero: [9]u8 = [4, 2, 0x00, 0x15, 0, 0, 0, 0, 0];
    const r4z = parseSocks4Request(s4_zero[0..]) ?? unreachable;
    expect_eq(r4z.cmd, CMD_BIND);
    expect_eq(r4z.isSocks4a(), false);
});

test("6. Malformed packets, invalid version, non-zero RSV, bad ATYP, and truncation rejection", () => {
    // Bad greeting version / zero methods / truncated methods
    const bad_ver: [3]u8 = [4, 1, 0];
    expect_eq(parseSocks5Greeting(bad_ver[0..]) == null, true);
    const zero_m: [2]u8 = [5, 0];
    expect_eq(parseSocks5Greeting(zero_m[0..]) == null, true);
    const trunc_m: [3]u8 = [5, 3, 0];
    expect_eq(parseSocks5Greeting(trunc_m[0..]) == null, true);

    // Non-zero RSV in Request / Reply / UDP
    const bad_rsv_req: [10]u8 = [5, CMD_CONNECT, 0x01, ATYP_IPV4, 1, 2, 3, 4, 0, 80];
    expect_eq(parseSocks5Request(bad_rsv_req[0..]) == null, true);

    const bad_rsv_rep: [10]u8 = [5, REP_SUCCEEDED, 0xFF, ATYP_IPV4, 1, 2, 3, 4, 0, 80];
    expect_eq(parseSocks5Reply(bad_rsv_rep[0..]) == null, true);

    const bad_rsv_udp: [10]u8 = [0, 1, 0, ATYP_IPV4, 1, 2, 3, 4, 0, 53];
    expect_eq(parseSocks5Udp(bad_rsv_udp[0..]) == null, true);

    // Invalid CMD and invalid ATYP
    const bad_cmd: [10]u8 = [5, 0x09, 0, ATYP_IPV4, 1, 2, 3, 4, 0, 80];
    expect_eq(parseSocks5Request(bad_cmd[0..]) == null, true);
    const bad_atyp: [10]u8 = [5, CMD_CONNECT, 0, 0x02, 1, 2, 3, 4, 0, 80];
    expect_eq(parseSocks5Request(bad_atyp[0..]) == null, true);

    // Truncated IPv4 / Domain / IPv6
    const trunc_v4: [9]u8 = [5, CMD_CONNECT, 0, ATYP_IPV4, 1, 2, 3, 4, 0];
    expect_eq(parseSocks5Request(trunc_v4[0..]) == null, true);
    const zero_dom: [7]u8 = [5, CMD_CONNECT, 0, ATYP_DOMAIN, 0, 0, 80];
    expect_eq(parseSocks5Request(zero_dom[0..]) == null, true);
    const trunc_v6: [18]u8 = [5, CMD_CONNECT, 0, ATYP_IPV6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(parseSocks5Request(trunc_v6[0..]) == null, true);

    // Malformed RFC 1929 user/pass & SOCKS4/4a
    const bad_up_ver: [5]u8 = [2, 1, 'a', 1, 'b'];
    expect_eq(parseSocks5UserPass(bad_up_ver[0..]) == null, true);
    const s4_no_nul: [9]u8 = [4, 1, 0, 80, 1, 2, 3, 4, 'a'];
    expect_eq(parseSocks4Request(s4_no_nul[0..]) == null, true);
    const s4a_empty_dom: [10]u8 = [4, 1, 0, 80, 0, 0, 0, 1, 0, 0];
    expect_eq(parseSocks4Request(s4a_empty_dom[0..]) == null, true);
    const s4a_missing_dom: [9]u8 = [4, 1, 0, 80, 0, 0, 0, 1, 0];
    expect_eq(parseSocks4Request(s4a_missing_dom[0..]) == null, true);

    // Small output buffer rejection
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    const ip4: [4]u8 = [1, 2, 3, 4];
    const addr4 = Socks5Addr{ .atyp = ATYP_IPV4, .raw = ip4[0..], .port = 80 };
    expect_eq(encodeSocks5Request(CMD_UDP_ASSOCIATE, addr4, tiny[0..]) == null, true);
});
