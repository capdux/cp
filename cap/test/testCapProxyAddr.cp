// cap/test/testCapProxyAddr.cp
// Comprehensive unit tests for @cap/proxy-addr

import { test, expect_eq } from "cap:test";
import {
    TRUST_NONE,
    TRUST_LOOPBACK,
    TRUST_LINKLOCAL,
    TRUST_UNIQUELOCAL,
    TRUST_ALL_PRIVATE,
    IpSpan,
    isLoopback,
    isLinkLocal,
    isUniqueLocal,
    isTrustedBuiltin,
    parseXffAddresses,
    resolveClientIp
} from "../../packages/@cap/proxy-addr/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/proxy-addr — No XFF returns remote_addr directly", () => {
        let out: [64]u8 = undefined;
        let n1 = resolveClientIp("203.0.113.9", "", TRUST_ALL_PRIVATE, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n1], "203.0.113.9"), true);

        // Even when remote_addr is trusted loopback, empty XFF returns remote_addr
        let n2 = resolveClientIp("127.0.0.1", "", TRUST_LOOPBACK, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n2], "127.0.0.1"), true);
    });

    test("2. @cap/proxy-addr — Untrusted proxy prevents XFF spoofing", () => {
        let out: [64]u8 = undefined;

        // TRUST_NONE: ignores XFF even from 127.0.0.1
        let n1 = resolveClientIp("127.0.0.1", "1.2.3.4", TRUST_NONE, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n1], "127.0.0.1"), true);

        // Public remote_addr sending spoofed XFF is not trusted under TRUST_ALL_PRIVATE
        let n2 = resolveClientIp("198.51.100.42", "10.0.0.99, 127.0.0.1", TRUST_ALL_PRIVATE, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n2], "198.51.100.42"), true);
    });

    test("3. @cap/proxy-addr — Trust loopback traverses 127.0.0.1 & ::ffff:127.0.0.1", () => {
        expect_eq(isLoopback("127.0.0.1"), true);
        expect_eq(isLoopback("127.255.0.5"), true);
        expect_eq(isLoopback("::ffff:127.0.0.1"), true);
        expect_eq(isLoopback("10.0.0.1"), false);

        let out: [64]u8 = undefined;
        let n1 = resolveClientIp("127.0.0.1", "198.51.100.7", TRUST_LOOPBACK, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n1], "198.51.100.7"), true);

        let n2 = resolveClientIp("::ffff:127.0.0.1", " 203.0.113.88 , 127.0.0.2 ", TRUST_LOOPBACK, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n2], "203.0.113.88"), true);
    });

    test("4. @cap/proxy-addr — Multi-hop private proxy chain (10.0.0.1 -> 172.16.0.2 -> 203.0.113.5)", () => {
        expect_eq(isUniqueLocal("10.0.0.1"), true);
        expect_eq(isUniqueLocal("172.16.0.2"), true);
        expect_eq(isUniqueLocal("172.31.255.254"), true);
        expect_eq(isUniqueLocal("172.32.0.1"), false);
        expect_eq(isUniqueLocal("192.168.1.10"), true);
        expect_eq(isLinkLocal("169.254.10.20"), true);

        let xff = "203.0.113.5, 172.16.0.2";
        let spans: [8]IpSpan = undefined;
        let cnt = parseXffAddresses("10.0.0.1", xff, spans[0..8]);
        expect_eq(cnt, 2 as usize);
        expect_eq(sliceEq(xff[spans[0].start..(spans[0].start + spans[0].len)], "172.16.0.2"), true);
        expect_eq(sliceEq(xff[spans[1].start..(spans[1].start + spans[1].len)], "203.0.113.5"), true);

        let out: [64]u8 = undefined;
        let n = resolveClientIp("10.0.0.1", xff, TRUST_UNIQUELOCAL, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n], "203.0.113.5"), true);
    });

    test("5. @cap/proxy-addr — max_hops limits proxy traversal depth", () => {
        let out: [64]u8 = undefined;
        let xff = "203.0.113.5, 172.16.0.2, 192.168.1.5";

        // max_hops = 1: skips 10.0.0.1 only, stops at 192.168.1.5
        let n1 = resolveClientIp("10.0.0.1", xff, TRUST_ALL_PRIVATE, 1, out[0..64]);
        expect_eq(sliceEq(out[0..n1], "192.168.1.5"), true);

        // max_hops = 2: skips 10.0.0.1 and 192.168.1.5, stops at 172.16.0.2
        let n2 = resolveClientIp("10.0.0.1", xff, TRUST_ALL_PRIVATE, 2, out[0..64]);
        expect_eq(sliceEq(out[0..n2], "172.16.0.2"), true);

        // max_hops = 3: reaches real public client 203.0.113.5
        let n3 = resolveClientIp("10.0.0.1", xff, TRUST_ALL_PRIVATE, 3, out[0..64]);
        expect_eq(sliceEq(out[0..n3], "203.0.113.5"), true);
    });

    test("6. @cap/proxy-addr — IPv6 ::1 / fe80::1 / fc00::1 & empty/small buffer boundaries", () => {
        expect_eq(isLoopback("::1"), true);
        expect_eq(isLinkLocal("fe80::1"), true);
        expect_eq(isLinkLocal("fe80::1%eth0"), true);
        expect_eq(isUniqueLocal("fc00::1"), true);
        expect_eq(isUniqueLocal("fd12:3456:789a::1"), true);
        expect_eq(isTrustedBuiltin("2001:db8::9", TRUST_ALL_PRIVATE), false);
        expect_eq(isTrustedBuiltin("fc00::1", TRUST_ALL_PRIVATE), true);

        let out: [64]u8 = undefined;
        let n = resolveClientIp("::1", "2001:db8::9, fc00::1, fe80::1", TRUST_ALL_PRIVATE, 0, out[0..64]);
        expect_eq(sliceEq(out[0..n], "2001:db8::9"), true);

        // Empty remote_addr & tiny output buffer safety
        expect_eq(resolveClientIp("", "1.2.3.4", TRUST_ALL_PRIVATE, 0, out[0..64]), 0 as usize);
        let tiny: [4]u8 = undefined;
        expect_eq(resolveClientIp("203.0.113.5", "", TRUST_NONE, 0, tiny[0..4]), 0 as usize);
    });
}
