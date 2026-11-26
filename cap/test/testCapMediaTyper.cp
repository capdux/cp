// cap/test/testCapMediaTyper.cp
// Comprehensive unit tests for @cap/media-typer

import { test, expect_eq } from "cap:test";
import {
    MediaType,
    parse,
    testMediaType,
    format,
    isValidToken
} from "../../packages/@cap/media-typer/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/media-typer — basic text/html parsing", () => {
        let input = "text/html";
        let mt = parse(input);
        expect_eq(mt.valid, true);
        expect_eq(mt.has_suffix, false);
        expect_eq(sliceEq(input[mt.type_start..(mt.type_start + mt.type_len)], "text"), true);
        expect_eq(sliceEq(input[mt.subtype_start..(mt.subtype_start + mt.subtype_len)], "html"), true);
        expect_eq(mt.suffix_len, 0 as usize);
        expect_eq(testMediaType("text/html"), true);
    });

    test("2. @cap/media-typer — image/svg+xml & application/vnd.api+json suffix extraction", () => {
        let s1 = "image/svg+xml";
        let m1 = parse(s1);
        expect_eq(m1.valid, true);
        expect_eq(m1.has_suffix, true);
        expect_eq(sliceEq(s1[m1.type_start..(m1.type_start + m1.type_len)], "image"), true);
        expect_eq(sliceEq(s1[m1.subtype_start..(m1.subtype_start + m1.subtype_len)], "svg"), true);
        expect_eq(sliceEq(s1[m1.suffix_start..(m1.suffix_start + m1.suffix_len)], "xml"), true);

        let s2 = "application/vnd.api+json";
        let m2 = parse(s2);
        expect_eq(m2.valid, true);
        expect_eq(m2.has_suffix, true);
        expect_eq(sliceEq(s2[m2.type_start..(m2.type_start + m2.type_len)], "application"), true);
        expect_eq(sliceEq(s2[m2.subtype_start..(m2.subtype_start + m2.subtype_len)], "vnd.api"), true);
        expect_eq(sliceEq(s2[m2.suffix_start..(m2.suffix_start + m2.suffix_len)], "json"), true);
    });

    test("3. @cap/media-typer — complex vendor subtype (OOXML wordprocessingml)", () => {
        let s = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        let mt = parse(s);
        expect_eq(mt.valid, true);
        expect_eq(mt.has_suffix, false);
        expect_eq(sliceEq(s[mt.type_start..(mt.type_start + mt.type_len)], "application"), true);
        expect_eq(
            sliceEq(
                s[mt.subtype_start..(mt.subtype_start + mt.subtype_len)],
                "vnd.openxmlformats-officedocument.wordprocessingml.document"
            ),
            true
        );
        expect_eq(isValidToken("vnd.openxmlformats-officedocument.wordprocessingml.document"), true);
        expect_eq(isValidToken("x-custom_type!#$&^"), true);
    });

    test("4. @cap/media-typer — format valid media types & reject invalid parts", () => {
        let buf: [64]u8 = undefined;

        let n1 = format("application", "ld", "json", buf[0..64]);
        expect_eq(n1, 19 as usize);
        expect_eq(sliceEq(buf[0..n1], "application/ld+json"), true);

        let n2 = format("text", "plain", "", buf[0..64]);
        expect_eq(n2, 10 as usize);
        expect_eq(sliceEq(buf[0..n2], "text/plain"), true);

        // Reject invalid type (dot in type, leading dash, space, empty)
        expect_eq(format("text.bad", "plain", "", buf[0..64]), 0 as usize);
        expect_eq(format("-text", "plain", "", buf[0..64]), 0 as usize);
        expect_eq(format("", "plain", "", buf[0..64]), 0 as usize);
        expect_eq(format("text", "plain;charset=utf-8", "", buf[0..64]), 0 as usize);
        expect_eq(format("application", "ld", "bad.suf", buf[0..64]), 0 as usize);
    });

    test("5. @cap/media-typer — testMediaType rejects semicolons, spaces, empty & double slashes", () => {
        expect_eq(testMediaType(""), false);
        expect_eq(testMediaType("text"), false);
        expect_eq(testMediaType("text/"), false);
        expect_eq(testMediaType("/html"), false);
        expect_eq(testMediaType("text//html"), false);
        expect_eq(testMediaType("text/html/extra"), false);
        expect_eq(testMediaType(" text/html"), false);
        expect_eq(testMediaType("text/html "), false);
        expect_eq(testMediaType("text / html"), false);
        expect_eq(testMediaType("text/html; charset=utf-8"), false);
        expect_eq(testMediaType("text/html;charset=utf-8"), false);
        expect_eq(testMediaType("image/+xml"), false);
        expect_eq(testMediaType("image/svg+"), false);
        expect_eq(testMediaType("-text/html"), false);
        expect_eq(testMediaType("text/-html"), false);
    });

    test("6. @cap/media-typer — zero-length & buffer boundary safety", () => {
        let empty = parse("");
        expect_eq(empty.valid, false);
        expect_eq(isValidToken(""), false);
        expect_eq(isValidToken("-leading"), false);
        expect_eq(isValidToken("has space"), false);

        // Buffer too small for "application/ld+json" (needs 19 bytes)
        let tiny: [18]u8 = undefined;
        expect_eq(format("application", "ld", "json", tiny[0..18]), 0 as usize);

        // Exact buffer size (19 bytes) succeeds
        let exact: [19]u8 = undefined;
        let n = format("application", "ld", "json", exact[0..19]);
        expect_eq(n, 19 as usize);
        expect_eq(sliceEq(exact[0..n], "application/ld+json"), true);
    });
}
