// Source: cp/cap/test/test_http_host_header_production_deep.cp (Production deep tests for cap:http_host_header)

import { test, expect_eq } from "cap:test";
import { HostHeader } from "cap:http_host_header";

test("http_host_header: domain, IPv4 and bracketed IPv6 parsing with ports") {
    const h1 = HostHeader.parse("example.com:443").?;
    expect_eq(std.mem.eql(u8, h1.host, "example.com"), true);
    expect_eq(std.mem.eql(u8, h1.unbracketedHost(), "example.com"), true);
    expect_eq(h1.port.?, 443);
    expect_eq(h1.is_ipv6, false);

    const h2 = HostHeader.parse("127.0.0.1").?;
    expect_eq(std.mem.eql(u8, h2.host, "127.0.0.1"), true);
    expect_eq(h2.port == null, true);
    expect_eq(h2.is_ipv6, false);

    const h3 = HostHeader.parse("[::1]:8080").?;
    expect_eq(std.mem.eql(u8, h3.host, "[::1]"), true);
    expect_eq(std.mem.eql(u8, h3.unbracketedHost(), "::1"), true);
    expect_eq(h3.port.?, 8080);
    expect_eq(h3.is_ipv6, true);

    const h4 = HostHeader.parse("[2001:db8::1]").?;
    expect_eq(std.mem.eql(u8, h4.host, "[2001:db8::1]"), true);
    expect_eq(std.mem.eql(u8, h4.unbracketedHost(), "2001:db8::1"), true);
    expect_eq(h4.port == null, true);
    expect_eq(h4.is_ipv6, true);
}

test("http_host_header: effectivePort and isLoopback detection") {
    const plain = HostHeader.parse("LOCALHOST").?;
    expect_eq(plain.effectivePort(false), 80);
    expect_eq(plain.effectivePort(true), 443);
    expect_eq(plain.isLoopback(), true);

    const custom = HostHeader.parse("[::1]:9000").?;
    expect_eq(custom.effectivePort(false), 9000);
    expect_eq(custom.effectivePort(true), 9000);
    expect_eq(custom.isLoopback(), true);

    const ip4 = HostHeader.parse("127.0.0.1:3000").?;
    expect_eq(ip4.isLoopback(), true);

    const ext = HostHeader.parse("api.example.com").?;
    expect_eq(ext.isLoopback(), false);
}

test("http_host_header: formatCanonical lowercases host and elides default ports") {
    let buf: [64]u8 = [0; 64];

    const h_tls = HostHeader.parse("API.Example.COM:443").?;
    expect_eq(std.mem.eql(u8, h_tls.formatCanonical(true, buf[0..]).?, "api.example.com"), true);
    expect_eq(std.mem.eql(u8, h_tls.formatCanonical(false, buf[0..]).?, "api.example.com:443"), true);

    const h_http = HostHeader.parse("Example.ORG:80").?;
    expect_eq(std.mem.eql(u8, h_http.formatCanonical(false, buf[0..]).?, "example.org"), true);
    expect_eq(std.mem.eql(u8, h_http.formatCanonical(true, buf[0..]).?, "example.org:80"), true);

    const h_v6 = HostHeader.parse("[2001:DB8::A]:8443").?;
    expect_eq(std.mem.eql(u8, h_v6.formatCanonical(true, buf[0..]).?, "[2001:db8::a]:8443"), true);
}

test("http_host_header: RFC 9110 §7.2 malformed host header rejection") {
    // 空串、首尾/内部空白、userinfo @、路径/查询/锚点
    expect_eq(HostHeader.parse("") == null, true);
    expect_eq(HostHeader.parse(" example.com") == null, true);
    expect_eq(HostHeader.parse("example.com ") == null, true);
    expect_eq(HostHeader.parse("exa mple.com") == null, true);
    expect_eq(HostHeader.parse("user@example.com") == null, true);
    expect_eq(HostHeader.parse("example.com/path") == null, true);
    expect_eq(HostHeader.parse("example.com?q=1") == null, true);
    expect_eq(HostHeader.parse("example.com#frag") == null, true);

    // 端口边界与畸形冒号/方括号
    expect_eq(HostHeader.parse("example.com:") == null, true);
    expect_eq(HostHeader.parse("example.com:0") == null, true);
    expect_eq(HostHeader.parse("example.com:080") == null, true);
    expect_eq(HostHeader.parse("example.com:65536") == null, true);
    expect_eq(HostHeader.parse("example.com:80:80") == null, true);
    expect_eq(HostHeader.parse(":8080") == null, true);
    expect_eq(HostHeader.parse("::1") == null, true);
    expect_eq(HostHeader.parse("[::1") == null, true);
    expect_eq(HostHeader.parse("[::1]extra") == null, true);
    expect_eq(HostHeader.parse("[]") == null, true);
}

test("http_host_header: small output buffer overflow guard") {
    const h = HostHeader.parse("example.com:8080").?;
    let tiny: [5]u8 = [0; 5];
    expect_eq(h.formatCanonical(false, tiny[0..]) == null, true);
}
