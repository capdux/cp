// cap/test/testCapParseurl.cp
// Comprehensive unit tests for @cap/parseurl (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    ParsedRequestUrl,
    parseRequestUrl,
    decodePathname,
    isSafeRequestPath,
    getRequestQueryParam,
    decodeQueryValue
} from "../../packages/parseurl/src/index.cp";

fn main() {
    test("1. parseRequestUrl — full origin-form URL with query and hash", () => {
        const u: ParsedRequestUrl = parseRequestUrl("/api/v1/users?page=2&sort=asc#top");
        expect_eq(u.href, "/api/v1/users?page=2&sort=asc#top");
        expect_eq(u.pathname, "/api/v1/users");
        expect_eq(u.search, "?page=2&sort=asc");
        expect_eq(u.query, "page=2&sort=asc");
        expect_eq(u.hash, "#top");
        expect_eq(u.path, "/api/v1/users?page=2&sort=asc");
        expect_eq(u.has_query, true);
    });

    test("2. parseRequestUrl — absolute-form HTTP/HTTPS URLs", () => {
        const u1 = parseRequestUrl("http://host:8080/a/b?x=1");
        expect_eq(u1.pathname, "/a/b");
        expect_eq(u1.search, "?x=1");
        expect_eq(u1.query, "x=1");
        expect_eq(u1.path, "/a/b?x=1");
        expect_eq(u1.hash, "");
        expect_eq(u1.has_query, true);

        const u2 = parseRequestUrl("https://example.com");
        expect_eq(u2.pathname, "/");
        expect_eq(u2.path, "/");
        expect_eq(u2.has_query, false);

        const u3 = parseRequestUrl("https://example.com?k=v#sec");
        expect_eq(u3.pathname, "/");
        expect_eq(u3.query, "k=v");
        expect_eq(u3.hash, "#sec");
        expect_eq(u3.has_query, true);
    });

    test("3. parseRequestUrl — plain path, hash-only & empty string edge cases", () => {
        const u1 = parseRequestUrl("/static/app.js");
        expect_eq(u1.pathname, "/static/app.js");
        expect_eq(u1.path, "/static/app.js");
        expect_eq(u1.search, "");
        expect_eq(u1.query, "");
        expect_eq(u1.hash, "");
        expect_eq(u1.has_query, false);

        const u2 = parseRequestUrl("/docs#intro?notquery");
        expect_eq(u2.pathname, "/docs");
        expect_eq(u2.hash, "#intro?notquery");
        expect_eq(u2.has_query, false);

        const u3 = parseRequestUrl("");
        expect_eq(u3.href, "");
        expect_eq(u3.pathname, "");
        expect_eq(u3.path, "");
        expect_eq(u3.has_query, false);
    });

    test("4. decodePathname — decodes %XX while preserving literal '+'", () => {
        let out: [128]u8 = undefined;

        const n1 = decodePathname("/docs/c++%20guide/a+b%2F1.txt", out[0..128]);
        expect_eq(out[0..n1], "/docs/c++ guide/a+b/1.txt");

        const n2 = decodePathname("/files/%E6%8A%A5%E5%91%8A.pdf", out[0..128]);
        expect_eq(out[0..n2], "/files/报告.pdf");

        // Malformed % sequence preserved verbatim
        const n3 = decodePathname("/100%zz/ok%", out[0..128]);
        expect_eq(out[0..n3], "/100%zz/ok%");
    });

    test("5. isSafeRequestPath — rejects NUL, backslash & '..' traversal segments", () => {
        expect_eq(isSafeRequestPath("/api/v1/users"), true);
        expect_eq(isSafeRequestPath("/files/a..b.txt"), true);
        expect_eq(isSafeRequestPath("/files/.../report.pdf"), true);

        expect_eq(isSafeRequestPath(""), false);
        expect_eq(isSafeRequestPath("/api/../secret"), false);
        expect_eq(isSafeRequestPath("/.."), false);
        expect_eq(isSafeRequestPath("../etc/passwd"), false);
        expect_eq(isSafeRequestPath("/%2e%2e/etc/passwd"), false);
        expect_eq(isSafeRequestPath("/a\\b"), false);
        expect_eq(isSafeRequestPath("/a%5Cb"), false);
        expect_eq(isSafeRequestPath("/a\x00b"), false);
        expect_eq(isSafeRequestPath("/a%00b"), false);
    });

    test("6. getRequestQueryParam & decodeQueryValue — delegates to @cap/querystring", () => {
        let out: [128]u8 = undefined;
        const url = "http://api.local:8080/search?q=hello+world&tag=cp%2Dlang&page=3#results";

        const qLen = getRequestQueryParam(url, "q", out[0..128]);
        expect_eq(qLen != null, true);
        if (qLen != null) {
            expect_eq(out[0..qLen], "hello world");
        }

        const tagLen = getRequestQueryParam(url, "tag", out[0..128]);
        expect_eq(tagLen != null, true);
        if (tagLen != null) {
            expect_eq(out[0..tagLen], "cp-lang");
        }

        const missing = getRequestQueryParam(url, "nonexistent", out[0..128]);
        expect_eq(missing == null, true);

        const noQuery = getRequestQueryParam("/no/query/here", "q", out[0..128]);
        expect_eq(noQuery == null, true);

        const dvLen = decodeQueryValue("foo+bar%21", out[0..128]);
        expect_eq(out[0..dvLen], "foo bar!");
    });
}
