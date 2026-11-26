// cap/test/testCapIpaddr.cp
// Comprehensive unit tests for @cap/ipaddr (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    Ipv4Addr,
    Ipv6Addr,
    CidrV4,
    CidrV6,
    parseIpv4,
    parseIpv6,
    parseCidrV4,
    parseCidrV6,
    isValidIp,
    isValidIpv4,
    isValidIpv6,
    ipKind,
} from "../../packages/@cap/ipaddr/src/index.cp";

fn main() {
    test("1. @cap/ipaddr — IPv4 parse, toU32, toStr & all range() classifications", () => {
        const ip = parseIpv4("192.168.1.42");
        expect_eq(ip.ok, true);
        expect_eq(ip.octets[0], 192 as u8);
        expect_eq(ip.octets[1], 168 as u8);
        expect_eq(ip.octets[2], 1 as u8);
        expect_eq(ip.octets[3], 42 as u8);
        expect_eq(ip.toU32(), 0xC0A8012A as u32);
        expect_eq(ip.kind(), "ipv4");

        let buf: [32]u8 = undefined;
        const n = ip.toStr(buf[0..32]);
        expect_eq(buf[0..n], "192.168.1.42");

        // Roundtrip fromU32
        const rebuilt = Ipv4Addr.fromU32(0xC0A8012A as u32);
        expect_eq(rebuilt.equals(ip), true);

        // Exhaustive IPv4 range() checks
        expect_eq(parseIpv4("0.0.0.0").range(), "unspecified");
        expect_eq(parseIpv4("0.12.34.56").range(), "unspecified");
        expect_eq(parseIpv4("255.255.255.255").range(), "broadcast");
        expect_eq(parseIpv4("224.0.0.1").range(), "multicast");
        expect_eq(parseIpv4("239.255.255.255").range(), "multicast");
        expect_eq(parseIpv4("169.254.1.10").range(), "linkLocal");
        expect_eq(parseIpv4("127.0.0.1").range(), "loopback");
        expect_eq(parseIpv4("127.255.0.1").range(), "loopback");
        expect_eq(parseIpv4("100.64.0.1").range(), "carrierGradeNat");
        expect_eq(parseIpv4("100.127.255.254").range(), "carrierGradeNat");
        expect_eq(parseIpv4("10.0.0.1").range(), "private");
        expect_eq(parseIpv4("172.16.0.1").range(), "private");
        expect_eq(parseIpv4("172.31.255.254").range(), "private");
        expect_eq(parseIpv4("192.168.0.1").range(), "private");
        expect_eq(parseIpv4("240.0.0.1").range(), "reserved");
        expect_eq(parseIpv4("255.255.255.254").range(), "reserved");
        expect_eq(parseIpv4("8.8.8.8").range(), "unicast");
        expect_eq(parseIpv4("172.32.0.1").range(), "unicast");
        expect_eq(parseIpv4("100.63.255.255").range(), "unicast");
        expect_eq(parseIpv4("100.128.0.1").range(), "unicast");

        // Subnet mask helper
        expect_eq(parseIpv4("255.255.255.0").prefixLengthFromSubnetMask(), 24 as i8);
        expect_eq(parseIpv4("255.0.255.0").prefixLengthFromSubnetMask(), -1 as i8);
    });

    test("2. @cap/ipaddr — IPv4 CIDR matching including /0, /32 & boundaries", () => {
        const net = parseIpv4("10.20.30.0");
        expect_eq(parseIpv4("10.20.30.0").matchCidr(net, 24), true);
        expect_eq(parseIpv4("10.20.30.255").matchCidr(net, 24), true);
        expect_eq(parseIpv4("10.20.31.0").matchCidr(net, 24), false);

        // /0 matches every valid IPv4
        expect_eq(parseIpv4("1.2.3.4").matchCidr(parseIpv4("200.100.50.25"), 0), true);

        // /32 exact match only
        const host = parseIpv4("192.168.10.5");
        expect_eq(parseIpv4("192.168.10.5").matchCidr(host, 32), true);
        expect_eq(parseIpv4("192.168.10.4").matchCidr(host, 32), false);
        expect_eq(parseIpv4("192.168.10.6").matchCidr(host, 32), false);

        // Non-byte-aligned prefix /20 (172.16.0.0 .. 172.16.15.255)
        const c20 = parseCidrV4("172.16.0.0/20");
        expect_eq(c20.ok, true);
        expect_eq(c20.prefix, 20 as u8);
        expect_eq(c20.matches(parseIpv4("172.16.15.255")), true);
        expect_eq(c20.matches(parseIpv4("172.16.16.0")), false);

        // Invalid prefix rejected
        expect_eq(parseIpv4("10.0.0.1").matchCidr(net, 33), false);
        expect_eq(parseCidrV4("10.0.0.0/33").ok, false);
        expect_eq(parseCidrV4("10.0.0.0/08").ok, false);
        expect_eq(parseCidrV4("10.0.0.0").ok, false);
    });

    test("3. @cap/ipaddr — IPv6 full parsing (:: compression, mixed case) & RFC 5952 toStr", () => {
        let buf: [64]u8 = undefined;

        const v1 = parseIpv6("2001:0db8:0000:0000:0000:0000:0000:0001");
        expect_eq(v1.ok, true);
        expect_eq(v1.parts[0], 0x2001 as u16);
        expect_eq(v1.parts[1], 0x0db8 as u16);
        expect_eq(v1.parts[7], 0x0001 as u16);
        const n1 = v1.toStr(buf[0..64]);
        expect_eq(buf[0..n1], "2001:db8::1");

        // :: and ::1 and trailing ::
        const unspec = parseIpv6("::");
        expect_eq(unspec.ok, true);
        const n2 = unspec.toStr(buf[0..64]);
        expect_eq(buf[0..n2], "::");

        const lo = parseIpv6("0:0:0:0:0:0:0:1");
        expect_eq(lo.ok, true);
        const n3 = lo.toStr(buf[0..64]);
        expect_eq(buf[0..n3], "::1");

        const trail = parseIpv6("FE80::");
        expect_eq(trail.ok, true);
        const n4 = trail.toStr(buf[0..64]);
        expect_eq(buf[0..n4], "fe80::");

        // RFC 5952 §4.2.2: single 0 group MUST NOT be compressed
        const singleZero = parseIpv6("2001:db8:0:1:1:1:1:1");
        expect_eq(singleZero.ok, true);
        const n5 = singleZero.toStr(buf[0..64]);
        expect_eq(buf[0..n5], "2001:db8:0:1:1:1:1:1");

        // RFC 5952 §4.2.3: longest zero run wins; equal runs pick first
        const longestRun = parseIpv6("2001:0:0:1:0:0:0:1");
        expect_eq(longestRun.ok, true);
        const n6 = longestRun.toStr(buf[0..64]);
        expect_eq(buf[0..n6], "2001:0:0:1::1");

        const equalRuns = parseIpv6("2001:db8:0:0:1:0:0:1");
        expect_eq(equalRuns.ok, true);
        const n7 = equalRuns.toStr(buf[0..64]);
        expect_eq(buf[0..n7], "2001:db8::1:0:0:1");

        // Mixed-case hex normalized to lowercase
        const upper = parseIpv6("2001:DB8:ABCD:EF01::1");
        expect_eq(upper.ok, true);
        const n8 = upper.toStr(buf[0..64]);
        expect_eq(buf[0..n8], "2001:db8:abcd:ef01::1");
    });

    test("4. @cap/ipaddr — IPv4-mapped IPv6 (::ffff:192.168.1.1) bidirectional conversion", () => {
        const mapped = parseIpv6("::ffff:192.168.1.1");
        expect_eq(mapped.ok, true);
        expect_eq(mapped.isIpv4Mapped(), true);
        expect_eq(mapped.parts[5], 0xFFFF as u16);
        expect_eq(mapped.parts[6], 0xC0A8 as u16);
        expect_eq(mapped.parts[7], 0x0101 as u16);
        expect_eq(mapped.range(), "ipv4Mapped");

        // IPv6 -> IPv4
        const v4 = mapped.toIpv4();
        expect_eq(v4.ok, true);
        let buf: [32]u8 = undefined;
        const n = v4.toStr(buf[0..32]);
        expect_eq(buf[0..n], "192.168.1.1");

        // IPv4 -> IPv6 roundtrip
        const back = v4.toIpv4MappedIpv6();
        expect_eq(back.ok, true);
        expect_eq(back.isIpv4Mapped(), true);
        expect_eq(back.equals(mapped), true);

        // Full uncompressed embedded IPv4 form
        const fullMapped = parseIpv6("0:0:0:0:0:FFFF:192.0.2.128");
        expect_eq(fullMapped.ok, true);
        expect_eq(fullMapped.isIpv4Mapped(), true);
        const v4b = fullMapped.toIpv4();
        const nb = v4b.toStr(buf[0..32]);
        expect_eq(buf[0..nb], "192.0.2.128");

        // Non-mapped IPv6 must return ok == false on toIpv4()
        const normalV6 = parseIpv6("2001:db8::1");
        expect_eq(normalV6.isIpv4Mapped(), false);
        expect_eq(normalV6.toIpv4().ok, false);
    });

    test("5. @cap/ipaddr — IPv6 CIDR matching & range() classifications", () => {
        // Range classifications
        expect_eq(parseIpv6("::").range(), "unspecified");
        expect_eq(parseIpv6("::1").range(), "loopback");
        expect_eq(parseIpv6("fe80::1").range(), "linkLocal");
        expect_eq(parseIpv6("febf:ffff::1").range(), "linkLocal");
        expect_eq(parseIpv6("fc00::1").range(), "uniqueLocal");
        expect_eq(parseIpv6("fd12:3456:789a::1").range(), "uniqueLocal");
        expect_eq(parseIpv6("::ffff:10.0.0.1").range(), "ipv4Mapped");
        expect_eq(parseIpv6("ff02::1").range(), "multicast");
        expect_eq(parseIpv6("2001:db8::1").range(), "unicast");

        // CIDR matching (/0, /32, /64, /128, non-aligned /37)
        const c32 = parseCidrV6("2001:db8::/32");
        expect_eq(c32.ok, true);
        expect_eq(c32.matches(parseIpv6("2001:db8:ffff::1")), true);
        expect_eq(c32.matches(parseIpv6("2001:db9::1")), false);

        const c0 = parseCidrV6("::/0");
        expect_eq(c0.ok, true);
        expect_eq(c0.matches(parseIpv6("2606:4700:4700::1111")), true);

        const c128 = parseCidrV6("2001:db8::1/128");
        expect_eq(c128.ok, true);
        expect_eq(c128.matches(parseIpv6("2001:db8::1")), true);
        expect_eq(c128.matches(parseIpv6("2001:db8::2")), false);

        // Non-16-bit-aligned prefix: fe80::/10 (top 10 bits: 1111 1110 10xx xxxx -> fe80..febf)
        const c10 = parseCidrV6("fe80::/10");
        expect_eq(c10.ok, true);
        expect_eq(c10.matches(parseIpv6("febf::1")), true);
        expect_eq(c10.matches(parseIpv6("fec0::1")), false);

        expect_eq(parseCidrV6("2001:db8::/129").ok, false);
        expect_eq(parseCidrV6("2001:db8::/064").ok, false);
    });

    test("6. @cap/ipaddr — strict rejection of malformed IPs & isValidIp / ipKind", () => {
        // Invalid IPv4
        expect_eq(parseIpv4("").ok, false);
        expect_eq(parseIpv4("256.0.0.1").ok, false);
        expect_eq(parseIpv4("01.2.3.4").ok, false);
        expect_eq(parseIpv4("1.02.3.4").ok, false);
        expect_eq(parseIpv4("1.2.3").ok, false);
        expect_eq(parseIpv4("1.2.3.4.5").ok, false);
        expect_eq(parseIpv4("1.2.3.4.").ok, false);
        expect_eq(parseIpv4(".1.2.3.4").ok, false);
        expect_eq(parseIpv4("1.2.3.4 ").ok, false);
        expect_eq(parseIpv4("1.2.3.-1").ok, false);

        // Invalid IPv6
        expect_eq(parseIpv6("").ok, false);
        expect_eq(parseIpv6(":").ok, false);
        expect_eq(parseIpv6(":::").ok, false);
        expect_eq(parseIpv6("1:2::3::4").ok, false);
        expect_eq(parseIpv6(":1:2:3:4:5:6:7").ok, false);
        expect_eq(parseIpv6("1:2:3:4:5:6:7:").ok, false);
        expect_eq(parseIpv6("12345::1").ok, false);
        expect_eq(parseIpv6("2001:db8::zzzz").ok, false);
        expect_eq(parseIpv6("1:2:3:4:5:6:7:8:9").ok, false);
        expect_eq(parseIpv6("1:2:3:4:5:6:7:8::").ok, false);
        expect_eq(parseIpv6("::ffff:256.0.0.1").ok, false);
        expect_eq(parseIpv6("::ffff:01.2.3.4").ok, false);

        // isValidIp / isValidIpv4 / isValidIpv6 / ipKind
        expect_eq(isValidIp("127.0.0.1"), true);
        expect_eq(isValidIp("::1"), true);
        expect_eq(isValidIp("256.0.0.1"), false);
        expect_eq(isValidIp("1:2::3::4"), false);
        expect_eq(isValidIp(""), false);

        expect_eq(isValidIpv4("10.0.0.1"), true);
        expect_eq(isValidIpv4("::1"), false);
        expect_eq(isValidIpv6("::1"), true);
        expect_eq(isValidIpv6("10.0.0.1"), false);

        expect_eq(ipKind("10.0.0.1"), "ipv4");
        expect_eq(ipKind("2001:db8::1"), "ipv6");
        expect_eq(ipKind("not-an-ip"), "invalid");
    });
}
