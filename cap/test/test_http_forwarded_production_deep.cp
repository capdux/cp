// Source: cp/cap/test/test_http_forwarded_production_deep.cp (Production deep tests for cap:http_forwarded)

import { test, expect_eq } from "cap:test";
import {
    ForwardedHop,
    ForwardedIterator,
    parseFirstHop,
    extractClientIpFromXff,
    stripPortFromForwardedFor,
} from "cap:http";

test("http_forwarded: RFC 7239 multi-hop parsing with quoted IPv6+port and case-insensitive keys") {
    const hdr =
        "For=\"[2001:db8:cafe::17]:4711\";Proto=https;Host=\"api.example.com:443\";By=203.0.113.43, " ++
        "for=192.0.2.60:8080; proto=http; by=_hidden";

    let it = ForwardedIterator.init(hdr);

    const h1 = it.next().?;
    expect_eq(std.mem.eql(u8, h1.for_addr, "[2001:db8:cafe::17]:4711"), true);
    expect_eq(std.mem.eql(u8, h1.proto, "https"), true);
    expect_eq(std.mem.eql(u8, h1.host, "api.example.com:443"), true);
    expect_eq(std.mem.eql(u8, h1.by_addr, "203.0.113.43"), true);

    const h2 = it.next().?;
    expect_eq(std.mem.eql(u8, h2.for_addr, "192.0.2.60:8080"), true);
    expect_eq(std.mem.eql(u8, h2.proto, "http"), true);
    expect_eq(std.mem.eql(u8, h2.by_addr, "_hidden"), true);
    expect_eq(h2.host.len, 0);

    expect_eq(it.next() == null, true);
}

test("http_forwarded: parseFirstHop and quoted comma/semicolon escape protection") {
    const hdr = "for=198.51.100.1; host=\"edge,1;\\\"a\\\"\"; proto=https, for=10.0.0.1";
    const first = parseFirstHop(hdr).?;
    expect_eq(std.mem.eql(u8, first.for_addr, "198.51.100.1"), true);
    expect_eq(std.mem.eql(u8, first.host, "edge,1;\\\"a\\\""), true);
    expect_eq(std.mem.eql(u8, first.proto, "https"), true);
}

test("http_forwarded: extractClientIpFromXff with trusted_proxies hop selection") {
    const xff = "  203.0.113.1 , , 10.0.0.1 , 10.0.0.2  ";

    // 0 个可信代理 -> 取最右侧直接对端
    expect_eq(std.mem.eql(u8, extractClientIpFromXff(xff, 0).?, "10.0.0.2"), true);
    // 1 个可信代理 -> 跳过 10.0.0.2，取 10.0.0.1
    expect_eq(std.mem.eql(u8, extractClientIpFromXff(xff, 1).?, "10.0.0.1"), true);
    // 2 个可信代理 -> 跳过 10.0.0.1 与 10.0.0.2，取真实客户端 203.0.113.1
    expect_eq(std.mem.eql(u8, extractClientIpFromXff(xff, 2).?, "203.0.113.1"), true);
    // 可信代理数超过链长 -> 安全钳位到最左侧客户端 IP
    expect_eq(std.mem.eql(u8, extractClientIpFromXff(xff, 10).?, "203.0.113.1"), true);

    // 空与纯逗号头部返回 null
    expect_eq(extractClientIpFromXff("", 1) == null, true);
    expect_eq(extractClientIpFromXff(" , , \t , ", 1) == null, true);
}

test("http_forwarded: stripPortFromForwardedFor IPv6, IPv4, obfuscated and bare IPv6") {
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("[2001:db8::1]:4711"), "2001:db8::1"), true);
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("\"[2001:db8::1]:4711\""), "2001:db8::1"), true);
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("[2001:db8::1]"), "2001:db8::1"), true);
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("2001:db8::1"), "2001:db8::1"), true);
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("192.0.2.60:8080"), "192.0.2.60"), true);
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("192.0.2.60"), "192.0.2.60"), true);
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("_hidden"), "_hidden"), true);
    expect_eq(std.mem.eql(u8, stripPortFromForwardedFor("unknown"), "unknown"), true);
    expect_eq(stripPortFromForwardedFor("").len, 0);
}

test("http_forwarded: malformed and empty Forwarded headers guard") {
    expect_eq(parseFirstHop("") == null, true);
    expect_eq(parseFirstHop(" , ; , ") == null, true);
    expect_eq(parseFirstHop("garbage_without_equals; unknown_key=123") == null, true);

    // 前置畸形跳被跳过，后续有效跳正常解析
    const mixed = "invalid_hop, , for=192.0.2.1; proto=https";
    const hop = parseFirstHop(mixed).?;
    expect_eq(std.mem.eql(u8, hop.for_addr, "192.0.2.1"), true);
    expect_eq(std.mem.eql(u8, hop.proto, "https"), true);
}
