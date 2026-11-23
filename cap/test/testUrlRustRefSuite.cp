// Reference-hardened test suite against servo/rust-url (MIT/Apache-2.0) & WHATWG URL Standard
// Covers:
//   1. Basic WHATWG URL parsing, credentials, IPv6 hosts, default port normalization
//   2. Hostname, IPv4, IPv6 & port strict security boundary checks
//   3. Path dot-segment (., .., %2e, %2e%2e) & special-scheme backslash normalization
//   4. Strict percent-encoding validation & decoding (rejecting truncated % and %ZZ)
//   5. cap:simd delimiter & percent scanner 0-diff parity against scalar reference
//   6. Deterministic PRNG Fuzz (>= 1,500 malformed URL / truncated % / control-char / long-path cases)

import { test, expect_eq } from "cap:test";
import {
    URL,
    URL_MAX_LEN,
    isSpecialScheme,
    getDefaultPortForScheme,
    normalizeDefaultPort,
    isValidPortStrict,
    isValidIPv4Strict,
    isValidIPv6Strict,
    isValidHostStrict,
    normalizeUrlPath,
    isValidUrlStrict,
    simdFindUrlDelimiter,
    simdFindUrlDelimiterScalar,
} from "../../lib/url/url";
import {
    PercentEncoding,
    percentDecodeStrict,
    isValidPercentEncoding,
} from "../../lib/url/percent";
const std = @import("std");

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn optUsizeEq(a: usize?, b: usize?): bool {
    if (va := a) {
        if (vb := b) {
            return va == vb;
        }
        return false;
    }
    return b == null;
}

fn main() {
    test("1. WHATWG URL basic parsing, credentials, IPv6 & default ports", () => {
        // Full URL with credentials, port, path, query, hash
        let u1 = URL.parse(
            "https://alice:s3cr3t@api.example.com:8443/v1/items?a=1&b=2#top",
            null
        ) catch unreachable;
        expect_eq(u1.protocol, "https:");
        expect_eq(u1.scheme, "https");
        expect_eq(u1.username, "alice");
        expect_eq(u1.password, "s3cr3t");
        expect_eq(u1.hostname, "api.example.com");
        expect_eq(u1.port, "8443");
        expect_eq(u1.pathname, "/v1/items");
        expect_eq(u1.search, "?a=1&b=2");
        expect_eq(u1.hash, "#top");
        expect_eq(u1.getPortAuto(), 8443 as u16);

        // IPv6 host with & without port
        let u_v6 = URL.parse("http://[2001:db8::1]:8080/status", null) catch unreachable;
        expect_eq(u_v6.hostname, "[2001:db8::1]");
        expect_eq(u_v6.port, "8080");
        expect_eq(u_v6.pathname, "/status");

        let u_v6_loop = URL.parse("wss://[::1]/ws", null) catch unreachable;
        expect_eq(u_v6_loop.hostname, "[::1]");
        expect_eq(u_v6_loop.port, "");
        expect_eq(u_v6_loop.isWSS(), true);
        expect_eq(u_v6_loop.getDefaultPort(), 443 as u16);
        expect_eq(u_v6_loop.getPortAuto(), 443 as u16);

        // Special scheme default ports
        expect_eq(isSpecialScheme("http"), true);
        expect_eq(isSpecialScheme("HTTPS:"), true);
        expect_eq(isSpecialScheme("ws"), true);
        expect_eq(isSpecialScheme("wss"), true);
        expect_eq(isSpecialScheme("ftp"), true);
        expect_eq(isSpecialScheme("file"), true);
        expect_eq(isSpecialScheme("custom"), false);

        expect_eq(getDefaultPortForScheme("http") orelse 0, 80 as u16);
        expect_eq(getDefaultPortForScheme("https") orelse 0, 443 as u16);
        expect_eq(getDefaultPortForScheme("ws") orelse 0, 80 as u16);
        expect_eq(getDefaultPortForScheme("wss") orelse 0, 443 as u16);
        expect_eq(getDefaultPortForScheme("ftp") orelse 0, 21 as u16);
        expect_eq(getDefaultPortForScheme("custom") == null, true);

        expect_eq(normalizeDefaultPort("http", "80"), "");
        expect_eq(normalizeDefaultPort("https", "443"), "");
        expect_eq(normalizeDefaultPort("ws", "80"), "");
        expect_eq(normalizeDefaultPort("wss", "443"), "");
        expect_eq(normalizeDefaultPort("ftp", "21"), "");
        expect_eq(normalizeDefaultPort("https", "8443"), "8443");
        expect_eq(normalizeDefaultPort("http", "443"), "443");

        // Last '@' wins in authority to prevent host-confusion
        let u_at = URL.parse("https://user@first@real.example.com/p", null) catch unreachable;
        expect_eq(u_at.username, "user@first");
        expect_eq(u_at.hostname, "real.example.com");
    });

    test("2. Strict IPv4, IPv6, hostname & port security validation (rust-url parity)", () => {
        // IPv4 valid
        expect_eq(isValidIPv4Strict("0.0.0.0"), true);
        expect_eq(isValidIPv4Strict("127.0.0.1"), true);
        expect_eq(isValidIPv4Strict("255.255.255.255"), true);
        // IPv4 invalid (overflow, leading zero, wrong octet count)
        expect_eq(isValidIPv4Strict("256.0.0.1"), false);
        expect_eq(isValidIPv4Strict("127.0.0.01"), false);
        expect_eq(isValidIPv4Strict("127.0.1"), false);
        expect_eq(isValidIPv4Strict("127.0.0.1."), false);
        expect_eq(isValidIPv4Strict("1.2.3.4.5"), false);

        // IPv6 valid
        expect_eq(isValidIPv6Strict("[::]"), true);
        expect_eq(isValidIPv6Strict("[::1]"), true);
        expect_eq(isValidIPv6Strict("[2001:db8::1]"), true);
        expect_eq(isValidIPv6Strict("[fe80::1:2:3:4]"), true);
        expect_eq(isValidIPv6Strict("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]"), true);
        expect_eq(isValidIPv6Strict("[::ffff:192.0.2.128]"), true);
        // IPv6 invalid (unclosed bracket, double ::, too many groups, bad hex, bad embedded IPv4)
        expect_eq(isValidIPv6Strict("[::1"), false);
        expect_eq(isValidIPv6Strict("::1]"), false);
        expect_eq(isValidIPv6Strict("[]"), false);
        expect_eq(isValidIPv6Strict("[2001::db8::1]"), false);
        expect_eq(isValidIPv6Strict("[1:2:3:4:5:6:7:8:9]"), false);
        expect_eq(isValidIPv6Strict("[1:2:3:4:5:6:7]"), false);
        expect_eq(isValidIPv6Strict("[2001:db8::g1]"), false);
        expect_eq(isValidIPv6Strict("[::ffff:999.0.0.1]"), false);
        expect_eq(isValidIPv6Strict("[::ffff:01.2.3.4]"), false);
        expect_eq(isValidIPv6Strict("[2001:db8:]"), false);

        // Hostname strict checks (forbidden chars & confusion guards)
        expect_eq(isValidHostStrict("example.com"), true);
        expect_eq(isValidHostStrict("sub-1.api.example.io"), true);
        expect_eq(isValidHostStrict("[2001:db8::1]"), true);
        expect_eq(isValidHostStrict("192.168.0.1"), true);
        expect_eq(isValidHostStrict(""), false);
        expect_eq(isValidHostStrict("evil.com#good.com"), false);
        expect_eq(isValidHostStrict("evil.com/good.com"), false);
        expect_eq(isValidHostStrict("evil.com?good.com"), false);
        expect_eq(isValidHostStrict("evil.com@good.com"), false);
        expect_eq(isValidHostStrict("evil.com\\good.com"), false);
        expect_eq(isValidHostStrict("bad host.com"), false);
        expect_eq(isValidHostStrict("bad\thost.com"), false);
        expect_eq(isValidHostStrict("bad\r\nhost.com"), false);
        expect_eq(isValidHostStrict("evil%2fhost.com"), false);
        expect_eq(isValidHostStrict("evil%40host.com"), false);
        expect_eq(isValidHostStrict("999.999.999.999"), false);
        expect_eq(isValidHostStrict(".leadingdot.com"), false);
        expect_eq(isValidHostStrict("double..dot.com"), false);

        // Port bounds
        expect_eq(isValidPortStrict(""), true);
        expect_eq(isValidPortStrict("0"), true);
        expect_eq(isValidPortStrict("80"), true);
        expect_eq(isValidPortStrict("65535"), true);
        expect_eq(isValidPortStrict("65536"), false);
        expect_eq(isValidPortStrict("99999"), false);
        expect_eq(isValidPortStrict("100000"), false);
        expect_eq(isValidPortStrict("80a"), false);
        expect_eq(isValidPortStrict("-1"), false);
    });

    test("3. Path dot-segment, %2e%2e traversal & special-scheme backslash normalization", () => {
        let out: [256]u8 = undefined;

        expect_eq(normalizeUrlPath("/a/b/../c", true, &out) orelse "", "/a/c");
        expect_eq(normalizeUrlPath("/a/b/./c", true, &out) orelse "", "/a/b/c");
        expect_eq(normalizeUrlPath("/a/b/.", true, &out) orelse "", "/a/b/");
        expect_eq(normalizeUrlPath("/a/b/..", true, &out) orelse "", "/a/");
        expect_eq(normalizeUrlPath("/../../etc/passwd", true, &out) orelse "", "/etc/passwd");
        expect_eq(normalizeUrlPath("/a/%2e%2e/secret", true, &out) orelse "", "/secret");
        expect_eq(normalizeUrlPath("/a/%2E%2e/secret", true, &out) orelse "", "/secret");
        expect_eq(normalizeUrlPath("/a/.%2e/secret", true, &out) orelse "", "/secret");
        expect_eq(normalizeUrlPath("/a/%2e./secret", true, &out) orelse "", "/secret");
        expect_eq(normalizeUrlPath("/a/%2e/b", true, &out) orelse "", "/a/b");
        expect_eq(normalizeUrlPath("/a/%2E/b", true, &out) orelse "", "/a/b");

        // Special scheme backslash normalization vs non-special preservation
        expect_eq(normalizeUrlPath("\\a\\b\\..\\c\\d", true, &out) orelse "", "/a/c/d");
        expect_eq(normalizeUrlPath("/a\\b/../c", true, &out) orelse "", "/a/c");
        expect_eq(normalizeUrlPath("/a\\b", false, &out) orelse "", "/a\\b");

        // Small output buffer rejected cleanly (no silent truncation)
        let tiny: [3]u8 = undefined;
        expect_eq(normalizeUrlPath("/abcdef", true, &tiny) == null, true);
    });

    test("4. Strict URL validation & strict percent-decoding", () => {
        // Valid URLs
        expect_eq(isValidUrlStrict("https://example.com/a/b?x=1#y"), true);
        expect_eq(isValidUrlStrict("http://[::1]:8080/health"), true);
        expect_eq(isValidUrlStrict("https://user:pass@sub.example.com:443/path%20ok"), true);
        expect_eq(isValidUrlStrict("file:///etc/hosts"), true);
        expect_eq(isValidUrlStrict("ftp://files.example.com:21/pub/readme.txt"), true);
        expect_eq(isValidUrlStrict("mailto:alice@example.com"), true);

        // Rejected malformed / boundary URLs
        expect_eq(isValidUrlStrict(""), false);
        expect_eq(isValidUrlStrict("http://example.com:65536/"), false);
        expect_eq(isValidUrlStrict("http://example.com:99999/"), false);
        expect_eq(isValidUrlStrict("http://example.com:abc/"), false);
        expect_eq(isValidUrlStrict("http://[::1/path"), false);
        expect_eq(isValidUrlStrict("http://[2001::db8::1]/"), false);
        expect_eq(isValidUrlStrict("http://example.com/bad\x00null"), false);
        expect_eq(isValidUrlStrict("http://example.com/bad\r\nheader"), false);
        expect_eq(isValidUrlStrict("http://example.com/bad space"), false);
        expect_eq(isValidUrlStrict("http://example.com/bad%ZZ"), false);
        expect_eq(isValidUrlStrict("http://example.com/trunc%2"), false);
        expect_eq(isValidUrlStrict("http://example.com/trunc%"), false);
        expect_eq(isValidUrlStrict("http:///missing-host"), false);
        expect_eq(isValidUrlStrict("123invalid://example.com"), false);
        expect_eq(isValidUrlStrict("http://a:b:c/"), false);

        // Strict percent decode
        let dec_buf: [128]u8 = undefined;
        expect_eq(percentDecodeStrict("hello%20world%21", &dec_buf) orelse "", "hello world!");
        expect_eq(percentDecodeStrict("plain-ascii-longer-than-16-bytes-for-simd-path", &dec_buf) orelse "", "plain-ascii-longer-than-16-bytes-for-simd-path");
        expect_eq(percentDecodeStrict("a%2Fb%3Fc%3Dd%26e%3D100%25", &dec_buf) orelse "", "a/b?c=d&e=100%");
        expect_eq(percentDecodeStrict("broken%", &dec_buf) == null, true);
        expect_eq(percentDecodeStrict("broken%2", &dec_buf) == null, true);
        expect_eq(percentDecodeStrict("broken%ZZ", &dec_buf) == null, true);
        expect_eq(percentDecodeStrict("broken%2G", &dec_buf) == null, true);
        expect_eq(isValidPercentEncoding("ok%20%7E%FF"), true);
        expect_eq(isValidPercentEncoding("bad%1"), false);
        expect_eq(isValidPercentEncoding("bad%GG"), false);

        // URL_MAX_LEN constant check
        expect_eq(URL_MAX_LEN, 65536 as usize);
    });

    test("5. cap:simd delimiter & percent scanner 0-diff parity vs scalar", () => {
        let buf: [96]u8 = undefined;
        let len: usize = 0;
        while (len <= 96) {
            // Fill with non-delimiter bytes
            let j: usize = 0;
            while (j < len) {
                buf[j] = 97 as u8; // 'a'
                j += 1;
            }
            expect_eq(optUsizeEq(simdFindUrlDelimiter(buf[0..len]), simdFindUrlDelimiterScalar(buf[0..len])), true);
            expect_eq(optUsizeEq(PercentEncoding.findNextPercent(buf[0..len]), PercentEncoding.findNextPercentScalar(buf[0..len])), true);

            // Place delimiter / '%' at every position around 16/32-byte SIMD boundaries
            if (len > 0) {
                const pos = len - 1;
                buf[pos] = 47 as u8; // '/'
                expect_eq(optUsizeEq(simdFindUrlDelimiter(buf[0..len]), simdFindUrlDelimiterScalar(buf[0..len])), true);
                buf[pos] = 63 as u8; // '?'
                expect_eq(optUsizeEq(simdFindUrlDelimiter(buf[0..len]), simdFindUrlDelimiterScalar(buf[0..len])), true);
                buf[pos] = 35 as u8; // '#'
                expect_eq(optUsizeEq(simdFindUrlDelimiter(buf[0..len]), simdFindUrlDelimiterScalar(buf[0..len])), true);
                buf[pos] = 58 as u8; // ':'
                expect_eq(optUsizeEq(simdFindUrlDelimiter(buf[0..len]), simdFindUrlDelimiterScalar(buf[0..len])), true);
                buf[pos] = 37 as u8; // '%'
                expect_eq(optUsizeEq(PercentEncoding.findNextPercent(buf[0..len]), PercentEncoding.findNextPercentScalar(buf[0..len])), true);
            }
            len += 1;
        }
    });

    test("6. Deterministic PRNG Fuzz (>= 1,500 malformed URL / % / path / control-char cases)", () => {
        const alphabet: []const u8 = "https://[::1]:8080/a/b/../%2e%2e/?q=%ZZ%2#frag\\\x00\r\n\t @<>|[]^0123456789abcdefABCDEF.:";
        let seed: u64 = 0x5EED_CAFE_2026_0032;
        let fuzz_buf: [160]u8 = undefined;
        let norm_out: [256]u8 = undefined;
        let dec_out: [256]u8 = undefined;

        let iter: usize = 0;
        let total_valid: usize = 0;
        let total_invalid: usize = 0;
        while (iter < 1500) {
            const slen: usize = ((nextRng(&seed) % 150) + 1) as usize;
            let k: usize = 0;
            while (k < slen) {
                const idx: usize = (nextRng(&seed) % (alphabet.len as u64)) as usize;
                fuzz_buf[k] = alphabet[idx];
                k += 1;
            }
            const sample = fuzz_buf[0..slen];

            // Must never panic, OOB, or infinite-loop
            if (isValidUrlStrict(sample)) {
                total_valid += 1;
            } else {
                total_invalid += 1;
            }

            const parsed = URL.parseSlice(sample);
            * = parsed.getPortAuto();
            * = normalizeUrlPath(sample, (iter % 2) == 0, &norm_out);
            * = percentDecodeStrict(sample, &dec_out);

            // SIMD vs scalar differential check on every fuzz sample
            expect_eq(optUsizeEq(simdFindUrlDelimiter(sample), simdFindUrlDelimiterScalar(sample)), true);
            expect_eq(optUsizeEq(PercentEncoding.findNextPercent(sample), PercentEncoding.findNextPercentScalar(sample)), true);

            iter += 1;
        }
        expect_eq(total_valid + total_invalid, 1500 as usize);
        expect_eq(total_invalid > 1000, true);
    });
}
