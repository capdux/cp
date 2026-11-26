// cap/test/testCapIpRange.cp
// Comprehensive unit tests for @cap/ip-range (IPv4 & CIDR subnet engine)

import { test, expect_eq } from "cap:test";
import {
    Ipv4Result,
    Ipv4Cidr,
    parseIpv4,
    formatIpv4,
    parseCidr,
    cidrContains,
    cidrOverlaps,
    isPrivateIpv4,
    isLoopbackIpv4,
    isLinkLocalIpv4,
    isPublicIpv4,
} from "../../packages/@cap/ip-range/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ip-range — parseIpv4 & formatIpv4 round-trip & rejects leading zeros", () => {
        const r: Ipv4Result = parseIpv4("192.168.1.42");
        expect_eq(r.valid, true);
        let buf: [24]u8 = undefined;
        const n: usize = formatIpv4(r.ip, buf[0..24]);
        expect_eq(sliceEq(buf[0..n], "192.168.1.42"), true);

        const z: Ipv4Result = parseIpv4("0.0.0.0");
        expect_eq(z.valid, true);
        expect_eq(z.ip, 0 as u32);

        // Reject leading zeros (octal ambiguity) and out-of-range octets
        expect_eq(parseIpv4("010.0.0.1").valid, false);
        expect_eq(parseIpv4("192.168.01.1").valid, false);
        expect_eq(parseIpv4("256.0.0.1").valid, false);
        expect_eq(parseIpv4("1.2.3").valid, false);
    });

    test("2. @cap/ip-range — parseCidr for /24, /0 & /32", () => {
        const c24: Ipv4Cidr = parseCidr("192.168.1.130/24");
        expect_eq(c24.valid, true);
        expect_eq(c24.prefix_len, 24 as u8);
        expect_eq(c24.mask, 0xFFFFFF00 as u32);
        expect_eq(c24.network, parseIpv4("192.168.1.0").ip);
        expect_eq(c24.broadcast, parseIpv4("192.168.1.255").ip);

        const c0: Ipv4Cidr = parseCidr("0.0.0.0/0");
        expect_eq(c0.valid, true);
        expect_eq(c0.network, 0 as u32);
        expect_eq(c0.broadcast, 0xFFFFFFFF as u32);

        const c32: Ipv4Cidr = parseCidr("8.8.8.8/32");
        expect_eq(c32.valid, true);
        expect_eq(c32.network, c32.broadcast);
    });

    test("3. @cap/ip-range — cidrContains exact boundaries", () => {
        const c: Ipv4Cidr = parseCidr("10.10.0.0/16");
        expect_eq(cidrContains(c, parseIpv4("10.10.0.0").ip), true);
        expect_eq(cidrContains(c, parseIpv4("10.10.255.255").ip), true);
        expect_eq(cidrContains(c, parseIpv4("10.9.255.255").ip), false);
        expect_eq(cidrContains(c, parseIpv4("10.11.0.0").ip), false);
    });

    test("4. @cap/ip-range — cidrOverlaps overlapping & disjoint subnets", () => {
        const big: Ipv4Cidr = parseCidr("10.0.0.0/8");
        const sub: Ipv4Cidr = parseCidr("10.20.0.0/16");
        const other: Ipv4Cidr = parseCidr("192.168.0.0/16");

        expect_eq(cidrOverlaps(big, sub), true);
        expect_eq(cidrOverlaps(sub, big), true);
        expect_eq(cidrOverlaps(big, other), false);
    });

    test("5. @cap/ip-range — RFC 1918 private, loopback, link-local 169.254.169.254 & isPublicIpv4", () => {
        expect_eq(isPrivateIpv4(parseIpv4("10.0.0.1").ip), true);
        expect_eq(isPrivateIpv4(parseIpv4("172.16.0.1").ip), true);
        expect_eq(isPrivateIpv4(parseIpv4("172.31.255.254").ip), true);
        expect_eq(isPrivateIpv4(parseIpv4("172.32.0.1").ip), false);
        expect_eq(isPrivateIpv4(parseIpv4("192.168.0.1").ip), true);

        expect_eq(isLoopbackIpv4(parseIpv4("127.0.0.1").ip), true);
        expect_eq(isLinkLocalIpv4(parseIpv4("169.254.169.254").ip), true);

        expect_eq(isPublicIpv4(parseIpv4("8.8.8.8").ip), true);
        expect_eq(isPublicIpv4(parseIpv4("1.1.1.1").ip), true);
        expect_eq(isPublicIpv4(parseIpv4("10.0.0.1").ip), false);
        expect_eq(isPublicIpv4(parseIpv4("127.0.0.1").ip), false);
        expect_eq(isPublicIpv4(parseIpv4("169.254.169.254").ip), false);
        expect_eq(isPublicIpv4(parseIpv4("100.64.0.1").ip), false);
        expect_eq(isPublicIpv4(parseIpv4("224.0.0.1").ip), false);
        expect_eq(isPublicIpv4(parseIpv4("255.255.255.255").ip), false);
    });

    test("6. @cap/ip-range — rejects invalid CIDR & tiny buffers", () => {
        expect_eq(parseCidr("10.0.0.0/33").valid, false);
        expect_eq(parseCidr("10.0.0.0").valid, false);
        expect_eq(parseCidr("10.0.0.0/08").valid, false);
        expect_eq(parseCidr("bad/24").valid, false);

        let tiny: [5]u8 = undefined;
        expect_eq(formatIpv4(parseIpv4("192.168.1.1").ip, tiny[0..5]), 0 as usize);
    });
}
