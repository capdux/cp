// cap/test/testCapForwarded.cp
// Comprehensive unit tests for @cap/forwarded

import { test, expect_eq } from "cap:test";
import {
    parseForwarded,
    parseXForwardedFor,
    extractClientIp,
    isPrivateIp,
} from "../../packages/@cap/forwarded/src/index.cp";

fn main() {
    test("1. @cap/forwarded — RFC 7239 single & multi-hop parsing", () => {
        const hdr = "for=192.0.2.60;proto=https;by=203.0.113.43;host=api.example.com, For=198.51.100.17:8080;Proto=http;By=10.0.0.1";
        const chain = parseForwarded(hdr);

        expect_eq(chain.count, 2 as usize);
        expect_eq(chain.clientIp(), "192.0.2.60");
        expect_eq(chain.clientProto(), "https");
        expect_eq(chain.clientHost(), "api.example.com");

        const h0 = chain.hopAt(0 as usize);
        expect_eq(h0.for_addr, "192.0.2.60");
        expect_eq(h0.by_addr, "203.0.113.43");

        const h1 = chain.hopAt(1 as usize);
        expect_eq(h1.for_addr, "198.51.100.17");
        expect_eq(h1.proto, "http");
        expect_eq(h1.by_addr, "10.0.0.1");
    });

    test("2. @cap/forwarded — IPv6 brackets, quotes & port stripping", () => {
        const hdr = "for=\"[2001:db8:cafe::17]:4711\";host=\"example.com:8443\";proto=https, for=\"[::1]\"";
        const chain = parseForwarded(hdr);

        expect_eq(chain.count, 2 as usize);
        expect_eq(chain.hopAt(0 as usize).for_addr, "2001:db8:cafe::17");
        expect_eq(chain.hopAt(0 as usize).host, "example.com:8443");
        expect_eq(chain.hopAt(0 as usize).proto, "https");
        expect_eq(chain.hopAt(1 as usize).for_addr, "::1");
    });

    test("3. @cap/forwarded — X-Forwarded-For chain parsing with remote_addr", () => {
        const chain = parseXForwardedFor("203.0.113.195, 70.41.3.18:54321, [2001:db8::2]:443", "10.0.0.5");

        expect_eq(chain.count, 4 as usize);
        expect_eq(chain.clientIp(), "203.0.113.195");
        expect_eq(chain.hopAt(1 as usize).for_addr, "70.41.3.18");
        expect_eq(chain.hopAt(2 as usize).for_addr, "2001:db8::2");
        expect_eq(chain.hopAt(3 as usize).for_addr, "10.0.0.5");
    });

    test("4. @cap/forwarded — anti-spoofing extractClientIp & ipAtTrustDepth", () => {
        // Spoofed "1.2.3.4" prepended by client; real client is "203.0.113.50";
        // two trusted proxies: "10.0.0.1" (edge) and "10.0.0.2" (remote_addr).
        const xff = "1.2.3.4, 203.0.113.50, 10.0.0.1";
        const remote = "10.0.0.2";

        // 0 trusted proxies -> direct peer only
        expect_eq(extractClientIp(xff, remote, 0 as usize), "10.0.0.2");
        // 1 trusted proxy -> hop before remote
        expect_eq(extractClientIp(xff, remote, 1 as usize), "10.0.0.1");
        // 2 trusted proxies -> real client IP, ignoring spoofed 1.2.3.4
        expect_eq(extractClientIp(xff, remote, 2 as usize), "203.0.113.50");
        // Excess trusted_hops clamps safely to leftmost entry
        expect_eq(extractClientIp(xff, remote, 10 as usize), "1.2.3.4");
    });

    test("5. @cap/forwarded — isPrivateIp for IPv4 & IPv6 ranges", () => {
        // Private / loopback IPv4
        expect_eq(isPrivateIp("10.0.0.1"), true);
        expect_eq(isPrivateIp("10.255.255.254"), true);
        expect_eq(isPrivateIp("172.16.0.1"), true);
        expect_eq(isPrivateIp("172.31.255.255"), true);
        expect_eq(isPrivateIp("192.168.1.100"), true);
        expect_eq(isPrivateIp("127.0.0.1"), true);
        expect_eq(isPrivateIp("127.0.0.1:8080"), true);

        // Public IPv4 (including boundaries around 172.16/12)
        expect_eq(isPrivateIp("172.15.255.255"), false);
        expect_eq(isPrivateIp("172.32.0.1"), false);
        expect_eq(isPrivateIp("8.8.8.8"), false);
        expect_eq(isPrivateIp("203.0.113.50"), false);

        // Private / loopback / link-local IPv6
        expect_eq(isPrivateIp("::1"), true);
        expect_eq(isPrivateIp("[::1]:443"), true);
        expect_eq(isPrivateIp("fc00::1"), true);
        expect_eq(isPrivateIp("fd12:3456:789a::1"), true);
        expect_eq(isPrivateIp("fe80::1"), true);
        expect_eq(isPrivateIp("febf::abcd"), true);
        expect_eq(isPrivateIp("::ffff:10.0.0.9"), true);

        // Public IPv6 & invalid strings
        expect_eq(isPrivateIp("2001:db8:cafe::17"), false);
        expect_eq(isPrivateIp("2606:4700:4700::1111"), false);
        expect_eq(isPrivateIp("::ffff:8.8.8.8"), false);
        expect_eq(isPrivateIp("not-an-ip"), false);
        expect_eq(isPrivateIp(""), false);
    });

    test("6. @cap/forwarded — empty & malformed headers handled safely", () => {
        const empty = parseForwarded("");
        expect_eq(empty.count, 0 as usize);
        expect_eq(empty.clientIp(), "");
        expect_eq(empty.clientProto(), "");
        expect_eq(empty.clientHost(), "");
        expect_eq(empty.ipAtTrustDepth(1 as usize), "");
        expect_eq(empty.hopAt(99 as usize).for_addr, "");

        const malformed = parseForwarded(";;;,,, garbage, for=203.0.113.9;unknown=1");
        expect_eq(malformed.count, 1 as usize);
        expect_eq(malformed.clientIp(), "203.0.113.9");

        // Empty XFF falls back to remote_addr
        expect_eq(extractClientIp("", "198.51.100.7", 1 as usize), "198.51.100.7");
    });
}
