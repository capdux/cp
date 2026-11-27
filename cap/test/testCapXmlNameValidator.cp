import { test, expect_eq } from "cap:test";
import {
    isXmlChar,
    isNameStartChar,
    isNameChar,
    isValidXmlName,
    isValidNCName,
    isValidQName,
} from "../../packages/xml-name-validator/src/index.cp";

fn main() {
    test("1. @cap/xml-name-validator — isXmlChar boundaries (XML 1.0 §2.2)", () => {
        // Allowed control chars & ranges
        expect_eq(isXmlChar(0x09), true);
        expect_eq(isXmlChar(0x0A), true);
        expect_eq(isXmlChar(0x0D), true);
        expect_eq(isXmlChar(0x20), true);
        expect_eq(isXmlChar(0xD7FF), true);
        expect_eq(isXmlChar(0xE000), true);
        expect_eq(isXmlChar(0xFFFD), true);
        expect_eq(isXmlChar(0x10000), true);
        expect_eq(isXmlChar(0x10FFFF), true);

        // Forbidden NUL, C0 controls, surrogates, FFFE/FFFF, out-of-range
        expect_eq(isXmlChar(0x00), false);
        expect_eq(isXmlChar(0x08), false);
        expect_eq(isXmlChar(0x0B), false);
        expect_eq(isXmlChar(0x0C), false);
        expect_eq(isXmlChar(0x1F), false);
        expect_eq(isXmlChar(0xD800), false);
        expect_eq(isXmlChar(0xDFFF), false);
        expect_eq(isXmlChar(0xFFFE), false);
        expect_eq(isXmlChar(0xFFFF), false);
        expect_eq(isXmlChar(0x110000), false);
    });

    test("2. @cap/xml-name-validator — isNameStartChar & isNameChar W3C §2.3", () => {
        expect_eq(isNameStartChar(0x3A), true); // ':'
        expect_eq(isNameStartChar(0x5F), true); // '_'
        expect_eq(isNameStartChar(0x41), true); // 'A'
        expect_eq(isNameStartChar(0x7A), true); // 'z'
        expect_eq(isNameStartChar(0xC0), true);
        expect_eq(isNameStartChar(0xD7), false); // multiplication sign excluded
        expect_eq(isNameStartChar(0xF7), false); // division sign excluded
        expect_eq(isNameStartChar(0x37E), false); // Greek question mark excluded
        expect_eq(isNameStartChar(0x200C), true);
        expect_eq(isNameStartChar(0x4E2D), true); // '中'
        expect_eq(isNameStartChar(0xEFFFF), true);
        expect_eq(isNameStartChar(0xF0000), false); // beyond #xEFFFF

        // Digits, '-', '.', middle dot, combining marks are NameChar but not NameStartChar
        expect_eq(isNameStartChar(0x30), false);
        expect_eq(isNameStartChar(0x2D), false);
        expect_eq(isNameStartChar(0x2E), false);
        expect_eq(isNameStartChar(0xB7), false);
        expect_eq(isNameChar(0x30), true);
        expect_eq(isNameChar(0x2D), true);
        expect_eq(isNameChar(0x2E), true);
        expect_eq(isNameChar(0xB7), true);
        expect_eq(isNameChar(0x0300), true);
        expect_eq(isNameChar(0x203F), true);
        expect_eq(isNameChar(0x20), false);
    });

    test("3. @cap/xml-name-validator — isValidXmlName ASCII & invalid cases", () => {
        expect_eq(isValidXmlName("x"), true);
        expect_eq(isValidXmlName("_foo-bar.123"), true);
        expect_eq(isValidXmlName("svg:rect"), true);
        expect_eq(isValidXmlName(":leading"), true);
        expect_eq(isValidXmlName("a:b:c"), true);

        expect_eq(isValidXmlName(""), false);
        expect_eq(isValidXmlName("123abc"), false);
        expect_eq(isValidXmlName("-foo"), false);
        expect_eq(isValidXmlName(".bar"), false);
        expect_eq(isValidXmlName("foo bar"), false);
        expect_eq(isValidXmlName("foo/bar"), false);
    });

    test("4. @cap/xml-name-validator — isValidXmlName Unicode & malformed UTF-8", () => {
        expect_eq(isValidXmlName("中文标签"), true);
        expect_eq(isValidXmlName("élement·1"), true);
        // 4-byte supplementary plane char U+10000 (F0 90 80 80) is valid NameStartChar
        const supp: [4]u8 = [0xF0, 0x90, 0x80, 0x80];
        expect_eq(isValidXmlName(supp[0..4]), true);

        // Invalid UTF-8: lone continuation byte & overlong 2-byte sequence
        const bad1: [2]u8 = [0x61, 0x80];
        const bad2: [2]u8 = [0xC0, 0xAF];
        expect_eq(isValidXmlName(bad1[0..2]), false);
        expect_eq(isValidXmlName(bad2[0..2]), false);
    });

    test("5. @cap/xml-name-validator — isValidNCName rejects colons", () => {
        expect_eq(isValidNCName("div"), true);
        expect_eq(isValidNCName("_my-tag.v2"), true);
        expect_eq(isValidNCName("中文_1"), true);

        expect_eq(isValidNCName(""), false);
        expect_eq(isValidNCName("svg:rect"), false);
        expect_eq(isValidNCName(":foo"), false);
        expect_eq(isValidNCName("foo:"), false);
        expect_eq(isValidNCName("9tag"), false);
    });

    test("6. @cap/xml-name-validator — isValidQName accepts NCName / NCName:NCName only", () => {
        expect_eq(isValidQName("div"), true);
        expect_eq(isValidQName("svg:rect"), true);
        expect_eq(isValidQName("xlink:href"), true);
        expect_eq(isValidQName("ns1:标签-2"), true);

        expect_eq(isValidQName(""), false);
        expect_eq(isValidQName(":"), false);
        expect_eq(isValidQName(":foo"), false);
        expect_eq(isValidQName("foo:"), false);
        expect_eq(isValidQName("a:b:c"), false);
        expect_eq(isValidQName("a:1b"), false);
        expect_eq(isValidQName("1a:b"), false);
    });
}
