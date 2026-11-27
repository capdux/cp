// cap/test/testCapMimeSniff.cp
// Comprehensive unit tests for @cap/mime-sniff

import { test, expect_eq } from "cap:test";
import {
    CT_BINARY,
    CT_TEXT,
    CT_HTML,
    CT_XML,
    CT_JSON,
    CT_SVG,
    isBinary,
    isText,
    sniffContentType,
    contentTypeToMime,
    sniffMime,
} from "../../packages/istextorbinary/src/index.cp";

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

fn main() {
    test("1. @cap/mime-sniff — Plain ASCII & UTF-8 Chinese classified as text", () => {
        const ascii = "Hello, world!\t\nLine 2\r\n";
        expect_eq(isBinary(ascii), false);
        expect_eq(isText(ascii), true);
        expect_eq(sniffContentType(ascii), CT_TEXT);

        const utf8 = "你好，CP 语言！\n";
        expect_eq(isBinary(utf8), false);
        expect_eq(isText(utf8), true);
        expect_eq(sniffContentType(utf8), CT_TEXT);
    });

    test("2. @cap/mime-sniff — NUL & disallowed control bytes classified as binary", () => {
        let withNul: [6]u8 = undefined;
        withNul[0] = 'A' as u8;
        withNul[1] = 'B' as u8;
        withNul[2] = 0x00 as u8;
        withNul[3] = 'C' as u8;
        withNul[4] = 'D' as u8;
        withNul[5] = 'E' as u8;
        expect_eq(isBinary(withNul[0..6]), true);
        expect_eq(isText(withNul[0..6]), false);
        expect_eq(sniffContentType(withNul[0..6]), CT_BINARY);

        let withCtrl: [3]u8 = undefined;
        withCtrl[0] = 'h' as u8;
        withCtrl[1] = 0x07 as u8; // BEL control byte
        withCtrl[2] = 'i' as u8;
        expect_eq(isBinary(withCtrl[0..3]), true);
    });

    test("3. @cap/mime-sniff — UTF-16 BOM classified as text despite NUL bytes", () => {
        let utf16le: [6]u8 = undefined;
        utf16le[0] = 0xFF as u8;
        utf16le[1] = 0xFE as u8;
        utf16le[2] = 'H' as u8;
        utf16le[3] = 0x00 as u8;
        utf16le[4] = 'i' as u8;
        utf16le[5] = 0x00 as u8;
        expect_eq(isBinary(utf16le[0..6]), false);
        expect_eq(isText(utf16le[0..6]), true);

        let utf16be: [4]u8 = undefined;
        utf16be[0] = 0xFE as u8;
        utf16be[1] = 0xFF as u8;
        utf16be[2] = 0x00 as u8;
        utf16be[3] = 'A' as u8;
        expect_eq(isBinary(utf16be[0..4]), false);
    });

    test("4. @cap/mime-sniff — HTML, SVG & XML sniffing with leading whitespace", () => {
        expect_eq(sniffContentType("  \n<!DOCTYPE html><html></html>"), CT_HTML);
        expect_eq(sniffContentType("<HTML lang=\"en\">"), CT_HTML);
        expect_eq(sniffContentType("  <script>alert(1)</script>"), CT_HTML);
        expect_eq(sniffContentType("\t<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>"), CT_SVG);
        expect_eq(sniffContentType("<?xml version=\"1.0\"?><rss></rss>"), CT_XML);
    });

    test("5. @cap/mime-sniff — JSON object & array sniffing", () => {
        expect_eq(sniffContentType("  {\"key\": 123}  "), CT_JSON);
        expect_eq(sniffContentType("\n[1, 2, 3]"), CT_JSON);
        expect_eq(sliceEq(sniffMime("{\"ok\":true}"), "application/json"), true);
    });

    test("6. @cap/mime-sniff — Empty slice & complete contentTypeToMime / sniffMime mapping", () => {
        expect_eq(isBinary(""), false);
        expect_eq(sniffContentType(""), CT_TEXT);
        expect_eq(sliceEq(sniffMime(""), "text/plain; charset=utf-8"), true);

        expect_eq(sliceEq(contentTypeToMime(CT_BINARY), "application/octet-stream"), true);
        expect_eq(sliceEq(contentTypeToMime(CT_TEXT), "text/plain; charset=utf-8"), true);
        expect_eq(sliceEq(contentTypeToMime(CT_HTML), "text/html; charset=utf-8"), true);
        expect_eq(sliceEq(contentTypeToMime(CT_XML), "application/xml"), true);
        expect_eq(sliceEq(contentTypeToMime(CT_JSON), "application/json"), true);
        expect_eq(sliceEq(contentTypeToMime(CT_SVG), "image/svg+xml"), true);
    });
}
