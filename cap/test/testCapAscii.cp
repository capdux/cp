import { test, expect_eq } from "cap:test";
import {
    isAscii,
    firstNonAscii,
    isAsciiAlpha,
    isAsciiDigit,
    isAsciiAlphanumeric,
    isAsciiHexDigit,
    isAsciiPrintable,
    isAsciiControl,
    eqIgnoreAsciiCase,
    toAsciiLower,
    toAsciiUpper,
    trimAscii,
} from "../../packages/ascii/src/index.cp";

fn main() {
    test("1. @cap/ascii — validates 7-bit ASCII and locates non-ASCII bytes", () => {
        expect_eq(isAscii("Hello, CP 123!"), true);
        expect_eq(firstNonAscii("Hello") == null, true);

        const non_ascii: [5]u8 = [0x61, 0x62, 0xC3, 0xA9, 0x63];
        expect_eq(isAscii(non_ascii[0..5]), false);
        expect_eq(firstNonAscii(non_ascii[0..5]) ?? 99, 2 as usize);
    });

    test("2. @cap/ascii — classifies alpha, digit, hex, printable, and control bytes", () => {
        expect_eq(isAsciiAlpha(65), true);
        expect_eq(isAsciiAlpha(122), true);
        expect_eq(isAsciiDigit(57), true);
        expect_eq(isAsciiAlphanumeric(48), true);
        expect_eq(isAsciiHexDigit(102), true);
        expect_eq(isAsciiHexDigit(103), false);
        expect_eq(isAsciiPrintable(32), true);
        expect_eq(isAsciiPrintable(126), true);
        expect_eq(isAsciiControl(0), true);
        expect_eq(isAsciiControl(127), true);
    });

    test("3. @cap/ascii — eqIgnoreAsciiCase compares case-insensitively", () => {
        expect_eq(eqIgnoreAsciiCase("Content-Type", "content-type"), true);
        expect_eq(eqIgnoreAsciiCase("HTTP/1.1", "http/1.1"), true);
        expect_eq(eqIgnoreAsciiCase("abc", "abd"), false);
        expect_eq(eqIgnoreAsciiCase("abc", "abcd"), false);
    });

    test("4. @cap/ascii — toAsciiLower and toAsciiUpper transform into output buffer", () => {
        let buf: [64]u8 = [0; 64];
        const nl = toAsciiLower("HeLLo-123!", buf[0..64]);
        expect_eq(buf[0..nl] as string, "hello-123!");
        const nu = toAsciiUpper("HeLLo-123!", buf[0..64]);
        expect_eq(buf[0..nu] as string, "HELLO-123!");
    });

    test("5. @cap/ascii — trimAscii strips leading and trailing ASCII whitespace", () => {
        expect_eq(trimAscii(" \t\r\nhello world\n\t "), "hello world");
        expect_eq(trimAscii("   "), "");
        expect_eq(trimAscii("clean"), "clean");
    });

    test("6. @cap/ascii — handles empty slices safely", () => {
        expect_eq(isAscii(""), true);
        expect_eq(eqIgnoreAsciiCase("", ""), true);
        let buf: [8]u8 = [0; 8];
        expect_eq(toAsciiLower("", buf[0..8]), 0 as usize);
    });
}
