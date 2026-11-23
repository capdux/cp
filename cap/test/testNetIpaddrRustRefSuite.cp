// Reference-hardened test suite against krisprice/ipnet (MIT/Apache-2.0) & RFC 4291/5952/6890
// Covers:
//   1. Strict IPv4 & IPv6 parsing (rejecting octal leading zeros like 0177.0.0.1, double ::, zone IDs) & RFC 5952 formatting
//   2. RFC 6890 SSRF classification & IPv4-mapped IPv6 bypass blocking (::ffff:127.0.0.1, ::ffff:169.254.169.254)
//   3. Full Ipv4Net & Ipv6Net CIDR algebra (0..=32, 0..=128, /0 & /128 boundary, network/broadcast/netmask/hostmask/contains/overlaps/supernet)
//   4. 16-lane @Vector(16, u8) SIMD IPv6 subnet & equality 0-diff parity vs scalar reference
//   5. Deterministic PRNG Fuzz (>= 1,600 malformed IP/CIDR strings + random IPv6 prefix containment parity)

import { test, expect_eq } from "cap:test";
import {
    Ipv4Addr,
    Ipv6Addr,
    IpAddr,
    Ipv4Net,
    Ipv6Net,
    simdIpv6Eql,
    simdIpv6EqlScalar,
    simdIpv6InSubnet,
    simdIpv6InSubnetScalar,
} from "../../lib/net/ipaddr";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn main() {
    test("1. Strict IPv4/IPv6 parsing (octal/double-::/zone-ID rejection) & RFC 5952 format", () => {
        // Valid IPv4
        expect_eq(Ipv4Addr.parse("0.0.0.0") != null, true);
        expect_eq(Ipv4Addr.parse("127.0.0.1") != null, true);
        expect_eq(Ipv4Addr.parse("255.255.255.255") != null, true);
        // Invalid IPv4 (octal SSRF bypass, overflow, wrong parts)
        expect_eq(Ipv4Addr.parse("0177.0.0.1") == null, true);
        expect_eq(Ipv4Addr.parse("127.0.0.01") == null, true);
        expect_eq(Ipv4Addr.parse("256.0.0.1") == null, true);
        expect_eq(Ipv4Addr.parse("127.1") == null, true);
        expect_eq(Ipv4Addr.parse("127.0.0.1.") == null, true);

        // Valid IPv6 & RFC 5952 canonical formatting
        let fbuf: [64]u8 = undefined;
        const v6_doc = Ipv6Addr.parse("2001:0DB8:0000:0000:0000:0000:0000:0001").?;
        const fmt_doc: ([]const u8)? = v6_doc.format(&fbuf);
        expect_eq(fmt_doc orelse "", "2001:db8::1");

        const v6_mapped = Ipv6Addr.parse("::ffff:192.0.2.128").?;
        const fmt_mapped: ([]const u8)? = v6_mapped.format(&fbuf);
        expect_eq(fmt_mapped orelse "", "::ffff:192.0.2.128");

        // Invalid IPv6 (double ::, zone ID, octal embedded IPv4, trailing :, too many groups)
        expect_eq(Ipv6Addr.parse("2001::db8::1") == null, true);
        expect_eq(Ipv6Addr.parse("fe80::1%eth0") == null, true);
        expect_eq(Ipv6Addr.parse("::ffff:0177.0.0.1") == null, true);
        expect_eq(Ipv6Addr.parse("2001:db8:") == null, true);
        expect_eq(Ipv6Addr.parse("1:2:3:4:5:6:7:8:9") == null, true);
    });

    test("2. RFC 6890 SSRF classification & IPv4-mapped IPv6 bypass defense", () => {
        // Public global unicast -> ALLOWED
        expect_eq(IpAddr.parse("8.8.8.8").?.isGlobalUnicast(), true);
        expect_eq(IpAddr.parse("1.1.1.1").?.isGlobalUnicast(), true);
        expect_eq(IpAddr.parse("2606:4700:4700::1111").?.isGlobalUnicast(), true);
        expect_eq(IpAddr.parse("::ffff:8.8.8.8").?.isGlobalUnicast(), true);

        // Non-global / SSRF targets -> BLOCKED
        expect_eq(IpAddr.parse("127.0.0.1").?.isLoopback(), true);
        expect_eq(IpAddr.parse("127.0.0.1").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("10.0.0.1").?.isPrivate(), true);
        expect_eq(IpAddr.parse("10.0.0.1").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("172.16.5.1").?.isPrivate(), true);
        expect_eq(IpAddr.parse("192.168.1.1").?.isPrivate(), true);
        expect_eq(IpAddr.parse("169.254.169.254").?.isLinkLocal(), true);
        expect_eq(IpAddr.parse("169.254.169.254").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("100.64.0.1").?.isCgnat(), true);
        expect_eq(IpAddr.parse("100.64.0.1").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("192.0.2.1").?.isDocumentation(), true);
        expect_eq(IpAddr.parse("192.0.2.1").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("224.0.0.1").?.isMulticast(), true);
        expect_eq(IpAddr.parse("0.0.0.0").?.isGlobalUnicast(), false);
        expect_eq(Ipv4Addr.parse("0.1.2.3").?.isThisNetwork(), true);
        expect_eq(Ipv4Addr.parse("0.1.2.3").?.isGlobalUnicast(), false);
        expect_eq(Ipv4Addr.parse("198.18.0.1").?.isBenchmarking(), true);
        expect_eq(Ipv4Addr.parse("198.18.0.1").?.isGlobalUnicast(), false);
        expect_eq(Ipv4Addr.parse("192.0.0.1").?.isIetfProtocol(), true);
        expect_eq(Ipv4Addr.parse("192.0.0.1").?.isGlobalUnicast(), false);
        expect_eq(Ipv4Addr.parse("240.0.0.1").?.isReserved(), true);
        expect_eq(Ipv4Addr.parse("240.0.0.1").?.isGlobalUnicast(), false);

        // IPv6 special ranges & IPv4-mapped SSRF bypass MUST BE BLOCKED
        expect_eq(IpAddr.parse("::1").?.isLoopback(), true);
        expect_eq(IpAddr.parse("::1").?.isGlobalUnicast(), false);
        expect_eq(Ipv6Addr.parse("::1").?.isIpv4Compatible(), false);
        expect_eq(Ipv6Addr.parse("::").?.isIpv4Compatible(), false);
        expect_eq(Ipv6Addr.parse("::192.0.2.1").?.isIpv4Compatible(), true);
        expect_eq(Ipv6Addr.parse("::192.0.2.1").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("fc00::1").?.isPrivate(), true);
        expect_eq(IpAddr.parse("fc00::1").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("fe80::1").?.isLinkLocal(), true);
        expect_eq(IpAddr.parse("fe80::1").?.isGlobalUnicast(), false);
        expect_eq(IpAddr.parse("2001:db8::1").?.isDocumentation(), true);
        expect_eq(IpAddr.parse("2001:db8::1").?.isGlobalUnicast(), false);
        expect_eq(Ipv6Addr.parse("100::1").?.isDiscardOnly(), true);
        expect_eq(Ipv6Addr.parse("100::1").?.isGlobalUnicast(), false);
        expect_eq(Ipv6Addr.parse("2001:2::1").?.isBenchmarking(), true);
        expect_eq(Ipv6Addr.parse("2001:2::1").?.isGlobalUnicast(), false);
        expect_eq(Ipv6Addr.parse("fec0::1").?.isGlobalUnicast(), false);
        expect_eq(Ipv6Addr.parse("64:ff9b::1").?.isGlobalUnicast(), false);

        // Critical IPv4-mapped IPv6 bypass checks
        const m_loop = IpAddr.parse("::ffff:127.0.0.1").?;
        expect_eq(m_loop.isLoopback(), true);
        expect_eq(m_loop.isGlobalUnicast(), false);

        const m_meta = IpAddr.parse("::ffff:169.254.169.254").?;
        expect_eq(m_meta.isLinkLocal(), true);
        expect_eq(m_meta.isGlobalUnicast(), false);

        const m_priv = IpAddr.parse("::ffff:10.0.0.5").?;
        expect_eq(m_priv.isPrivate(), true);
        expect_eq(m_priv.isGlobalUnicast(), false);

        const m_cgnat = IpAddr.parse("::ffff:100.64.1.1").?;
        expect_eq(m_cgnat.isCgnat(), true);
        expect_eq(m_cgnat.isGlobalUnicast(), false);
    });

    test("3. Full Ipv4Net & Ipv6Net CIDR algebra (/0, /32, /128 boundaries, supernet, overlaps)", () => {
        let fbuf: [64]u8 = undefined;

        // IPv4 CIDR
        const n4 = Ipv4Net.parse("192.168.1.42/24").?;
        expect_eq(n4.network().eql(Ipv4Addr.parse("192.168.1.0").?), true);
        expect_eq(n4.broadcast().eql(Ipv4Addr.parse("192.168.1.255").?), true);
        expect_eq(n4.netmask().eql(Ipv4Addr.parse("255.255.255.0").?), true);
        expect_eq(n4.hostmask().eql(Ipv4Addr.parse("0.0.0.255").?), true);
        expect_eq(n4.contains(Ipv4Addr.parse("192.168.1.200").?), true);
        expect_eq(n4.contains(Ipv4Addr.parse("192.168.2.1").?), false);

        const n4_sub = Ipv4Net.parse("192.168.1.128/25").?;
        expect_eq(n4.containsNet(n4_sub), true);
        expect_eq(n4_sub.containsNet(n4), false);
        expect_eq(n4.overlaps(n4_sub), true);
        expect_eq(n4_sub.supernet().?.eql(n4), true);

        const fmt4: ([]const u8)? = n4.format(&fbuf);
        expect_eq(fmt4 orelse "", "192.168.1.0/24");

        // /0 & /32 boundaries (no shift UB)
        const n4_zero = Ipv4Net.parse("0.0.0.0/0").?;
        expect_eq(n4_zero.contains(Ipv4Addr.parse("8.8.8.8").?), true);
        expect_eq(n4_zero.supernet() == null, true);
        const n4_host = Ipv4Net.parse("8.8.8.8/32").?;
        expect_eq(n4_host.contains(Ipv4Addr.parse("8.8.8.8").?), true);
        expect_eq(n4_host.contains(Ipv4Addr.parse("8.8.8.9").?), false);

        // Invalid prefix_len (overflow, leading zero)
        expect_eq(Ipv4Net.parse("10.0.0.0/33") == null, true);
        expect_eq(Ipv4Net.parse("10.0.0.0/08") == null, true);
        expect_eq(Ipv6Net.parse("2001:db8::/129") == null, true);
        expect_eq(Ipv6Net.parse("2001:db8::/064") == null, true);

        // IPv6 CIDR (/0, /32, /64, /128)
        const n6 = Ipv6Net.parse("2001:db8:abcd:1234::5/32").?;
        expect_eq(n6.network().eql(Ipv6Addr.parse("2001:db8::").?), true);
        expect_eq(n6.broadcast().eql(Ipv6Addr.parse("2001:db8:ffff:ffff:ffff:ffff:ffff:ffff").?), true);
        expect_eq(n6.contains(Ipv6Addr.parse("2001:db8:9999::1").?), true);
        expect_eq(n6.contains(Ipv6Addr.parse("2001:db9::1").?), false);

        const n6_sub = Ipv6Net.parse("2001:db8:1::/48").?;
        expect_eq(n6.containsNet(n6_sub), true);
        expect_eq(n6.overlaps(n6_sub), true);
        expect_eq(n6_sub.supernet().?.prefix_len, 47 as u8);

        const n6_zero = Ipv6Net.parse("::/0").?;
        expect_eq(n6_zero.contains(Ipv6Addr.parse("2606:4700::1").?), true);
        expect_eq(n6_zero.supernet() == null, true);

        const n6_128 = Ipv6Net.parse("::1/128").?;
        expect_eq(n6_128.contains(Ipv6Addr.parse("::1").?), true);
        expect_eq(n6_128.contains(Ipv6Addr.parse("::2").?), false);
    });

    test("4. 16-lane SIMD IPv6 subnet & equality 0-diff parity vs scalar (all 0..=128 prefixes)", () => {
        const ip1: [16]u8 = Ipv6Addr.parse("2001:db8:85a3:0:0:8a2e:370:7334").?.toBytes();
        const ip2: [16]u8 = Ipv6Addr.parse("2001:db8:85a3:0:0:8a2e:370:7335").?.toBytes();
        const ip3: [16]u8 = Ipv6Addr.parse("fe80::1").?.toBytes();

        expect_eq(simdIpv6Eql(ip1, ip1), simdIpv6EqlScalar(ip1, ip1));
        expect_eq(simdIpv6Eql(ip1, ip2), simdIpv6EqlScalar(ip1, ip2));
        expect_eq(simdIpv6Eql(ip1, ip3), simdIpv6EqlScalar(ip1, ip3));

        let pfx: u8 = 0;
        while (pfx <= 128) {
            expect_eq(simdIpv6InSubnet(ip1, ip2, pfx), simdIpv6InSubnetScalar(ip1, ip2, pfx));
            expect_eq(simdIpv6InSubnet(ip1, ip3, pfx), simdIpv6InSubnetScalar(ip1, ip3, pfx));
            if (pfx == 128) break;
            pfx += 1;
        }
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 malformed IP/CIDR strings + random IPv6 SIMD parity)", () => {
        const alphabet: []const u8 = "0123456789abcdefABCDEF.:/%[] \t\r\n\x00gGzZ";
        let seed: u64 = 0x5EED_1A04_2026_0033;
        let fuzz_buf: [64]u8 = undefined;

        // 5A. 1,000 malformed IP / CIDR string fuzz iterations
        let iter: usize = 0;
        while (iter < 1000) {
            const slen: usize = ((nextRng(&seed) % 56) + 1) as usize;
            let k: usize = 0;
            while (k < slen) {
                const idx: usize = (nextRng(&seed) % (alphabet.len as u64)) as usize;
                fuzz_buf[k] = alphabet[idx];
                k += 1;
            }
            const sample = fuzz_buf[0..slen];
            * = Ipv4Addr.parse(sample);
            * = Ipv6Addr.parse(sample);
            * = IpAddr.parse(sample);
            * = Ipv4Net.parse(sample);
            * = Ipv6Net.parse(sample);
            iter += 1;
        }

        // 5B. 600 random IPv6 16-byte SIMD vs scalar subnet & equality checks
        let a: [16]u8 = undefined;
        let b: [16]u8 = undefined;
        let r_iter: usize = 0;
        while (r_iter < 600) {
            let j: usize = 0;
            while (j < 16) {
                a[j] = (nextRng(&seed) & 0xFF) as u8;
                b[j] = if (j < 6) a[j] else ((nextRng(&seed) & 0xFF) as u8);
                j += 1;
            }
            const pfx: u8 = (nextRng(&seed) % 129) as u8;
            expect_eq(simdIpv6Eql(a, b), simdIpv6EqlScalar(a, b));
            expect_eq(simdIpv6InSubnet(a, b, pfx), simdIpv6InSubnetScalar(a, b, pfx));
            r_iter += 1;
        }
    });
}
