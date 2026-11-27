import { test, expect_eq } from "cap:test";
import {
    isAscii,
    firstNonAscii,
    isAsciiAlpha,
    isAsciiDigit,
    isAsciiAlphanumeric,
    isAsciiHexDigit,
    isAsciiOctDigit,
    isAsciiWhitespace,
    isAsciiPrintable,
    isAsciiControl,
    isAsciiLowercase,
    isAsciiUppercase,
    isAsciiGraph,
    isAsciiPunctuation,
    AsciiChar,
    AsciiStr,
    eqIgnoreAsciiCase,
    toAsciiLower,
    toAsciiUpper,
    toAsciiLowercase,
    toAsciiUppercase,
    makeAsciiLowercase,
    makeAsciiUppercase,
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

    test("2. @cap/ascii — full byte classification predicates", () => {
        expect_eq(isAsciiAlpha(65), true);
        expect_eq(isAsciiAlpha(122), true);
        expect_eq(isAsciiDigit(57), true);
        expect_eq(isAsciiOctDigit(55), true);
        expect_eq(isAsciiOctDigit(56), false);
        expect_eq(isAsciiAlphanumeric(48), true);
        expect_eq(isAsciiHexDigit(102), true);
        expect_eq(isAsciiHexDigit(103), false);
        expect_eq(isAsciiPrintable(32), true);
        expect_eq(isAsciiPrintable(126), true);
        expect_eq(isAsciiGraph(32), false);
        expect_eq(isAsciiGraph(33), true);
        expect_eq(isAsciiPunctuation('!'), true);
        expect_eq(isAsciiPunctuation('~'), true);
        expect_eq(isAsciiPunctuation('A'), false);
        expect_eq(isAsciiLowercase('z'), true);
        expect_eq(isAsciiLowercase('Z'), false);
        expect_eq(isAsciiUppercase('A'), true);
        expect_eq(isAsciiUppercase('a'), false);
        expect_eq(isAsciiWhitespace('\t'), true);
        expect_eq(isAsciiControl(0), true);
        expect_eq(isAsciiControl(127), true);
    });

    test("3. @cap/ascii — AsciiChar validation, classification & case conversion", () => {
        expect_eq(AsciiChar.fromByte(0x80) == null, true);
        const ch = AsciiChar.fromByte('a').?;
        expect_eq(ch.asByte(), 'a' as u8);
        expect_eq(ch.isAlphabetic(), true);
        expect_eq(ch.isAlphanumeric(), true);
        expect_eq(ch.isLowercase(), true);
        expect_eq(ch.isUppercase(), false);
        expect_eq(ch.isHexDigit(), true);
        expect_eq(ch.isGraph(), true);
        expect_eq(ch.isPunctuation(), false);
        expect_eq(ch.isControl(), false);
        expect_eq(ch.isWhitespace(), false);

        const up = ch.toAsciiUppercase();
        expect_eq(up.asByte(), 'A' as u8);
        expect_eq(up.isUppercase(), true);
        expect_eq(up.eqIgnoreAsciiCase(ch), true);
        expect_eq(up.toAsciiLowercase().asByte(), 'a' as u8);

        const punct = AsciiChar.fromByte('#').?;
        expect_eq(punct.isPunctuation(), true);
        expect_eq(punct.isGraph(), true);
        expect_eq(punct.isDigit(), false);
    });

    test("4. @cap/ascii — AsciiStr validation, slicing, trim & case conversion", () => {
        const bad: [3]u8 = [0x41, 0xFF, 0x42];
        expect_eq(AsciiStr.fromStr(bad[0..3] as string) == null, true);

        const s = AsciiStr.fromStr("  HeLLo-CP  ").?;
        expect_eq(s.len(), 12 as usize);
        expect_eq(s.isEmpty(), false);
        expect_eq(s.get(2).?.asByte(), 'H' as u8);
        expect_eq(s.get(99) == null, true);

        const trimmed = s.trim();
        expect_eq(trimmed.asStr(), "HeLLo-CP");
        expect_eq(s.trimStart().asStr(), "HeLLo-CP  ");
        expect_eq(s.trimEnd().asStr(), "  HeLLo-CP");

        const other = AsciiStr.fromStr("hello-cp").?;
        expect_eq(trimmed.eqIgnoreAsciiCase(other), true);

        let buf: [32]u8 = undefined;
        const n1 = trimmed.toAsciiLowercase(buf[0..32]);
        expect_eq(buf[0..n1] as string, "hello-cp");
        const n2 = trimmed.toAsciiUppercase(buf[0..32]);
        expect_eq(buf[0..n2] as string, "HELLO-CP");
    });

    test("5. @cap/ascii — eqIgnoreAsciiCase compares case-insensitively", () => {
        expect_eq(eqIgnoreAsciiCase("Content-Type", "content-type"), true);
        expect_eq(eqIgnoreAsciiCase("HTTP/1.1", "http/1.1"), true);
        expect_eq(eqIgnoreAsciiCase("abc", "abd"), false);
        expect_eq(eqIgnoreAsciiCase("abc", "abcd"), false);
    });

    test("6. @cap/ascii — toAsciiLower/Upper & in-place makeAsciiLowercase/Uppercase", () => {
        let buf: [64]u8 = undefined;
        const nl = toAsciiLower("HeLLo-123!", buf[0..64]);
        expect_eq(buf[0..nl] as string, "hello-123!");
        const nu = toAsciiUpper("HeLLo-123!", buf[0..64]);
        expect_eq(buf[0..nu] as string, "HELLO-123!");
        const nl2 = toAsciiLowercase("AbC", buf[0..64]);
        expect_eq(buf[0..nl2] as string, "abc");
        const nu2 = toAsciiUppercase("aBc", buf[0..64]);
        expect_eq(buf[0..nu2] as string, "ABC");

        let mut_buf: [5]u8 = ['H', 'e', 'L', 'l', 'O'];
        makeAsciiLowercase(mut_buf[0..5]);
        expect_eq(mut_buf[0..5] as string, "hello");
        makeAsciiUppercase(mut_buf[0..5]);
        expect_eq(mut_buf[0..5] as string, "HELLO");
    });

    test("7. @cap/ascii — trimAscii & empty slices safely", () => {
        expect_eq(trimAscii(" \t\r\nhello world\n\t "), "hello world");
        expect_eq(trimAscii("   "), "");
        expect_eq(trimAscii("clean"), "clean");
        expect_eq(isAscii(""), true);
        expect_eq(eqIgnoreAsciiCase("", ""), true);
        let buf: [8]u8 = undefined;
        expect_eq(toAsciiLower("", buf[0..8]), 0 as usize);
    });
}
