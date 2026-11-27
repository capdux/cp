// cap/test/testCapHumanizeUrl.cp
// Comprehensive unit tests for @cap/humanize-url (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    humanizeUrl,
    humanizeUrlSlice,
} from "../../packages/humanize-url/src/index.cp";

fn main() {
    test("1. @cap/humanize-url — strips http://, https://, // and trailing slash", () => {
        expect_eq(humanizeUrlSlice("http://sindresorhus.com"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("https://sindresorhus.com/"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("//sindresorhus.com/"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("  HTTPS://example.org/  "), "example.org");
    });

    test("2. @cap/humanize-url — strips case-insensitive www. without breaking www.com", () => {
        expect_eq(humanizeUrlSlice("http://www.sindresorhus.com"), "sindresorhus.com");
        expect_eq(humanizeUrlSlice("https://WWW.Example.COM/"), "Example.COM");
        expect_eq(humanizeUrlSlice("//www.docs.example.io/"), "docs.example.io");
        expect_eq(humanizeUrlSlice("https://www.com/"), "www.com");
        expect_eq(humanizeUrlSlice("https://www.com/about"), "www.com/about");
    });

    test("3. @cap/humanize-url — preserves paths, queries, ports, and fragments", () => {
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

    test("4. @cap/humanize-url — keeps non-http schemes intact", () => {
        expect_eq(humanizeUrlSlice("ftp://files.example.com/"), "ftp://files.example.com");
        expect_eq(humanizeUrlSlice("mailto:hi@example.com"), "mailto:hi@example.com");
        expect_eq(humanizeUrlSlice("example.com/"), "example.com");
    });

    test("5. @cap/humanize-url — humanizeUrl writes into caller buffer", () => {
        let buf: [128]u8 = undefined;
        const n1 = humanizeUrl("  https://www.github.com/sindresorhus/  ", buf[0..128]);
        expect_eq(buf[0..n1], "github.com/sindresorhus");

        const n2 = humanizeUrl("HTTP://WWW.CP-LANG.ORG/", buf[0..128]);
        expect_eq(buf[0..n2], "CP-LANG.ORG");
    });

    test("6. @cap/humanize-url — empty/whitespace inputs and bounded output buffer", () => {
        expect_eq(humanizeUrlSlice(""), "");
        expect_eq(humanizeUrlSlice("   \t\r\n "), "");

        let buf: [64]u8 = undefined;
        expect_eq(humanizeUrl("", buf[0..64]), 0 as usize);

        let tiny: [7]u8 = undefined;
        const nTiny = humanizeUrl("https://www.sindresorhus.com", tiny[0..7]);
        expect_eq(nTiny, 7 as usize);
        expect_eq(tiny[0..nTiny], "sindres");
    });
}
