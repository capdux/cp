// cap/test/testCapPrependHttp.cp
// Comprehensive unit tests for @cap/prepend-http (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    prependHttp,
    hasScheme,
    isProtocolRelative,
} from "../../packages/@cap/prepend-http/src/index.cp";

fn main() {
    test("1. @cap/prepend-http — prepends https:// and http:// to bare domains and host:port", () => {
        let buf: [128]u8 = undefined;
        const n1 = prependHttp("todomvc.com", true, buf[0..128]);
        expect_eq(buf[0..n1], "https://todomvc.com");

        const n2 = prependHttp("todomvc.com", false, buf[0..128]);
        expect_eq(buf[0..n2], "http://todomvc.com");

        const n3 = prependHttp("localhost:3000/api", false, buf[0..128]);
        expect_eq(buf[0..n3], "http://localhost:3000/api");

        const n4 = prependHttp("  \t example.org/docs \r\n ", true, buf[0..128]);
        expect_eq(buf[0..n4], "https://example.org/docs");
    });

    test("2. @cap/prepend-http — preserves existing scheme:// and mailto: URLs", () => {
        let buf: [128]u8 = undefined;
        const n1 = prependHttp("http://todomvc.com", true, buf[0..128]);
        expect_eq(buf[0..n1], "http://todomvc.com");

        const n2 = prependHttp("https://todomvc.com", false, buf[0..128]);
        expect_eq(buf[0..n2], "https://todomvc.com");

        const n3 = prependHttp("ftp://files.example.com/a.zip", true, buf[0..128]);
        expect_eq(buf[0..n3], "ftp://files.example.com/a.zip");

        const n4 = prependHttp("mailto:hi@example.com", true, buf[0..128]);
        expect_eq(buf[0..n4], "mailto:hi@example.com");

        const n5 = prependHttp("MAILTO:ops@example.com", false, buf[0..128]);
        expect_eq(buf[0..n5], "MAILTO:ops@example.com");
    });

    test("3. @cap/prepend-http — preserves protocol-relative and relative paths", () => {
        let buf: [128]u8 = undefined;
        const n1 = prependHttp("//cdn.example.com/app.js", true, buf[0..128]);
        expect_eq(buf[0..n1], "//cdn.example.com/app.js");

        const n2 = prependHttp("./local/index.html", true, buf[0..128]);
        expect_eq(buf[0..n2], "./local/index.html");

        const n3 = prependHttp("../parent/page", false, buf[0..128]);
        expect_eq(buf[0..n3], "../parent/page");

        const n4 = prependHttp("/root/path", true, buf[0..128]);
        expect_eq(buf[0..n4], "/root/path");
    });

    test("4. @cap/prepend-http — hasScheme accurately detects RFC 3986 schemes vs host:port", () => {
        expect_eq(hasScheme("https://example.com"), true);
        expect_eq(hasScheme("HTTP://example.com"), true);
        expect_eq(hasScheme("custom+v1.2-beta://host"), true);
        expect_eq(hasScheme("  mailto:test@example.com  "), true);
        expect_eq(hasScheme("localhost:8080"), false);
        expect_eq(hasScheme("example.com:443/path"), false);
        expect_eq(hasScheme("//example.com"), false);
        expect_eq(hasScheme("123://invalid-leading-digit"), false);
    });

    test("5. @cap/prepend-http — isProtocolRelative detects // with whitespace trimming", () => {
        expect_eq(isProtocolRelative("//example.com"), true);
        expect_eq(isProtocolRelative("  //cdn.site.org/a.css\n"), true);
        expect_eq(isProtocolRelative("/single/slash"), false);
        expect_eq(isProtocolRelative("https://example.com"), false);
        expect_eq(isProtocolRelative(""), false);
    });

    test("6. @cap/prepend-http — empty/whitespace inputs and bounded output buffer", () => {
        let buf: [64]u8 = undefined;
        expect_eq(prependHttp("", true, buf[0..64]), 0 as usize);
        expect_eq(prependHttp("   \t\r\n  ", false, buf[0..64]), 0 as usize);

        let tiny: [10]u8 = undefined;
        const nTiny = prependHttp("example.com", true, tiny[0..10]);
        expect_eq(nTiny, 10 as usize);
        expect_eq(tiny[0..nTiny], "https://ex");
    });
}
