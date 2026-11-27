import { test, expect_eq } from "cap:test";
import {
    parseMimeType,
} from "../../packages/whatwg-mimetype/src/index.cp";

fn main() {
    test("1. @cap/whatwg-mimetype — parses essence and charset parameter", () => {
        const mt = parseMimeType("  Text/HTML; Charset=UTF-8  ");
        expect_eq(mt.valid, true);
        expect_eq(mt.isHTML(), true);
        expect_eq(mt.getParam("charset") ?? "", "UTF-8");
        let buf: [64]u8 = undefined;
        let n = mt.essence(buf[0..64]);
        expect_eq(buf[0..n], "text/html");
    });

    test("2. @cap/whatwg-mimetype — first duplicate parameter wins per WHATWG §4.4", () => {
        const mt = parseMimeType("text/plain; charset=utf-8; charset=latin1");
        expect_eq(mt.valid, true);
        expect_eq(mt.paramCount(), 1 as usize);
        expect_eq(mt.getParam("charset") ?? "", "utf-8");
    });

    test("3. @cap/whatwg-mimetype — quoted parameter values and serialize quoting", () => {
        const mt = parseMimeType("text/plain; title=\"hello world\"; mode=fast");
        expect_eq(mt.valid, true);
        expect_eq(mt.getParam("title") ?? "", "hello world");
        let buf: [128]u8 = undefined;
        let n = mt.serialize(buf[0..128]);
        expect_eq(buf[0..n], "text/plain;title=\"hello world\";mode=fast");
    });

    test("4. @cap/whatwg-mimetype — isXML and isJSON detect +xml / +json suffixes", () => {
        expect_eq(parseMimeType("image/svg+xml").isXML(), true);
        expect_eq(parseMimeType("application/xml").isXML(), true);
        expect_eq(parseMimeType("application/ld+json").isJSON(), true);
        expect_eq(parseMimeType("application/json").isJSON(), true);
        expect_eq(parseMimeType("text/plain").isJSON(), false);
    });

    test("5. @cap/whatwg-mimetype — isJavaScript recognizes standard & legacy JS types", () => {
        expect_eq(parseMimeType("text/javascript").isJavaScript(), true);
        expect_eq(parseMimeType("application/javascript").isJavaScript(), true);
        expect_eq(parseMimeType("text/ecmascript").isJavaScript(), true);
        expect_eq(parseMimeType("text/css").isJavaScript(), false);
    });

    test("6. @cap/whatwg-mimetype — rejects malformed inputs", () => {
        expect_eq(parseMimeType("").valid, false);
        expect_eq(parseMimeType("text").valid, false);
        expect_eq(parseMimeType("/html").valid, false);
        expect_eq(parseMimeType("text/").valid, false);
        expect_eq(parseMimeType("text/ht ml").valid, false);
    });
}
