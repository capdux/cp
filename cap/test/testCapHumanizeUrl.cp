// cap/test/testCapHumanizeUrl.cp
// Comprehensive unit tests for humanize-url (8 test suites)

import { test, expect_eq } from "cap:test";
import {
    isProtocolRelative,
    extractScheme,
    stripProtocol,
    stripWww,
    stripTrailingSlash,
    humanizeUrlSlice,
    humanizedLen,
    humanizeUrl,
} from "../../packages/humanize-url/src/index.cp";

fn main() {
    test("1. humanize-url — strips http://, https://, // and trailing slash", () => {
        expect_eq(humanizeUrlSlice("http://sindresorhus.com"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("https://sindresorhus.com/"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("//sindresorhus.com/"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("  HTTPS://example.org/  "), "example.org");
    });

    test("2. humanize-url — strips case-insensitive www. without breaking www.com or www.com:8080", () => {
        expect_eq(humanizeUrlSlice("http://www.sindresorhus.com"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("https://WWW.Example.COM/"), "Example.COM");
        expect_eq(humanizeUrlSlice("//www.docs.example.io/"), "docs.example.io");
        expect_eq(humanizeUrlSlice("https://www.com/"), "www.com");
        expect_eq(humanizeUrlSlice("https://www.com/about"), "www.com/about");
        expect_eq(humanizeUrlSlice("http://www.com:8080/"), "www.com:8080");
        expect_eq(humanizeUrlSlice("//www.api.example.com:3000/v1/"), "api.example.com:3000/v1");
    });

    test("3. humanize-url — preserves paths, queries, ports, and fragments", () => {
        expect_eq(
            humanizeUrlSlice("https://www.example.com/a/b/c/"),
            "example.com/a/b/c",
        );
        expect_eq(
            humanizeUrlSlice("http://example.com:8080/api?v=2&ok=1#res"),
            "example.com:8080/api?v=2&ok=1#res",
        );
        expect_eq(
            humanizeUrlSlice("https://example.com/?x=1"),
            "example.com/?x=1",
        );
    });

    test("4. humanize-url — keeps non-http schemes intact", () => {
        expect_eq(humanizeUrlSlice("ftp://files.example.com/"), "ftp://files.example.com");
        expect_eq(humanizeUrlSlice("mailto:hi@example.com"), "mailto:hi@example.com");
        expect_eq(humanizeUrlSlice("example.com/"), "example.com");
    });

    test("5. humanize-url — humanizeUrl writes into caller buffer & humanizedLen matches", () => {
        let buf: [128]u8 = undefined;
        const url1 = "  https://www.github.com/sindresorhus/  ";
        const n1 = humanizeUrl(url1, buf[0..128]);
        expect_eq(buf[0..n1], "github.com/sindresorhus");
        expect_eq(n1, humanizedLen(url1));

        const n2 = humanizeUrl("HTTP://WWW.CP-LANG.ORG/", buf[0..128]);
        expect_eq(buf[0..n2], "CP-LANG.ORG");
    });

    test("6. humanize-url — empty/whitespace/protocol-only inputs and bounded output buffer", () => {
        expect_eq(humanizeUrlSlice(""), "");
        expect_eq(humanizeUrlSlice("   \t\r\n "), "");
        expect_eq(humanizeUrlSlice("http://"), "");
        expect_eq(humanizeUrlSlice("https://"), "");
        expect_eq(humanizeUrlSlice("//"), "");

        let buf: [64]u8 = undefined;
        expect_eq(humanizeUrl("", buf[0..64]), 0 as usize);

        let tiny: [7]u8 = undefined;
        const nTiny = humanizeUrl("https://www.sindresorhus.com", tiny[0..7]);
        expect_eq(nTiny, 7 as usize);
        expect_eq(tiny[0..nTiny], "sindres");
    });

    test("7. humanize-url — extractScheme & isProtocolRelative helpers", () => {
        expect_eq(extractScheme("https://example.com"), "https");
        expect_eq(extractScheme("  HTTP://a.b"), "HTTP");
        expect_eq(extractScheme("custom+proto://host"), "custom+proto");
        expect_eq(extractScheme("//example.com"), "");
        expect_eq(extractScheme("mailto:user@host"), "");

        expect_eq(isProtocolRelative("//cdn.example.com/a.js"), true);
        expect_eq(isProtocolRelative("  //cdn.example.com"), true);
        expect_eq(isProtocolRelative("https://cdn.example.com"), false);
        expect_eq(isProtocolRelative("/local/path"), false);
    });

    test("8. humanize-url — stripProtocol, stripWww, and stripTrailingSlash standalone", () => {
        expect_eq(stripProtocol("HTTPS://www.test.dev/p/"), "www.test.dev/p/");
        expect_eq(stripProtocol("ftp://files.dev/"), "ftp://files.dev/");
        expect_eq(stripWww("www.sub.dev/x"), "sub.dev/x");
        expect_eq(stripWww("www.com"), "www.com");
        expect_eq(stripTrailingSlash("a/b/"), "a/b");
        expect_eq(stripTrailingSlash("a/b"), "a/b");
    });
}
