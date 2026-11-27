// cap/test/testCapNormalizeUrl.cp
// Comprehensive unit tests for @cap/normalize-url (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    NormalizeUrlOptions,
    defaultNormalizeUrlOptions,
    normalizeUrl,
    normalizeUrlWithOptions,
} from "../../packages/normalize-url/src/index.cp";

fn main() {
    test("1. @cap/normalize-url — prepends default protocol, lowercases scheme/host, strips www", () => {
        let buf: [256]u8 = undefined;
        const n1 = normalizeUrl("sindresorhus.com", buf[0..256]);
        expect_eq(buf[0..n1], "http://sindresorhus.com");

        const n2 = normalizeUrl("  HTTP://WWW.SindreSorhus.COM/  ", buf[0..256]);
        expect_eq(buf[0..n2], "http://sindresorhus.com");

        const n3 = normalizeUrl("https://www.com/path", buf[0..256]);
        expect_eq(buf[0..n3], "https://www.com/path");
    });

    test("2. @cap/normalize-url — removes default ports (http:80, https:443) and keeps custom ports", () => {
        let buf: [256]u8 = undefined;
        const n1 = normalizeUrl("http://example.com:80/api", buf[0..256]);
        expect_eq(buf[0..n1], "http://example.com/api");

        const n2 = normalizeUrl("https://example.com:443/api", buf[0..256]);
        expect_eq(buf[0..n2], "https://example.com/api");

        const n3 = normalizeUrl("http://example.com:8080/api", buf[0..256]);
        expect_eq(buf[0..n3], "http://example.com:8080/api");

        const n4 = normalizeUrl("https://[::1]:443/v1", buf[0..256]);
        expect_eq(buf[0..n4], "https://[::1]/v1");
    });

    test("3. @cap/normalize-url — collapses duplicate slashes and resolves dot segments (. and ..)", () => {
        let buf: [256]u8 = undefined;
        const n1 = normalizeUrl("http://example.com//foo///bar//baz/", buf[0..256]);
        expect_eq(buf[0..n1], "http://example.com/foo/bar/baz");

        const n2 = normalizeUrl("https://example.com/a/b/c/./../../g", buf[0..256]);
        expect_eq(buf[0..n2], "https://example.com/a/g");

        const n3 = normalizeUrl("https://example.com/../../outside/ok", buf[0..256]);
        expect_eq(buf[0..n3], "https://example.com/outside/ok");
    });

    test("4. @cap/normalize-url — default_https, strip_www, remove_default_port, and strip_hash options", () => {
        let buf: [256]u8 = undefined;
        let opts = defaultNormalizeUrlOptions();
        opts.default_https = true;
        opts.strip_hash = true;

        const n1 = normalizeUrlWithOptions("www.example.com/docs/intro/#section-1", opts, buf[0..256]);
        expect_eq(buf[0..n1], "https://example.com/docs/intro");

        let keepOpts = NormalizeUrlOptions{
            default_https: false,
            strip_www: false,
            strip_hash: false,
            remove_trailing_slash: false,
            remove_default_port: false,
            normalize_protocol: true,
        };
        const n2 = normalizeUrlWithOptions("http://www.example.com:80/docs/#top", keepOpts, buf[0..256]);
        expect_eq(buf[0..n2], "http://www.example.com:80/docs/#top");
    });

    test("5. @cap/normalize-url — normalize_protocol controls protocol-relative // URLs", () => {
        let buf: [256]u8 = undefined;
        const n1 = normalizeUrl("//www.example.com/assets//app.js", buf[0..256]);
        expect_eq(buf[0..n1], "http://example.com/assets/app.js");

        let httpsOpts = defaultNormalizeUrlOptions();
        httpsOpts.default_https = true;
        const n2 = normalizeUrlWithOptions("//cdn.example.com/lib.js", httpsOpts, buf[0..256]);
        expect_eq(buf[0..n2], "https://cdn.example.com/lib.js");

        let keepRelOpts = defaultNormalizeUrlOptions();
        keepRelOpts.normalize_protocol = false;
        const n3 = normalizeUrlWithOptions("//WWW.Example.COM/a//b/", keepRelOpts, buf[0..256]);
        expect_eq(buf[0..n3], "//example.com/a/b");
    });

    test("6. @cap/normalize-url — preserves userinfo/query, handles empty input & bounded buffer", () => {
        let buf: [256]u8 = undefined;
        const n1 = normalizeUrl("https://User:Pass@WWW.Example.COM/Search?q=CP&lang=en#res", buf[0..256]);
        expect_eq(buf[0..n1], "https://User:Pass@example.com/Search?q=CP&lang=en#res");

        expect_eq(normalizeUrl("", buf[0..256]), 0 as usize);
        expect_eq(normalizeUrl("   \t\n ", buf[0..256]), 0 as usize);

        let tiny: [12]u8 = undefined;
        const nTiny = normalizeUrl("https://example.com/long/path", tiny[0..12]);
        expect_eq(nTiny, 12 as usize);
        expect_eq(tiny[0..nTiny], "https://exam");
    });
}
