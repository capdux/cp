import { test, expect_eq } from "cap:test";
import {
    typeIs, typeMatches, normalizeTypePattern, hasBody
} from "../../packages/type-is/src/index.cp";

fn main() {
    test("1. @cap/type-is — Exact & shorthand extension matching via @cap/mime-types", () => {
        expect_eq(typeMatches("application/json; charset=utf-8", "json"), true);
        expect_eq(typeMatches("text/html; charset=UTF-8", "html"), true);
        expect_eq(typeMatches("application/x-www-form-urlencoded", "urlencoded"), true);
        expect_eq(typeMatches("multipart/form-data; boundary=abc", "multipart"), true);
        expect_eq(typeMatches("text/plain", "json"), false);
    });

    test("2. @cap/type-is — Wildcard type/* and */* patterns", () => {
        expect_eq(typeMatches("text/css", "text/*"), true);
        expect_eq(typeMatches("image/png", "image/*"), true);
        expect_eq(typeMatches("image/png", "text/*"), false);
        expect_eq(typeMatches("application/octet-stream", "*/*"), true);
    });

    test("3. @cap/type-is — Structured suffix +json and application/*+xml", () => {
        expect_eq(typeMatches("application/vnd.api+json", "+json"), true);
        expect_eq(typeMatches("application/json", "+json"), true);
        expect_eq(typeMatches("application/problem+json; charset=utf-8", "application/*+json"), true);
        expect_eq(typeMatches("image/svg+xml", "image/*+xml"), true);
        expect_eq(typeMatches("text/plain", "+json"), false);
    });

    test("4. @cap/type-is — typeIs returns first matching candidate from CSV list", () => {
        expect_eq(typeIs("application/vnd.api+json", "html, +json, text/*") ?? "", "+json");
        expect_eq(typeIs("text/markdown", "json, text/*, */*") ?? "", "text/*");
        expect_eq(typeIs("image/webp", "json, html, xml") == null, true);
    });

    test("5. @cap/type-is — normalizeTypePattern helper", () => {
        expect_eq(normalizeTypePattern("urlencoded"), "application/x-www-form-urlencoded");
        expect_eq(normalizeTypePattern("multipart"), "multipart/*");
        expect_eq(normalizeTypePattern("json"), "application/json");
        expect_eq(normalizeTypePattern("+json"), "+json");
    });

    test("6. @cap/type-is — hasBody request body detection", () => {
        expect_eq(hasBody("chunked", ""), true);
        expect_eq(hasBody("", "128"), true);
        expect_eq(hasBody("", "0"), false);
        expect_eq(hasBody("", ""), false);
    });
}
