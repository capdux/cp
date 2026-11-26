import { test, expect_eq } from "cap:test";
import {
    camelCase,
    pascalCase,
    snakeCase,
    kebabCase,
    constantCase,
    dotCase,
    pathCase,
    trainCase,
    capitalCase,
    sentenceCase,
    noCase,
    swapCase,
} from "../../packages/@cap/change-case/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/change-case — camelCase & pascalCase cross-package reuse", () => {
        let buf: [64]u8 = undefined;
        const n1 = camelCase("foo_bar-baz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "fooBarBaz"), true);

        const n2 = pascalCase("foo_bar-baz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "FooBarBaz"), true);

        const n3 = camelCase("HTTP_RESPONSE_CODE", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "httpResponseCode"), true);

        const n4 = pascalCase("content-type", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n4, "ContentType"), true);
    });

    test("2. @cap/change-case — snakeCase, kebabCase & constantCase", () => {
        let buf: [64]u8 = undefined;
        const n1 = snakeCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "foo_bar_baz"), true);

        const n2 = kebabCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "foo-bar-baz"), true);

        const n3 = constantCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "FOO_BAR_BAZ"), true);

        const n4 = snakeCase("foo-bar  baz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n4, "foo_bar_baz"), true);

        const n5 = constantCase("content-type", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n5, "CONTENT_TYPE"), true);
    });

    test("3. @cap/change-case — dotCase, pathCase & noCase", () => {
        let buf: [64]u8 = undefined;
        const n1 = dotCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "foo.bar.baz"), true);

        const n2 = pathCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "foo/bar/baz"), true);

        const n3 = noCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "foo bar baz"), true);

        const n4 = pathCase("API_v2_users", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n4, "api/v2/users"), true);
    });

    test("4. @cap/change-case — trainCase, capitalCase & sentenceCase", () => {
        let buf: [64]u8 = undefined;
        const n1 = trainCase("content_type", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "Content-Type"), true);

        const n2 = trainCase("xForwardedFor", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "X-Forwarded-For"), true);

        const n3 = capitalCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "Foo Bar Baz"), true);

        const n4 = sentenceCase("fooBarBaz", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n4, "Foo bar baz"), true);
    });

    test("5. @cap/change-case — swapCase & acronym handling", () => {
        let buf: [64]u8 = undefined;
        const n1 = swapCase("FooBar", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "fOObAR"), true);

        const n2 = swapCase("Hello, World 123!", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "hELLO, wORLD 123!"), true);

        const n3 = snakeCase("parseHTTPResponse", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "parse_http_response"), true);

        const n4 = kebabCase("XMLHttpRequest", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n4, "xml-http-request"), true);

        const n5 = trainCase("parseHTTPResponse", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n5, "Parse-Http-Response"), true);

        const n6 = constantCase("parseHTTPResponse", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n6, "PARSE_HTTP_RESPONSE"), true);
    });

    test("6. @cap/change-case — Empty string & small buffer safe truncation", () => {
        let small: [6]u8 = undefined;
        expect_eq(camelCase("", small[0..6]), 0);
        expect_eq(snakeCase("", small[0..6]), 0);
        expect_eq(trainCase("___", small[0..6]), 0);
        expect_eq(swapCase("", small[0..6]), 0);

        const n1 = snakeCase("fooBarBaz", small[0..6]);
        expect_eq(n1, 6);
        expect_eq(eqBytes(small[0..6], n1, "foo_ba"), true);

        const n2 = trainCase("content_type", small[0..6]);
        expect_eq(n2, 6);
        expect_eq(eqBytes(small[0..6], n2, "Conten"), true);

        const n3 = constantCase("fooBarBaz", small[0..6]);
        expect_eq(n3, 6);
        expect_eq(eqBytes(small[0..6], n3, "FOO_BA"), true);

        const n4 = swapCase("FooBarBaz", small[0..6]);
        expect_eq(n4, 6);
        expect_eq(eqBytes(small[0..6], n4, "fOObAR"), true);

        const n5 = camelCase("fooBarBaz", small[0..6]);
        expect_eq(n5 <= 6, true);
    });
}
