import { test, expect_eq } from "cap:test";
import {
    hasProtocol,
    isRelative,
    withoutLeadingSlash,
    withoutTrailingSlash,
    withLeadingSlash,
    withTrailingSlash,
    cleanDoubleSlashes,
    joinUrl,
} from "../../packages/@cap/ufo/src/index.cp";

fn main() {
    test("1. @cap/ufo — hasProtocol detects valid URL schemes", () => {
        expect_eq(hasProtocol("https://example.com/a"), true);
        expect_eq(hasProtocol("http://localhost:8080"), true);
        expect_eq(hasProtocol("ws://127.0.0.1/socket"), true);
        expect_eq(hasProtocol("custom+proto://host"), true);
        expect_eq(hasProtocol("/relative/path"), false);
        expect_eq(hasProtocol("./local"), false);
        expect_eq(hasProtocol("localhost:3000"), false);
        expect_eq(hasProtocol("1bad://host"), false);
    });

    test("2. @cap/ufo — isRelative distinguishes relative paths from absolute and protocol URLs", () => {
        expect_eq(isRelative("./foo/bar"), true);
        expect_eq(isRelative("../up"), true);
        expect_eq(isRelative("assets/logo.png"), true);
        expect_eq(isRelative("/abs/path"), false);
        expect_eq(isRelative("//cdn.example.com/a"), false);
        expect_eq(isRelative("https://example.com"), false);
        expect_eq(isRelative(""), false);
    });

    test("3. @cap/ufo — withoutLeadingSlash and withoutTrailingSlash strip slashes", () => {
        expect_eq(withoutLeadingSlash("/foo/bar"), "foo/bar");
        expect_eq(withoutLeadingSlash("///foo"), "foo");
        expect_eq(withoutLeadingSlash("foo"), "foo");
        expect_eq(withoutTrailingSlash("/foo/bar/"), "/foo/bar");
        expect_eq(withoutTrailingSlash("foo///"), "foo");
        expect_eq(withoutTrailingSlash("/"), "");
    });

    test("4. @cap/ufo — withLeadingSlash and withTrailingSlash ensure single boundary slash", () => {
        let buf: [64]u8 = undefined;
        const n1 = withLeadingSlash("api/v1", buf[0..64]);
        expect_eq(buf[0..n1], "/api/v1");
        const n2 = withLeadingSlash("/api/v1", buf[0..64]);
        expect_eq(buf[0..n2], "/api/v1");

        const n3 = withTrailingSlash("/api/v1", buf[0..64]);
        expect_eq(buf[0..n3], "/api/v1/");
        const n4 = withTrailingSlash("/api/v1/", buf[0..64]);
        expect_eq(buf[0..n4], "/api/v1/");
    });

    test("5. @cap/ufo — cleanDoubleSlashes collapses extra slashes while preserving protocol", () => {
        let buf: [128]u8 = undefined;
        const n1 = cleanDoubleSlashes("https://example.com//a///b//c", buf[0..128]);
        expect_eq(buf[0..n1], "https://example.com/a/b/c");

        const n2 = cleanDoubleSlashes("http://host///", buf[0..128]);
        expect_eq(buf[0..n2], "http://host/");

        const n3 = cleanDoubleSlashes("//foo///bar//baz", buf[0..128]);
        expect_eq(buf[0..n3], "/foo/bar/baz");
    });

    test("6. @cap/ufo — joinUrl safely joins base and segment without duplicate slashes", () => {
        let buf: [128]u8 = undefined;
        const n1 = joinUrl("https://example.com/", "/api/v1", buf[0..128]);
        expect_eq(buf[0..n1], "https://example.com/api/v1");

        const n2 = joinUrl("https://example.com", "api/v1", buf[0..128]);
        expect_eq(buf[0..n2], "https://example.com/api/v1");

        const n3 = joinUrl("https://example.com///", "///users", buf[0..128]);
        expect_eq(buf[0..n3], "https://example.com/users");

        const n4 = joinUrl("/", "/hello", buf[0..128]);
        expect_eq(buf[0..n4], "/hello");

        const n5 = joinUrl("https://example.com", "", buf[0..128]);
        expect_eq(buf[0..n5], "https://example.com");
    });
}
