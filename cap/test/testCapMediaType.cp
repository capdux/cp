// cap/test/testCapMediaType.cp
// Comprehensive unit tests for @cap/media-type

import { test, expect_eq } from "cap:test";
import {
    MediaType,
    isValidToken,
    parseMediaType,
    getParam,
    formatMediaType,
    isJsonLike,
    isTextLike,
} from "../../packages/mediatype/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn emptyMt(): MediaType {
    return MediaType{
        top: "",
        subtype: "",
        suffix: "",
        params: "",
    };
}

fn main() {
    test("1. @cap/media-type — Parse standard text/html and +json/+xml structured suffixes", () => {
        let mt1 = emptyMt();
        expect_eq(parseMediaType("text/html; charset=utf-8", &mt1), true);
        expect_eq(sliceEq(mt1.top, "text"), true);
        expect_eq(sliceEq(mt1.subtype, "html"), true);
        expect_eq(mt1.suffix.len, 0 as usize);
        expect_eq(sliceEq(mt1.params, "charset=utf-8"), true);

        let mt2 = emptyMt();
        expect_eq(parseMediaType("application/vnd.api+json; profile=\"https://example.com\"", &mt2), true);
        expect_eq(sliceEq(mt2.top, "application"), true);
        expect_eq(sliceEq(mt2.subtype, "vnd.api"), true);
        expect_eq(sliceEq(mt2.suffix, "json"), true);

        let mt3 = emptyMt();
        expect_eq(parseMediaType("image/svg+xml", &mt3), true);
        expect_eq(sliceEq(mt3.top, "image"), true);
        expect_eq(sliceEq(mt3.subtype, "svg"), true);
        expect_eq(sliceEq(mt3.suffix, "xml"), true);
    });

    test("2. @cap/media-type — Case-insensitive getParam with quote stripping & unescaping", () => {
        let mt = emptyMt();
        expect_eq(parseMediaType("text/plain; Charset=\"utf-8\"; note=\"a; \\\"quoted\\\"\\\\b\"", &mt), true);

        let buf1: [32]u8 = undefined;
        const n1 = getParam(mt, "charset", buf1[0..32]);
        expect_eq(sliceEq(buf1[0..n1], "utf-8"), true);

        let buf2: [64]u8 = undefined;
        const n2 = getParam(mt, "NOTE", buf2[0..64]);
        expect_eq(sliceEq(buf2[0..n2], "a; \"quoted\"\\b"), true);

        expect_eq(getParam(mt, "missing", buf1[0..32]), 0 as usize);
    });

    test("3. @cap/media-type — Reject invalid tokens, missing slash & malformed suffixes", () => {
        let mt = emptyMt();
        expect_eq(isValidToken("application"), true);
        expect_eq(isValidToken("vnd.api+json"), true);
        expect_eq(isValidToken(""), false);
        expect_eq(isValidToken("bad token"), false);
        expect_eq(isValidToken("bad/token"), false);

        expect_eq(parseMediaType("texthtml", &mt), false);
        expect_eq(parseMediaType("text/", &mt), false);
        expect_eq(parseMediaType("/html", &mt), false);
        expect_eq(parseMediaType("application/+json", &mt), false);
        expect_eq(parseMediaType("application/json+", &mt), false);
        expect_eq(parseMediaType("text / html", &mt), false);
    });

    test("4. @cap/media-type — formatMediaType normalizes to lowercase", () => {
        let mt1 = emptyMt();
        expect_eq(parseMediaType("Application/VND.Api+JSON; charset=UTF-8", &mt1), true);
        let buf1: [64]u8 = undefined;
        const n1 = formatMediaType(mt1, buf1[0..64]);
        expect_eq(sliceEq(buf1[0..n1], "application/vnd.api+json"), true);

        let mt2 = emptyMt();
        expect_eq(parseMediaType("TEXT/PLAIN", &mt2), true);
        let buf2: [64]u8 = undefined;
        const n2 = formatMediaType(mt2, buf2[0..64]);
        expect_eq(sliceEq(buf2[0..n2], "text/plain"), true);
    });

    test("5. @cap/media-type — isJsonLike & isTextLike classification", () => {
        let mtJson = emptyMt();
        * = parseMediaType("application/json", &mtJson);
        expect_eq(isJsonLike(mtJson), true);
        expect_eq(isTextLike(mtJson), false);

        let mtHal = emptyMt();
        * = parseMediaType("application/hal+json", &mtHal);
        expect_eq(isJsonLike(mtHal), true);

        let mtSvg = emptyMt();
        * = parseMediaType("image/svg+xml", &mtSvg);
        expect_eq(isTextLike(mtSvg), true);
        expect_eq(isJsonLike(mtSvg), false);

        let mtJs = emptyMt();
        * = parseMediaType("application/javascript", &mtJs);
        expect_eq(isTextLike(mtJs), true);

        let mtPng = emptyMt();
        * = parseMediaType("image/png", &mtPng);
        expect_eq(isJsonLike(mtPng), false);
        expect_eq(isTextLike(mtPng), false);
    });

    test("6. @cap/media-type — Complex multipart/form-data with quoted boundary", () => {
        let mt = emptyMt();
        expect_eq(parseMediaType("multipart/form-data; boundary=\"---abc123\"; charset=utf-8", &mt), true);
        expect_eq(sliceEq(mt.top, "multipart"), true);
        expect_eq(sliceEq(mt.subtype, "form-data"), true);

        let bBuf: [32]u8 = undefined;
        const bn = getParam(mt, "boundary", bBuf[0..32]);
        expect_eq(sliceEq(bBuf[0..bn], "---abc123"), true);

        let cBuf: [32]u8 = undefined;
        const cn = getParam(mt, "charset", cBuf[0..32]);
        expect_eq(sliceEq(cBuf[0..cn], "utf-8"), true);
    });
}
