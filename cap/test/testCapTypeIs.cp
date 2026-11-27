import { test, expect_eq } from "cap:test";
import {
    RequestHeaders,
    typeIs, typeis, typeisCsv, is, matchType, normalize,
    typeMatches, normalizeTypePattern, hasBody, requestHasBody,
    extractEssence, isValidContentType, isJson, isText, isForm, isMultipart,
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
        expect_eq(typeMatches("application/octet-stream", "*"), true);
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

    test("5. @cap/type-is — normalizeTypePattern & strict normalize", () => {
        expect_eq(normalizeTypePattern("urlencoded"), "application/x-www-form-urlencoded");
        expect_eq(normalizeTypePattern("multipart"), "multipart/*");
        expect_eq(normalizeTypePattern("json"), "application/json");
        expect_eq(normalizeTypePattern("+json"), "+json");

        expect_eq(normalize("urlencoded") ?? "", "application/x-www-form-urlencoded");
        expect_eq(normalize("multipart") ?? "", "multipart/*");
        expect_eq(normalize("*") ?? "", "*/*");
        expect_eq(normalize("+xml") ?? "", "+xml");
        expect_eq(normalize("+") == null, true);
        expect_eq(normalize("") == null, true);
        expect_eq(normalize("notarealextensionxyz") == null, true);
    });

    test("6. @cap/type-is — hasBody & requestHasBody detection", () => {
        expect_eq(hasBody("chunked", ""), true);
        expect_eq(hasBody("", "128"), true);
        expect_eq(hasBody("", "0"), false);
        expect_eq(hasBody("", ""), false);

        const req_chunked = RequestHeaders{ content_type: "application/json", transfer_encoding: "chunked", content_length: "" };
        const req_empty = RequestHeaders{ content_type: "application/json", transfer_encoding: "", content_length: "0" };
        expect_eq(requestHasBody(req_chunked), true);
        expect_eq(requestHasBody(req_empty), false);
    });

    test("7. @cap/type-is — npm is(mediaType, types) & matchType(expected, actual)", () => {
        const pats: [3]string = ["html", "+json", "text/*"];
        expect_eq(is("application/vnd.api+json; charset=utf-8", pats[0..3]) ?? "", "+json");
        expect_eq(is("text/plain", pats[0..3]) ?? "", "text/*");
        expect_eq(is("image/png", pats[0..3]) == null, true);
        // Empty types slice returns normalized essence
        expect_eq(is("application/json; charset=utf-8", pats[0..0]) ?? "", "application/json");

        expect_eq(matchType("application/*+json", "application/ld+json"), true);
        expect_eq(matchType("text/*", "application/json"), false);
        expect_eq(matchType("", "application/json"), false);
    });

    test("8. @cap/type-is — typeis(req, types), typeisCsv & convenience predicates", () => {
        const req_ok = RequestHeaders{
            content_type: "application/problem+json; charset=utf-8",
            transfer_encoding: "",
            content_length: "42",
        };
        const req_no_body = RequestHeaders{
            content_type: "application/json",
            transfer_encoding: "",
            content_length: "0",
        };
        const pats: [2]string = ["xml", "+json"];
        expect_eq(typeis(req_ok, pats[0..2]) ?? "", "+json");
        expect_eq(typeis(req_ok, pats[0..0]) ?? "", "application/problem+json");
        expect_eq(typeis(req_no_body, pats[0..2]) == null, true);
        expect_eq(typeisCsv(req_ok, "html, +json") ?? "", "+json");
        expect_eq(typeisCsv(req_ok, "") ?? "", "application/problem+json");
        expect_eq(typeisCsv(req_no_body, "json") == null, true);

        expect_eq(extractEssence("text/html; charset=utf-8") ?? "", "text/html");
        expect_eq(isValidContentType("application/json"), true);
        expect_eq(isValidContentType("invalid"), false);
        expect_eq(isJson("application/vnd.api+json"), true);
        expect_eq(isText("text/csv"), true);
        expect_eq(isForm("application/x-www-form-urlencoded"), true);
        expect_eq(isMultipart("multipart/form-data; boundary=123"), true);
    });
}
