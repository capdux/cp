import { test, expect_eq } from "cap:test";
import {
    isAscii,
    firstNonAscii,
    isSpace,
    isAlpha,
    isDigit,
    isAlnum,
    isHexDigit,
    isPrintable,
    isControl,
    hexVal,
    hexCharLower,
    hexCharUpper,
    toLowerByte,
    toUpperByte,
    strEq,
    eqIgnoreCase,
    startsWith,
    endsWith,
    findByte,
    rfindByte,
    trim,
    trimStart,
    trimEnd,
    toLower,
    toUpper,
} from "cap:ascii";

fn main() {
    test("cap:ascii - isAscii and firstNonAscii", () => {
        expect_eq(isAscii(""), true);
        expect_eq(isAscii("Hello, CP 2026! ~0123456789"), true);
        expect_eq(firstNonAscii("Hello, CP 2026!") == null, true);

        const non_ascii: [10]u8 = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0x80, 'z'];
        expect_eq(isAscii(non_ascii[0..]), false);
        if (idx := firstNonAscii(non_ascii[0..])) {
            expect_eq(idx, 8);
        } else {
            expect_eq(false, true);
        }

        const early_non: [5]u8 = ['x', 0xFF, 'y', 'z', '0'];
        expect_eq(isAscii(early_non[0..]), false);
        if (idx2 := firstNonAscii(early_non[0..])) {
            expect_eq(idx2, 1);
        } else {
            expect_eq(false, true);
        }
    });

    test("cap:ascii - byte classification predicates", () => {
        expect_eq(isSpace(' '), true);
        expect_eq(isSpace('\t'), true);
        expect_eq(isSpace('\n'), true);
        expect_eq(isSpace('\r'), true);
        expect_eq(isSpace(0x0B), true);
        expect_eq(isSpace(0x0C), true);
        expect_eq(isSpace('a'), false);

        expect_eq(isAlpha('A'), true);
        expect_eq(isAlpha('Z'), true);
        expect_eq(isAlpha('a'), true);
        expect_eq(isAlpha('z'), true);
        expect_eq(isAlpha('0'), false);
        expect_eq(isAlpha('_'), false);

        expect_eq(isDigit('0'), true);
        expect_eq(isDigit('9'), true);
        expect_eq(isDigit('a'), false);

        expect_eq(isAlnum('7'), true);
        expect_eq(isAlnum('Q'), true);
        expect_eq(isAlnum('-'), false);

        expect_eq(isHexDigit('0'), true);
        expect_eq(isHexDigit('f'), true);
        expect_eq(isHexDigit('F'), true);
        expect_eq(isHexDigit('g'), false);

        expect_eq(isPrintable(0x20), true);
        expect_eq(isPrintable(0x7E), true);
        expect_eq(isPrintable(0x1F), false);
        expect_eq(isPrintable(0x7F), false);

        expect_eq(isControl(0x00), true);
        expect_eq(isControl(0x1F), true);
        expect_eq(isControl(0x7F), true);
        expect_eq(isControl(0x20), false);
        expect_eq(isControl(0x80), false);
    });

    test("cap:ascii - hexVal and hexCharLower/Upper", () => {
        expect_eq(hexVal('0').?, 0);
        expect_eq(hexVal('9').?, 9);
        expect_eq(hexVal('a').?, 10);
        expect_eq(hexVal('f').?, 15);
        expect_eq(hexVal('A').?, 10);
        expect_eq(hexVal('F').?, 15);
        expect_eq(hexVal('G') == null, true);
        expect_eq(hexVal('/') == null, true);

        expect_eq(hexCharLower(0), '0');
        expect_eq(hexCharLower(9), '9');
        expect_eq(hexCharLower(10), 'a');
        expect_eq(hexCharLower(15), 'f');
        expect_eq(hexCharLower(0xFA), 'a');

        expect_eq(hexCharUpper(0), '0');
        expect_eq(hexCharUpper(10), 'A');
        expect_eq(hexCharUpper(15), 'F');
        expect_eq(hexCharUpper(0x3D), 'D');
    });

    test("cap:ascii - strEq, eqIgnoreCase, startsWith, endsWith", () => {
        expect_eq(strEq("alpha", "alpha"), true);
        expect_eq(strEq("alpha", "Alpha"), false);
        expect_eq(strEq("alpha", "alph"), false);

        expect_eq(eqIgnoreCase("Content-Type", "content-type"), true);
        expect_eq(eqIgnoreCase("HTTP/1.1", "http/1.1"), true);
        expect_eq(eqIgnoreCase("abc", "abd"), false);
        expect_eq(eqIgnoreCase("abc", "ab"), false);

        expect_eq(startsWith("cap:ascii", "cap:"), true);
        expect_eq(startsWith("cap:ascii", ""), true);
        expect_eq(startsWith("cap", "cap:ascii"), false);
        expect_eq(startsWith("cap:ascii", "CAP:"), false);

        expect_eq(endsWith("index.cp", ".cp"), true);
        expect_eq(endsWith("index.cp", ""), true);
        expect_eq(endsWith(".cp", "index.cp"), false);
        expect_eq(endsWith("index.cp", ".rs"), false);
    });

    test("cap:ascii - findByte, rfindByte, trim/trimStart/trimEnd", () => {
        expect_eq(findByte("a/b/c/d", '/', 0).?, 1);
        expect_eq(findByte("a/b/c/d", '/', 2).?, 3);
        expect_eq(findByte("a/b/c/d", '/', 6) == null, true);
        expect_eq(findByte("a/b/c/d", '/', 99) == null, true);

        expect_eq(rfindByte("a/b/c/d", '/').?, 5);
        expect_eq(rfindByte("abcdef", '/') == null, true);
        expect_eq(rfindByte("", 'a') == null, true);

        expect_eq(strEq(trimStart(" \t\r\nhello  "), "hello  "), true);
        expect_eq(strEq(trimEnd("  hello\t\n "), "  hello"), true);
        expect_eq(strEq(trim(" \t\n hello world \r\n "), "hello world"), true);
        expect_eq(strEq(trim("   \t\r\n  "), ""), true);
        expect_eq(strEq(trim(""), ""), true);
    });

    test("cap:ascii - toLowerByte, toUpperByte, toLower, toUpper", () => {
        expect_eq(toLowerByte('A'), 'a');
        expect_eq(toLowerByte('Z'), 'z');
        expect_eq(toLowerByte('a'), 'a');
        expect_eq(toLowerByte('5'), '5');

        expect_eq(toUpperByte('a'), 'A');
        expect_eq(toUpperByte('z'), 'Z');
        expect_eq(toUpperByte('Z'), 'Z');
        expect_eq(toUpperByte('!'), '!');

        let buf: [32]u8 = undefined;
        const n1 = toLower("HeLLo-123_CP!", buf[0..]);
        expect_eq(n1, 13);
        expect_eq(strEq(buf[0..n1], "hello-123_cp!"), true);

        const n2 = toUpper("HeLLo-123_CP!", buf[0..]);
        expect_eq(n2, 13);
        expect_eq(strEq(buf[0..n2], "HELLO-123_CP!"), true);

        let small: [4]u8 = undefined;
        const n3 = toLower("ABCDEF", small[0..]);
        expect_eq(n3, 4);
        expect_eq(strEq(small[0..n3], "abcd"), true);
    });
}
