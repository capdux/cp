// cap/test/testCapWhatwgUrl.cp
// Comprehensive unit tests for @cap/whatwg-url (WHATWG URL Standard)

import { test, expect_eq } from "cap:test";
import {
    WhatwgUrl,
    defaultPortForScheme
} from "../../packages/@cap/whatwg-url/src/index.cp";

fn wuTestSliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/whatwg-url — Full parse with default port omission & dot-segment path normalization", () => {
        let u = WhatwgUrl.parse("HTTPS://user:pass@EXAMPLE.COM:443/a/b/../c/./d?k=1#top");
        expect_eq(u.ok, true);
        expect_eq(wuTestSliceEq(u.schemeStr(), "https"), true);
        expect_eq(wuTestSliceEq(u.usernameStr(), "user"), true);
        expect_eq(wuTestSliceEq(u.passwordStr(), "pass"), true);
        expect_eq(wuTestSliceEq(u.hostStr(), "example.com"), true);
        expect_eq(u.port, -1 as i32);
        expect_eq(u.effectivePort(), 443 as i32);
        expect_eq(wuTestSliceEq(u.pathStr(), "/a/c/d"), true);
        expect_eq(u.has_query, true);
        expect_eq(wuTestSliceEq(u.queryStr(), "k=1"), true);
        expect_eq(u.has_fragment, true);
        expect_eq(wuTestSliceEq(u.fragmentStr(), "top"), true);

        let buf: [256]u8 = undefined;
        let n = u.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n], "https://user:pass@example.com/a/c/d?k=1#top"), true);

        let orig: [128]u8 = undefined;
        let on = u.origin(orig[0..128]);
        expect_eq(wuTestSliceEq(orig[0..on], "https://example.com"), true);
    });

    test("2. @cap/whatwg-url — IDNA host normalization via @cap/idna-uts46 & UTF-8 path percent-encoding", () => {
        let u = WhatwgUrl.parse("https://münchen.de/路径");
        expect_eq(u.ok, true);
        expect_eq(wuTestSliceEq(u.hostStr(), "xn--mnchen-3ya.de"), true);
        expect_eq(wuTestSliceEq(u.pathStr(), "/%E8%B7%AF%E5%BE%84"), true);

        let buf: [256]u8 = undefined;
        let n = u.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n], "https://xn--mnchen-3ya.de/%E8%B7%AF%E5%BE%84"), true);
    });

    test("3. @cap/whatwg-url — IPv6 host http://[::1]:8080/api & origin()", () => {
        let u = WhatwgUrl.parse("http://[::1]:8080/api");
        expect_eq(u.ok, true);
        expect_eq(wuTestSliceEq(u.schemeStr(), "http"), true);
        expect_eq(wuTestSliceEq(u.hostStr(), "[::1]"), true);
        expect_eq(u.port, 8080 as i32);
        expect_eq(wuTestSliceEq(u.pathStr(), "/api"), true);

        let orig: [128]u8 = undefined;
        let on = u.origin(orig[0..128]);
        expect_eq(wuTestSliceEq(orig[0..on], "http://[::1]:8080"), true);

        let buf: [256]u8 = undefined;
        let n = u.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n], "http://[::1]:8080/api"), true);
    });

    test("4. @cap/whatwg-url — Relative URL join() covering //cdn/x, /root, ../up, ?q=2, #h, absolute", () => {
        let base = WhatwgUrl.parse("https://example.com/a/b/index.html?old=1#old");
        expect_eq(base.ok, true);
        let buf: [256]u8 = undefined;

        // Protocol-relative
        let j1 = WhatwgUrl.join(base, "//cdn.example.net/x");
        expect_eq(j1.ok, true);
        let n1 = j1.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n1], "https://cdn.example.net/x"), true);

        // Root-relative
        let j2 = WhatwgUrl.join(base, "/root");
        expect_eq(j2.ok, true);
        let n2 = j2.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n2], "https://example.com/root"), true);

        // Relative dot-dot path
        let j3 = WhatwgUrl.join(base, "../up");
        expect_eq(j3.ok, true);
        let n3 = j3.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n3], "https://example.com/a/up"), true);

        // Query-only
        let j4 = WhatwgUrl.join(base, "?q=2");
        expect_eq(j4.ok, true);
        let n4 = j4.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n4], "https://example.com/a/b/index.html?q=2"), true);

        // Fragment-only (preserves base query)
        let j5 = WhatwgUrl.join(base, "#h");
        expect_eq(j5.ok, true);
        let n5 = j5.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n5], "https://example.com/a/b/index.html?old=1#h"), true);

        // Absolute URL overrides base
        let j6 = WhatwgUrl.join(base, "http://other.test:8080/z");
        expect_eq(j6.ok, true);
        let n6 = j6.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n6], "http://other.test:8080/z"), true);
    });

    test("5. @cap/whatwg-url — Non-default port retention & ws/wss/ftp default ports", () => {
        expect_eq(defaultPortForScheme("http"), 80 as i32);
        expect_eq(defaultPortForScheme("https"), 443 as i32);
        expect_eq(defaultPortForScheme("ws"), 80 as i32);
        expect_eq(defaultPortForScheme("wss"), 443 as i32);
        expect_eq(defaultPortForScheme("ftp"), 21 as i32);
        expect_eq(defaultPortForScheme("custom"), -1 as i32);

        let buf: [256]u8 = undefined;

        let ws_def = WhatwgUrl.parse("ws://sock.example.com:80/chat");
        expect_eq(ws_def.ok, true);
        expect_eq(ws_def.port, -1 as i32);
        let n1 = ws_def.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n1], "ws://sock.example.com/chat"), true);

        let ws_custom = WhatwgUrl.parse("ws://sock.example.com:9000/chat");
        expect_eq(ws_custom.ok, true);
        expect_eq(ws_custom.port, 9000 as i32);
        let n2 = ws_custom.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n2], "ws://sock.example.com:9000/chat"), true);

        let ftp_def = WhatwgUrl.parse("ftp://files.example.com:21/pub");
        expect_eq(ftp_def.ok, true);
        expect_eq(ftp_def.port, -1 as i32);
        let n3 = ftp_def.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n3], "ftp://files.example.com/pub"), true);

        let ftp_custom = WhatwgUrl.parse("ftp://files.example.com:2121/pub");
        expect_eq(ftp_custom.ok, true);
        expect_eq(ftp_custom.port, 2121 as i32);
        let n4 = ftp_custom.toStr(buf[0..256]);
        expect_eq(wuTestSliceEq(buf[0..n4], "ftp://files.example.com:2121/pub"), true);
    });

    test("6. @cap/whatwg-url — Rejects invalid URLs (empty, missing scheme, port overflow 99999, illegal host)", () => {
        expect_eq(WhatwgUrl.parse("").ok, false);
        expect_eq(WhatwgUrl.parse("   ").ok, false);
        expect_eq(WhatwgUrl.parse("example.com/no-scheme").ok, false);
        expect_eq(WhatwgUrl.parse("http://example.com:99999/").ok, false);
        expect_eq(WhatwgUrl.parse("http://example.com:abc/").ok, false);
        expect_eq(WhatwgUrl.parse("http:///missing-host").ok, false);
        expect_eq(WhatwgUrl.parse("http://bad host.com/").ok, false);
        expect_eq(WhatwgUrl.parse("http://[::1/unclosed").ok, false);
        expect_eq(WhatwgUrl.parse("http://bad..host.com/").ok, false);
    });
}
