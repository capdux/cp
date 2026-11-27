// cap/test/testCapFastUri.cp
// Comprehensive unit tests for @cap/fast-uri

import { test, expect_eq } from "cap:test";
import {
    UriComponents,
    parseUri,
    serializeUri,
    resolveUri,
    equalUri,
} from "../../packages/fast-uri/src/index.cp";

fn main() {
    test("1. @cap/fast-uri — parse full HTTP URI with userinfo, port, query, fragment", () => {
        const u = parseUri("HTTPS://user:pass@Example.COM:8443/a/b?k=1&v=2#sec");
        expect_eq(u.valid, true);
        expect_eq(u.scheme, "HTTPS");
        expect_eq(u.userinfo, "user:pass");
        expect_eq(u.host, "Example.COM");
        expect_eq(u.port, 8443);
        expect_eq(u.path, "/a/b");
        expect_eq(u.has_query, true);
        expect_eq(u.query, "k=1&v=2");
        expect_eq(u.has_fragment, true);
        expect_eq(u.fragment, "sec");
    });

    test("2. @cap/fast-uri — IPv6 literals, relative paths, and validation guards", () => {
        const v6 = parseUri("http://[::1]:8080/status");
        expect_eq(v6.valid, true);
        expect_eq(v6.host, "[::1]");
        expect_eq(v6.port, 8080);
        expect_eq(v6.path, "/status");

        const rel = parseUri("../dir/file.txt?q=1");
        expect_eq(rel.valid, true);
        expect_eq(rel.scheme, "");
        expect_eq(rel.host, "");
        expect_eq(rel.port, -1);
        expect_eq(rel.path, "../dir/file.txt");
        expect_eq(rel.query, "q=1");

        // Invalid port / unclosed IPv6 / space
        expect_eq(parseUri("http://example.com:99999/").valid, false);
        expect_eq(parseUri("http://[::1/").valid, false);
        expect_eq(parseUri("http://bad host/").valid, false);
    });

    test("3. @cap/fast-uri — serializeUri lowercases scheme & host per RFC 3986 §5.3", () => {
        let out: [256]u8 = undefined;
        const u = parseUri("HTTP://User@ExAmPlE.Org:8080/p/q?a=1#h");
        const n = serializeUri(u, out[0..]);
        expect_eq(out[0..n], "http://User@example.org:8080/p/q?a=1#h");

        // Manual UriComponents with bare IPv6 host auto-brackets
        let c = UriComponents.empty();
        c.scheme = "HTTP";
        c.host = "2001:DB8::1";
        c.port = 9000;
        c.path = "/api";
        const n2 = serializeUri(c, out[0..]);
        expect_eq(out[0..n2], "http://[2001:db8::1]:9000/api");
    });

    test("4. @cap/fast-uri — resolveUri RFC 3986 §5.4 standard relative references", () => {
        let out: [256]u8 = undefined;
        const base = "http://a/b/c/d;p?q";

        const n1 = resolveUri(base, "g", out[0..]);
        expect_eq(out[0..n1], "http://a/b/c/g");

        const n2 = resolveUri(base, "./g", out[0..]);
        expect_eq(out[0..n2], "http://a/b/c/g");

        const n3 = resolveUri(base, "/g", out[0..]);
        expect_eq(out[0..n3], "http://a/g");

        const n4 = resolveUri(base, "//g", out[0..]);
        expect_eq(out[0..n4], "http://g");

        const n5 = resolveUri(base, "?y", out[0..]);
        expect_eq(out[0..n5], "http://a/b/c/d;p?y");

        const n6 = resolveUri(base, "#s", out[0..]);
        expect_eq(out[0..n6], "http://a/b/c/d;p?q#s");
    });

    test("5. @cap/fast-uri — resolveUri dot-segment removal edge cases", () => {
        let out: [256]u8 = undefined;
        const base = "http://a/b/c/d;p?q";

        const n1 = resolveUri(base, "..", out[0..]);
        expect_eq(out[0..n1], "http://a/b/");

        const n2 = resolveUri(base, "../g", out[0..]);
        expect_eq(out[0..n2], "http://a/b/g");

        const n3 = resolveUri(base, "../../g", out[0..]);
        expect_eq(out[0..n3], "http://a/g");

        const n4 = resolveUri(base, "../../../g", out[0..]);
        expect_eq(out[0..n4], "http://a/g");

        const n5 = resolveUri(base, "mid/content=5/../6", out[0..]);
        expect_eq(out[0..n5], "http://a/b/c/mid/6");
    });

    test("6. @cap/fast-uri — equalUri normalized equivalence comparison", () => {
        expect_eq(equalUri("HTTP://Example.COM:80/a/./b/../c", "http://example.com/a/c"), true);
        expect_eq(equalUri("https://example.com:443", "https://EXAMPLE.com/"), true);
        expect_eq(equalUri("http://example.com/%2f", "http://example.com/%2F"), true);
        expect_eq(equalUri("http://example.com:8080/a", "http://example.com/a"), false);
        expect_eq(equalUri("http://example.com/a?x=1", "http://example.com/a?x=2"), false);
        expect_eq(equalUri("http://[::1/bad", "http://[::1/bad"), false);
    });
}
