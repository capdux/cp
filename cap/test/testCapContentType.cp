// cap/test/testCapContentType.cp
// Comprehensive unit tests for @cap/content-type

import { test, expect_eq } from "cap:test";
import {
    parseContentType,
    formatContentType,
    isJsonContentType,
    isTextContentType,
    isMultipartContentType,
} from "../../packages/@cap/content-type/src/index.cp";

fn main() {
    test("1. @cap/content-type — basic text/html; charset=utf-8 parsing", () => {
        const ct = parseContentType("text/html; charset=utf-8");
        expect_eq(ct.ok, true);
        expect_eq(ct.type_str, "text");
        expect_eq(ct.subtype, "html");
        expect_eq(ct.suffix, "");
        expect_eq(ct.essence, "text/html");
        expect_eq(ct.param_count, 1 as usize);
        expect_eq(ct.charset() != null, true);
        expect_eq(ct.charset().?, "utf-8");
        expect_eq(ct.boundary() == null, true);
        expect_eq(isTextContentType("text/html; charset=utf-8"), true);
        expect_eq(isJsonContentType("text/html; charset=utf-8"), false);
    });

    test("2. @cap/content-type — multipart/form-data with quoted boundary", () => {
        const hdr = "multipart/form-data; boundary=\"----WebKitFormBoundary7MA4YWxkTrZu0gW\"";
        const ct = parseContentType(hdr);
        expect_eq(ct.ok, true);
        expect_eq(ct.type_str, "multipart");
        expect_eq(ct.subtype, "form-data");
        expect_eq(ct.boundary() != null, true);
        expect_eq(ct.boundary().?, "----WebKitFormBoundary7MA4YWxkTrZu0gW");
        expect_eq(isMultipartContentType(hdr), true);
        expect_eq(isTextContentType(hdr), false);
    });

    test("3. @cap/content-type — RFC 6838 +suffix & JSON predicates", () => {
        const ct1 = parseContentType("application/problem+json; charset=utf-8");
        expect_eq(ct1.ok, true);
        expect_eq(ct1.type_str, "application");
        expect_eq(ct1.subtype, "problem+json");
        expect_eq(ct1.suffix, "json");
        expect_eq(ct1.essence, "application/problem+json");

        expect_eq(isJsonContentType("application/json"), true);
        expect_eq(isJsonContentType("text/json; charset=utf-8"), true);
        expect_eq(isJsonContentType("application/problem+json"), true);
        expect_eq(isJsonContentType("application/vnd.api+json; profile=\"v1\""), true);
        expect_eq(isJsonContentType("application/ld+json"), true);
        expect_eq(isJsonContentType("application/xml"), false);
    });

    test("4. @cap/content-type — quoted parameter with embedded semicolon & escaped quotes", () => {
        const hdr = "text/plain; title=\"hello; \\\"world\\\"\"; CHARSET=UTF-8";
        const ct = parseContentType(hdr);
        expect_eq(ct.ok, true);
        expect_eq(ct.param_count, 2 as usize);
        expect_eq(ct.getParam("title").?, "hello; \\\"world\\\"");
        expect_eq(ct.charset().?, "UTF-8");
    });

    test("5. @cap/content-type — formatContentType normalization & round-trip", () => {
        const ct1 = parseContentType("Application/VND.Api+JSON; Charset=utf-8; Note=\"a; b\"");
        expect_eq(ct1.ok, true);

        let buf: [256]u8 = undefined;
        const formatted = formatContentType(ct1, buf[0..256]);
        expect_eq(formatted, "application/vnd.api+json; charset=utf-8; note=\"a; b\"");

        // Round-trip re-parse
        const ct2 = parseContentType(formatted);
        expect_eq(ct2.ok, true);
        expect_eq(ct2.essence, "application/vnd.api+json");
        expect_eq(ct2.suffix, "json");
        expect_eq(ct2.charset().?, "utf-8");
        expect_eq(ct2.getParam("note").?, "a; b");
    });

    test("6. @cap/content-type — reject invalid MIME types & malformed parameters", () => {
        expect_eq(parseContentType("").ok, false);
        expect_eq(parseContentType("text").ok, false);
        expect_eq(parseContentType("text/").ok, false);
        expect_eq(parseContentType("/html").ok, false);
        expect_eq(parseContentType("text/html/extra").ok, false);
        expect_eq(parseContentType("text / html").ok, false);
        expect_eq(parseContentType("text/html; charset").ok, false);
        expect_eq(parseContentType("text/html; charset=").ok, false);
        expect_eq(parseContentType("text/html; charset=\"unclosed").ok, false);
        expect_eq(parseContentType("text/html; bad key=val").ok, false);

        expect_eq(isJsonContentType("invalid"), false);
        expect_eq(isTextContentType("invalid"), false);
        expect_eq(isMultipartContentType("invalid"), false);
    });
}
