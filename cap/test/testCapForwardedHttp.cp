import { test, expect_eq } from "cap:test";
import {
    ForwardedEntry,
    parseXForwardedFor,
    extractClientIp,
    parseForwardedHeader,
    isLoopbackIp,
} from "../../packages/@cap/forwarded-http/src/index.cp";

fn strEq(a: string, b: string): bool {
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
    test("1. @cap/forwarded-http — parseXForwardedFor returns [remote, ..., client] order", () => {
        let addrs: [8]string = undefined;
        const n = parseXForwardedFor("127.0.0.1", "203.0.113.195, 70.41.3.18, 150.172.238.178", &addrs);
        expect_eq(n, 4);
        expect_eq(strEq(addrs[0], "127.0.0.1"), true);
        expect_eq(strEq(addrs[1], "150.172.238.178"), true);
        expect_eq(strEq(addrs[2], "70.41.3.18"), true);
        expect_eq(strEq(addrs[3], "203.0.113.195"), true);

        // Handles empty segments & small buffer limit
        let small: [2]string = undefined;
        const n2 = parseXForwardedFor("10.0.0.1", " 1.2.3.4 , , 5.6.7.8 ", &small);
        expect_eq(n2, 2);
        expect_eq(strEq(small[0], "10.0.0.1"), true);
        expect_eq(strEq(small[1], "5.6.7.8"), true);
    });

    test("2. @cap/forwarded-http — extractClientIp respects trust_hops & prevents spoofing", () => {
        const xff = "1.1.1.1, 203.0.113.5, 10.0.0.2";
        // trust_hops == 0: never trust XFF
        expect_eq(strEq(extractClientIp("10.0.0.1", xff, 0), "10.0.0.1"), true);
        // trust_hops == 1: trust 10.0.0.1 -> client is 10.0.0.2
        expect_eq(strEq(extractClientIp("10.0.0.1", xff, 1), "10.0.0.2"), true);
        // trust_hops == 2: trust 10.0.0.1 & 10.0.0.2 -> client is 203.0.113.5 (ignores spoofed 1.1.1.1)
        expect_eq(strEq(extractClientIp("10.0.0.1", xff, 2), "203.0.113.5"), true);
        // trust_hops > chain length -> clamps to leftmost entry
        expect_eq(strEq(extractClientIp("10.0.0.1", xff, 10), "1.1.1.1"), true);
        // Empty XFF falls back to remote_addr
        expect_eq(strEq(extractClientIp("192.0.2.1", "", 2), "192.0.2.1"), true);
    });

    test("3. @cap/forwarded-http — parseForwardedHeader RFC 7239 multi-hop, quotes & IPv6 brackets", () => {
        const hdr = "for=192.0.2.60;proto=http;by=203.0.113.43;host=example.com, FOR=\"[2001:db8:cafe::17]:4711\";PROTO=https";
        let entries: [4]ForwardedEntry = undefined;
        const n = parseForwardedHeader(hdr, &entries);
        expect_eq(n, 2);

        expect_eq(strEq(entries[0].for_val, "192.0.2.60"), true);
        expect_eq(strEq(entries[0].proto, "http"), true);
        expect_eq(strEq(entries[0].by_val, "203.0.113.43"), true);
        expect_eq(strEq(entries[0].host, "example.com"), true);

        expect_eq(strEq(entries[1].for_val, "[2001:db8:cafe::17]:4711"), true);
        expect_eq(strEq(entries[1].proto, "https"), true);
        expect_eq(entries[1].by_val.len, 0);
        expect_eq(entries[1].host.len, 0);
    });

    test("4. @cap/forwarded-http — parseForwardedHeader handles quoted commas/semicolons & unknown params", () => {
        const hdr = "By=\"_hidden,1\";Host=\"api.cap.dev:8443\";ext=ignored;For=198.51.100.17";
        let entries: [4]ForwardedEntry = undefined;
        const n = parseForwardedHeader(hdr, &entries);
        expect_eq(n, 1);
        expect_eq(strEq(entries[0].by_val, "_hidden,1"), true);
        expect_eq(strEq(entries[0].host, "api.cap.dev:8443"), true);
        expect_eq(strEq(entries[0].for_val, "198.51.100.17"), true);
        expect_eq(entries[0].proto.len, 0);
    });

    test("5. @cap/forwarded-http — isLoopbackIp detects IPv4 127.0.0.0/8, IPv6 ::1 & IPv4-mapped", () => {
        expect_eq(isLoopbackIp("127.0.0.1"), true);
        expect_eq(isLoopbackIp("127.0.1.42"), true);
        expect_eq(isLoopbackIp("127.255.255.254"), true);
        expect_eq(isLoopbackIp("127.0.0.1:8080"), true);
        expect_eq(isLoopbackIp("::1"), true);
        expect_eq(isLoopbackIp("[::1]:3000"), true);
        expect_eq(isLoopbackIp("0:0:0:0:0:0:0:1"), true);
        expect_eq(isLoopbackIp("::ffff:127.0.0.1"), true);
        expect_eq(isLoopbackIp("::FFFF:127.1.2.3"), true);
    });

    test("6. @cap/forwarded-http — isLoopbackIp rejects public, private & malformed IPs", () => {
        expect_eq(isLoopbackIp("192.168.1.1"), false);
        expect_eq(isLoopbackIp("10.0.0.1"), false);
        expect_eq(isLoopbackIp("128.0.0.1"), false);
        expect_eq(isLoopbackIp("127.0.0.256"), false);
        expect_eq(isLoopbackIp("127.0.0"), false);
        expect_eq(isLoopbackIp("::ffff:192.168.1.1"), false);
        expect_eq(isLoopbackIp("2001:db8::1"), false);
        expect_eq(isLoopbackIp(""), false);
    });
}
