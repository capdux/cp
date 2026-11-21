import std from "std";
import { test, expect_eq } from "cap:test";
import { ProxyProtoFamily, ProxyV1Header } from "cap:proxy_protocol_v1";

test("1. ProxyV1Header.parse parses TCP4 with trailing HTTP payload and exact header_len", () => {
    const packet = "PROXY TCP4 203.0.113.19 198.51.100.2 56324 443\r\nGET / HTTP/1.1\r\n";
    const hdr = ProxyV1Header.parse(packet) ?? unreachable;

    expect_eq(hdr.family == ProxyProtoFamily.tcp4, true);
    expect_eq(std.mem.eql(u8, hdr.src_addr, "203.0.113.19"), true);
    expect_eq(std.mem.eql(u8, hdr.dst_addr, "198.51.100.2"), true);
    expect_eq(hdr.src_port, u16(56324));
    expect_eq(hdr.dst_port, u16(443));
    expect_eq(hdr.header_len, usize(48));
    expect_eq(std.mem.eql(u8, packet[hdr.header_len..], "GET / HTTP/1.1\r\n"), true);
});

test("2. ProxyV1Header.parse parses TCP6 including compressed, mapped, and max 107-byte headers", () => {
    const p1 = "PROXY TCP6 2001:db8::1 ::ffff:192.0.2.128 0 65535\r\n";
    const h1 = ProxyV1Header.parse(p1) ?? unreachable;
    expect_eq(h1.family == ProxyProtoFamily.tcp6, true);
    expect_eq(std.mem.eql(u8, h1.src_addr, "2001:db8::1"), true);
    expect_eq(std.mem.eql(u8, h1.dst_addr, "::ffff:192.0.2.128"), true);
    expect_eq(h1.src_port, u16(0));
    expect_eq(h1.dst_port, u16(65535));
    expect_eq(h1.header_len, p1.len);

    // Exact 107-byte maximum valid PROXY v1 header
    const max_hdr =
        "PROXY TCP6 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff 65535 65535\r\n";
    expect_eq(max_hdr.len, usize(107));
    const hmax = ProxyV1Header.parse(max_hdr) ?? unreachable;
    expect_eq(hmax.family == ProxyProtoFamily.tcp6, true);
    expect_eq(hmax.header_len, usize(107));
});

test("3. ProxyV1Header.parse handles UNKNOWN with or without trailing info", () => {
    const u1 = "PROXY UNKNOWN\r\nDATA";
    const h1 = ProxyV1Header.parse(u1) ?? unreachable;
    expect_eq(h1.family == ProxyProtoFamily.unknown, true);
    expect_eq(h1.src_addr.len, usize(0));
    expect_eq(h1.dst_addr.len, usize(0));
    expect_eq(h1.src_port, u16(0));
    expect_eq(h1.dst_port, u16(0));
    expect_eq(h1.header_len, usize(15));
    expect_eq(std.mem.eql(u8, u1[h1.header_len..], "DATA"), true);

    const u2 = "PROXY UNKNOWN ffff::1 ffff::2 1234 5678\r\n";
    const h2 = ProxyV1Header.parse(u2) ?? unreachable;
    expect_eq(h2.family == ProxyProtoFamily.unknown, true);
    expect_eq(h2.header_len, u2.len);
});

test("4. ProxyV1Header.format round-trips TCP4, TCP6, and UNKNOWN cleanly", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);

    const h4 = ProxyV1Header{
        family: ProxyProtoFamily.tcp4,
        src_addr: "10.0.0.1",
        dst_addr: "10.0.0.2",
        src_port: 12345,
        dst_port: 80,
        header_len: 0,
    };
    const f4 = h4.format(&out) ?? unreachable;
    expect_eq(std.mem.eql(u8, f4, "PROXY TCP4 10.0.0.1 10.0.0.2 12345 80\r\n"), true);
    const p4 = ProxyV1Header.parse(f4) ?? unreachable;
    expect_eq(p4.src_port, u16(12345));
    expect_eq(p4.dst_port, u16(80));

    const h6 = ProxyV1Header{
        family: ProxyProtoFamily.tcp6,
        src_addr: "::1",
        dst_addr: "fe80::1",
        src_port: 5000,
        dst_port: 8443,
        header_len: 0,
    };
    const f6 = h6.format(&out) ?? unreachable;
    expect_eq(std.mem.eql(u8, f6, "PROXY TCP6 ::1 fe80::1 5000 8443\r\n"), true);

    const hu = ProxyV1Header{
        family: ProxyProtoFamily.unknown,
        src_addr: "",
        dst_addr: "",
        src_port: 0,
        dst_port: 0,
        header_len: 0,
    };
    const fu = hu.format(&out) ?? unreachable;
    expect_eq(std.mem.eql(u8, fu, "PROXY UNKNOWN\r\n"), true);

    // Tiny buffer or invalid address fails format
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(h4.format(&tiny) == null, true);
    const bad_h4 = ProxyV1Header{
        family: ProxyProtoFamily.tcp4,
        src_addr: "999.0.0.1",
        dst_addr: "10.0.0.2",
        src_port: 80,
        dst_port: 80,
        header_len: 0,
    };
    expect_eq(bad_h4.format(&out) == null, true);
});

test("5. Malformed headers, leading-zero ports, extra spaces, LF-only, and >107 bytes are rejected", () => {
    // Missing CRLF / LF-only
    expect_eq(ProxyV1Header.parse("PROXY TCP4 1.2.3.4 5.6.7.8 80 443\n") == null, true);
    expect_eq(ProxyV1Header.parse("PROXY TCP4 1.2.3.4 5.6.7.8 80 443") == null, true);

    // Wrong prefix / protocol
    expect_eq(ProxyV1Header.parse("proxy TCP4 1.2.3.4 5.6.7.8 80 443\r\n") == null, true);
    expect_eq(ProxyV1Header.parse("PROXY UDP4 1.2.3.4 5.6.7.8 80 443\r\n") == null, true);

    // Double spaces or trailing space
    expect_eq(ProxyV1Header.parse("PROXY TCP4  1.2.3.4 5.6.7.8 80 443\r\n") == null, true);
    expect_eq(ProxyV1Header.parse("PROXY TCP4 1.2.3.4 5.6.7.8 80 443 \r\n") == null, true);

    // Leading-zero ports & out-of-range ports
    expect_eq(ProxyV1Header.parse("PROXY TCP4 1.2.3.4 5.6.7.8 080 443\r\n") == null, true);
    expect_eq(ProxyV1Header.parse("PROXY TCP4 1.2.3.4 5.6.7.8 80 00\r\n") == null, true);
    expect_eq(ProxyV1Header.parse("PROXY TCP4 1.2.3.4 5.6.7.8 65536 443\r\n") == null, true);

    // Invalid IPv4 / IPv6
    expect_eq(ProxyV1Header.parse("PROXY TCP4 01.2.3.4 5.6.7.8 80 443\r\n") == null, true);
    expect_eq(ProxyV1Header.parse("PROXY TCP4 256.2.3.4 5.6.7.8 80 443\r\n") == null, true);
    expect_eq(ProxyV1Header.parse("PROXY TCP6 2001::db8::1 ::1 80 443\r\n") == null, true);

    // Over 107 bytes (108 bytes total)
    const over107 =
        "PROXY UNKNOWN 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901\r\n";
    expect_eq(over107.len, usize(108));
    expect_eq(ProxyV1Header.parse(over107) == null, true);
});
