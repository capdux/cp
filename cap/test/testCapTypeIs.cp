// cap/test/testCapTypeIs.cp
// Comprehensive unit tests for @cap/type-is

import { test, expect_eq } from "cap:test";
import {
    stripParams,
    normalizeType,
    mimeMatch,
    matchAny,
} from "../../packages/@cap/type-is/src/index.cp";

fn main() {
    test("1. @cap/type-is — stripParams removes ; charset=utf-8 & surrounding spaces", () => {
        expect_eq(stripParams("  application/json; charset=utf-8  "), "application/json");
        expect_eq(stripParams("text/html;charset=ISO-8859-1"), "text/html");
        expect_eq(stripParams("image/png"), "image/png");
    });

    test("2. @cap/type-is — Short alias normalization & matching", () => {
        expect_eq(normalizeType("json"), "application/json");
        expect_eq(normalizeType("urlencoded"), "application/x-www-form-urlencoded");
        expect_eq(normalizeType("multipart"), "multipart/*");
        expect_eq(normalizeType("html"), "text/html");
        expect_eq(normalizeType("text"), "text/plain");
        expect_eq(normalizeType("bin"), "application/octet-stream");

        expect_eq(mimeMatch("APPLICATION/JSON; charset=utf-8", "json"), true);
        expect_eq(mimeMatch("application/x-www-form-urlencoded", "urlencoded"), true);
        expect_eq(mimeMatch("multipart/form-data; boundary=---123", "multipart"), true);
        expect_eq(mimeMatch("text/html; charset=utf-8", "html"), true);
    });

    test("3. @cap/type-is — Wildcard matching (image/*, */json, */*)", () => {
        expect_eq(mimeMatch("image/webp", "image/*"), true);
        expect_eq(mimeMatch("text/plain", "image/*"), false);
        expect_eq(mimeMatch("application/json", "*/json"), true);
        expect_eq(mimeMatch("text/json", "*/json"), true);
        expect_eq(mimeMatch("application/octet-stream", "*/*"), true);
        expect_eq(mimeMatch("application/octet-stream", "*"), true);
    });

    test("4. @cap/type-is — RFC 6839 structured suffixes (+json, +xml)", () => {
        expect_eq(mimeMatch("application/problem+json; charset=utf-8", "+json"), true);
        expect_eq(mimeMatch("application/vnd.api+json", "+json"), true);
        expect_eq(mimeMatch("application/json", "+json"), true);
        expect_eq(mimeMatch("image/svg+xml", "+xml"), true);
        expect_eq(mimeMatch("application/xml", "+xml"), true);
        expect_eq(mimeMatch("text/plain", "+json"), false);
    });

    test("5. @cap/type-is — matchAny returns first matching pattern index or -1", () => {
        const pats: [3][]const u8 = ["html", "urlencoded", "+json"];
        expect_eq(matchAny("application/vnd.api+json; charset=utf-8", pats[0..3]), 2 as i64);
        expect_eq(matchAny("text/html", pats[0..3]), 0 as i64);
        expect_eq(matchAny("image/png", pats[0..3]), -1 as i64);
    });

    test("6. @cap/type-is — Empty & malformed Content-Type safely return false", () => {
        expect_eq(mimeMatch("", "json"), false);
        expect_eq(mimeMatch("invalid-no-slash", "*/*"), false);
        expect_eq(mimeMatch("/json", "json"), false);
        expect_eq(mimeMatch("application/", "application/*"), false);
        expect_eq(mimeMatch("a/b/c", "*/*"), false);
        expect_eq(mimeMatch("application/json", ""), false);
    });
}
