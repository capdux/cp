import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PROXY_V2_SIGNATURE,
    hasProxyV2Signature,
    ProxyV2Command,
    ProxyV2Family,
    ProxyV2Transport,
    ProxyV2TlvIter,
    ProxyV2Header,
} from "cap:net";

test("1. encodeIpv4 and parse round-trip with trailing payload and signature check", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const src: [4]u8 = [203, 0, 113, 19];
    const dst: [4]u8 = [198, 51, 100, 2];

    const enc = ProxyV2Header.encodeIpv4(src, dst, 56324, 443, &buf) ?? unreachable;
    expect_eq(enc.len, usize(28));
    expect_eq(hasProxyV2Signature(enc), true);

    // Append trailing HTTP bytes right after the 28-byte header
    buf[28] = 'G';
    buf[29] = 'E';
    buf[30] = 'T';

    const hdr = ProxyV2Header.parse(buf[0..31]) ?? unreachable;
    expect_eq(hdr.command == ProxyV2Command.proxy, true);
    expect_eq(hdr.family == ProxyV2Family.inet4, true);
    expect_eq(hdr.transport == ProxyV2Transport.stream, true);
    expect_eq(hdr.src_ipv4[0], u8(203));
    expect_eq(hdr.src_ipv4[3], u8(19));
    expect_eq(hdr.dst_ipv4[0], u8(198));
    expect_eq(hdr.dst_ipv4[3], u8(2));
    expect_eq(hdr.src_port, u16(56324));
    expect_eq(hdr.dst_port, u16(443));
    expect_eq(hdr.header_len, usize(28));
    expect_eq(hdr.tlvs_raw.len, usize(0));
    expect_eq(std.mem.eql(u8, buf[hdr.header_len..31], "GET"), true);
});

test("2. encodeLocal and parse handle LOCAL UNSPEC healthcheck headers", () => {
    let buf: [20]u8 = std.mem.zeroes([20]u8);
    const enc = ProxyV2Header.encodeLocal(&buf) ?? unreachable;
    expect_eq(enc.len, usize(16));

    const hdr = ProxyV2Header.parse(enc) ?? unreachable;
    expect_eq(hdr.command == ProxyV2Command.local, true);
    expect_eq(hdr.family == ProxyV2Family.unspec, true);
    expect_eq(hdr.transport == ProxyV2Transport.unspec, true);
    expect_eq(hdr.header_len, usize(16));
    expect_eq(hdr.tlvs_raw.len, usize(0));

    // Too small output buffers return null
    let tiny15: [15]u8 = std.mem.zeroes([15]u8);
    expect_eq(ProxyV2Header.encodeLocal(&tiny15) == null, true);
    let tiny27: [27]u8 = std.mem.zeroes([27]u8);
    expect_eq(ProxyV2Header.encodeIpv4([1, 2, 3, 4], [5, 6, 7, 8], 1, 2, &tiny27) == null, true);
});

test("3. parse IPv4 + TLV extensions (ALPN 0x01 and AUTHORITY 0x02) with ProxyV2TlvIter", () => {
    // 16 fixed + 12 ipv4 + 5 (ALPN "h2") + 10 (AUTHORITY "api.cp") = 43 bytes
    let pkt: [43]u8 = std.mem.zeroes([43]u8);
    let i: usize = 0;
    while (i < 12) {
        pkt[i] = PROXY_V2_SIGNATURE[i];
        i += 1;
    }
    pkt[12] = 0x21; // ver=2, PROXY
    pkt[13] = 0x11; // AF_INET, STREAM
    pkt[14] = 0x00;
    pkt[15] = 27;   // 12 + 5 + 10

    // src=10.0.0.1, dst=10.0.0.2, ports 1000 -> 8443
    pkt[16] = 10;
    pkt[19] = 1;
    pkt[20] = 10;
    pkt[23] = 2;
    pkt[24] = 0x03;
    pkt[25] = 0xE8;
    pkt[26] = 0x20;
    pkt[27] = 0xFB;

    // TLV 1: type=0x01 (ALPN), len=2, val="h2"
    pkt[28] = 0x01;
    pkt[29] = 0x00;
    pkt[30] = 0x02;
    pkt[31] = 'h';
    pkt[32] = '2';

    // TLV 2: type=0x02 (AUTHORITY), len=7, val="api.cp!"
    pkt[33] = 0x02;
    pkt[34] = 0x00;
    pkt[35] = 0x07;
    pkt[36] = 'a';
    pkt[37] = 'p';
    pkt[38] = 'i';
    pkt[39] = '.';
    pkt[40] = 'c';
    pkt[41] = 'p';
    pkt[42] = '!';

    const hdr = ProxyV2Header.parse(pkt[0..43]) ?? unreachable;
    expect_eq(hdr.header_len, usize(43));
    expect_eq(hdr.src_port, u16(1000));
    expect_eq(hdr.dst_port, u16(8443));
    expect_eq(hdr.tlvs_raw.len, usize(15));

    let it = ProxyV2TlvIter.init(hdr.tlvs_raw);
    const t1 = it.next() ?? unreachable;
    expect_eq(t1.tlv_type, u8(0x01));
    expect_eq(std.mem.eql(u8, t1.value, "h2"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.tlv_type, u8(0x02));
    expect_eq(std.mem.eql(u8, t2.value, "api.cp!"), true);

    expect_eq(it.next() == null, true);
});

test("4. parse IPv6 DGRAM header extracts 16-byte addresses and ports", () => {
    // 16 fixed + 36 ipv6 = 52 bytes
    let pkt: [52]u8 = std.mem.zeroes([52]u8);
    let i: usize = 0;
    while (i < 12) {
        pkt[i] = PROXY_V2_SIGNATURE[i];
        i += 1;
    }
    pkt[12] = 0x21; // ver=2, PROXY
    pkt[13] = 0x22; // AF_INET6, DGRAM
    pkt[14] = 0x00;
    pkt[15] = 36;

    // src = 2001:db8::1, dst = ::1
    pkt[16] = 0x20;
    pkt[17] = 0x01;
    pkt[18] = 0x0d;
    pkt[19] = 0xb8;
    pkt[31] = 0x01;
    pkt[47] = 0x01;

    // src_port = 5353 (0x14E9), dst_port = 53 (0x0035)
    pkt[48] = 0x14;
    pkt[49] = 0xE9;
    pkt[50] = 0x00;
    pkt[51] = 0x35;

    const hdr = ProxyV2Header.parse(pkt[0..52]) ?? unreachable;
    expect_eq(hdr.family == ProxyV2Family.inet6, true);
    expect_eq(hdr.transport == ProxyV2Transport.dgram, true);
    expect_eq(hdr.src_ipv6[0], u8(0x20));
    expect_eq(hdr.src_ipv6[15], u8(0x01));
    expect_eq(hdr.dst_ipv6[15], u8(0x01));
    expect_eq(hdr.src_port, u16(5353));
    expect_eq(hdr.dst_port, u16(53));
    expect_eq(hdr.header_len, usize(52));
});

test("5. Malformed signatures, invalid version/command/family, truncated payload and TLV are rejected", () => {
    expect_eq(hasProxyV2Signature("PROXY TCP4 "), false);
    expect_eq(ProxyV2Header.parse("short") == null, true);

    let base: [28]u8 = std.mem.zeroes([28]u8);
    * = ProxyV2Header.encodeIpv4([1, 2, 3, 4], [5, 6, 7, 8], 80, 443, &base);

    // Wrong version (ver=1)
    let bad_ver = base;
    bad_ver[12] = 0x11;
    expect_eq(ProxyV2Header.parse(bad_ver[0..28]) == null, true);

    // Invalid command (0x02)
    let bad_cmd = base;
    bad_cmd[12] = 0x22;
    expect_eq(ProxyV2Header.parse(bad_cmd[0..28]) == null, true);

    // Invalid family (0x04)
    let bad_fam = base;
    bad_fam[13] = 0x41;
    expect_eq(ProxyV2Header.parse(bad_fam[0..28]) == null, true);

    // Invalid transport (0x03)
    let bad_proto = base;
    bad_proto[13] = 0x13;
    expect_eq(ProxyV2Header.parse(bad_proto[0..28]) == null, true);

    // Declared payload_len exceeds buffer length
    expect_eq(ProxyV2Header.parse(base[0..27]) == null, true);

    // IPv4 family with payload_len < 12
    let short_v4 = base;
    short_v4[15] = 8;
    expect_eq(ProxyV2Header.parse(short_v4[0..24]) == null, true);

    // Truncated TLV terminates iterator safely
    const bad_tlv: [4]u8 = [0x01, 0x00, 0x10, 0xAA];
    let tit = ProxyV2TlvIter.init(bad_tlv[0..4]);
    expect_eq(tit.next() == null, true);
    expect_eq(tit.next() == null, true);
});
