// cap/test/testUnicodeAscii.cp
// Production-grade deep tests for cap:unicode ascii.cp
// (Unicode u21 ASCII codepoint predicates, branchless case folding, control codes & zero-alloc slice helpers)

import { test, assert, assertEq } from "cap:test";
import {
    AsciiControlCode,
    isAsciiCodepoint,
    isAsciiAlphanumeric,
    isAsciiAlphabetic,
    isAsciiControl,
    isAsciiDigit,
    isAsciiLower,
    isAsciiUpper,
    isAsciiPrint,
    isAsciiGraph,
    isAsciiPunct,
    isAsciiWhitespace,
    isAsciiHex,
    asciiToUpper,
    asciiToLower,
    asciiDigitValue,
    asciiHexValue,
    isAllAscii,
    asciiEqlIgnoreCase,
    asciiStartsWithIgnoreCase,
    asciiEndsWithIgnoreCase,
    asciiToLowerBuf,
    asciiToUpperBuf,
    asciiTrimWhitespace,
    AsciiStats,
    scanAsciiStats,
} from "cap:unicode";

test("Unicode ASCII predicates: full 0..=127 table & non-ASCII boundary rejection", () => {
    let cp: u21 = 0;
    let count_ascii: usize = 0;
    let count_alpha: usize = 0;
    let count_digit: usize = 0;
    let count_alnum: usize = 0;
    let count_lower: usize = 0;
    let count_upper: usize = 0;
    let count_ctrl: usize = 0;
    let count_print: usize = 0;
    let count_graph: usize = 0;
    let count_punct: usize = 0;
    let count_ws: usize = 0;
    let count_hex: usize = 0;

    while (cp <= 127) {
        if (isAsciiCodepoint(cp)) count_ascii += 1;
        if (isAsciiAlphabetic(cp)) count_alpha += 1;
        if (isAsciiDigit(cp)) count_digit += 1;
        if (isAsciiAlphanumeric(cp)) count_alnum += 1;
        if (isAsciiLower(cp)) count_lower += 1;
        if (isAsciiUpper(cp)) count_upper += 1;
        if (isAsciiControl(cp)) count_ctrl += 1;
        if (isAsciiPrint(cp)) count_print += 1;
        if (isAsciiGraph(cp)) count_graph += 1;
        if (isAsciiPunct(cp)) count_punct += 1;
        if (isAsciiWhitespace(cp)) count_ws += 1;
        if (isAsciiHex(cp)) count_hex += 1;
        cp += 1;
    }

    assertEq(count_ascii, 128);
    assertEq(count_alpha, 52);
    assertEq(count_digit, 10);
    assertEq(count_alnum, 62);
    assertEq(count_lower, 26);
    assertEq(count_upper, 26);
    assertEq(count_ctrl, 33); // 0x00..=0x1F (32) + 0x7F (1)
    assertEq(count_print, 95); // 0x20..=0x7E
    assertEq(count_graph, 94); // 0x21..=0x7E
    assertEq(count_punct, 32); // 94 - 62
    assertEq(count_ws, 6); // space + \t \n \v \f \r
    assertEq(count_hex, 22); // 10 + 6 + 6

    // Control code constants spot checks
    assert(isAsciiControl(AsciiControlCode.NUL));
    assert(isAsciiControl(AsciiControlCode.ESC));
    assert(isAsciiControl(AsciiControlCode.US));
    assert(isAsciiControl(AsciiControlCode.DEL));
    assert(!isAsciiControl(' '));
    assert(isAsciiPrint(' '));
    assert(!isAsciiGraph(' '));

    // Non-ASCII codepoints must return false for all ASCII predicates
    const non_ascii: [6]u21 = [0x80, 0xA0, 0x00E9, 0x4E2D, 0x1F600, 0x10FFFF];
    let i: usize = 0;
    while (i < non_ascii.len) {
        const c = non_ascii[i];
        assert(!isAsciiCodepoint(c));
        assert(!isAsciiAlphanumeric(c));
        assert(!isAsciiAlphabetic(c));
        assert(!isAsciiControl(c));
        assert(!isAsciiDigit(c));
        assert(!isAsciiLower(c));
        assert(!isAsciiUpper(c));
        assert(!isAsciiPrint(c));
        assert(!isAsciiGraph(c));
        assert(!isAsciiPunct(c));
        assert(!isAsciiWhitespace(c));
        assert(!isAsciiHex(c));
        i += 1;
    }
});

test("Branchless asciiToUpper & asciiToLower: roundtrip and non-letter invariance", () => {
    let cp: u21 = 0;
    while (cp <= 127) {
        if (cp >= 'a' && cp <= 'z') {
            assertEq(asciiToUpper(cp), cp - 32);
            assertEq(asciiToLower(cp), cp);
            assertEq(asciiToLower(asciiToUpper(cp)), cp);
        } else if (cp >= 'A' && cp <= 'Z') {
            assertEq(asciiToLower(cp), cp + 32);
            assertEq(asciiToUpper(cp), cp);
            assertEq(asciiToUpper(asciiToLower(cp)), cp);
        } else {
            assertEq(asciiToUpper(cp), cp);
            assertEq(asciiToLower(cp), cp);
        }
        cp += 1;
    }

    // Non-ASCII codepoints must never be altered by bitmasking
    assertEq(asciiToUpper(0x00E9), 0x00E9);
    assertEq(asciiToLower(0x00C9), 0x00C9);
    assertEq(asciiToUpper(0x1F680), 0x1F680);
    assertEq(asciiToLower(0x10FFFF), 0x10FFFF);
});

test("asciiDigitValue & asciiHexValue: valid digits and boundary rejections", () => {
    if (d0 := asciiDigitValue('0')) assertEq(d0, 0) else assert(false);
    if (d9 := asciiDigitValue('9')) assertEq(d9, 9) else assert(false);
    assert(asciiDigitValue('/') == null);
    assert(asciiDigitValue(':') == null);
    assert(asciiDigitValue('a') == null);
    assert(asciiDigitValue(0xFF10) == null); // fullwidth '0'

    if (h0 := asciiHexValue('0')) assertEq(h0, 0) else assert(false);
    if (ha := asciiHexValue('a')) assertEq(ha, 10) else assert(false);
    if (hf := asciiHexValue('f')) assertEq(hf, 15) else assert(false);
    if (hA := asciiHexValue('A')) assertEq(hA, 10) else assert(false);
    if (hF := asciiHexValue('F')) assertEq(hF, 15) else assert(false);
    assert(asciiHexValue('g') == null);
    assert(asciiHexValue('G') == null);
    assert(asciiHexValue('@') == null);
    assert(asciiHexValue('`') == null);
});

test("Case-insensitive slice comparison, prefix and suffix matching", () => {
    assert(asciiEqlIgnoreCase("Content-Type", "content-type"));
    assert(asciiEqlIgnoreCase("HTTP/1.1", "http/1.1"));
    assert(asciiEqlIgnoreCase("", ""));
    assert(!asciiEqlIgnoreCase("abc", "abcd"));
    assert(!asciiEqlIgnoreCase("abc", "abd"));

    assert(asciiStartsWithIgnoreCase("Authorization: Bearer xyz", "authorization:"));
    assert(asciiStartsWithIgnoreCase("hello", ""));
    assert(!asciiStartsWithIgnoreCase("hi", "hello"));

    assert(asciiEndsWithIgnoreCase("index.HTML", ".html"));
    assert(asciiEndsWithIgnoreCase("archive.TAR.GZ", ".tar.gz"));
    assert(asciiEndsWithIgnoreCase("abc", ""));
    assert(!asciiEndsWithIgnoreCase("ab", "abc"));
});

test("Zero-alloc asciiToLowerBuf, asciiToUpperBuf truncation guard & asciiTrimWhitespace", () => {
    let buf: [16]u8 = undefined;
    const src = "Hello, CP-2026!";

    const n_lo = asciiToLowerBuf(src, buf[0..]);
    assertEq(n_lo, src.len);
    assert(asciiEqlIgnoreCase(buf[0..n_lo], "hello, cp-2026!"));
    assertEq(buf[0], 'h');
    assertEq(buf[7], 'c');
    assertEq(buf[8], 'p');

    const n_hi = asciiToUpperBuf(src, buf[0..]);
    assertEq(n_hi, src.len);
    assertEq(buf[0], 'H');
    assertEq(buf[1], 'E');
    assertEq(buf[4], 'O');

    // Short destination buffer must safely truncate without out-of-bounds write
    let small: [4]u8 = undefined;
    const n_small = asciiToLowerBuf("ABCDEFGH", small[0..]);
    assertEq(n_small, 4);
    assertEq(small[0], 'a');
    assertEq(small[3], 'd');

    // Zero-length buffer guard
    let empty_buf: [0]u8 = undefined;
    assertEq(asciiToUpperBuf("abc", empty_buf[0..]), 0);

    // Zero-copy whitespace trimming
    const trimmed = asciiTrimWhitespace(" \t\r\n  hello world \x0B\x0C\n ");
    assertEq(trimmed.len, 11);
    assert(asciiEqlIgnoreCase(trimmed, "HELLO WORLD"));

    const all_ws = asciiTrimWhitespace(" \t\r\n");
    assertEq(all_ws.len, 0);

    const empty_trim = asciiTrimWhitespace("");
    assertEq(empty_trim.len, 0);
});

test("isAllAscii & scanAsciiStats: pure ASCII vs mixed UTF-8 & control bytes", () => {
    assert(isAllAscii(""));
    assert(isAllAscii("Hello, World! 123\r\n"));
    assert(!isAllAscii("Hello 世界"));

    const pure = scanAsciiStats("Abc 123!\n");
    assertEq(pure.total, 9);
    assertEq(pure.ascii, 9);
    assertEq(pure.non_ascii, 0);
    assertEq(pure.letters, 3);
    assertEq(pure.digits, 3);
    assertEq(pure.whitespace, 2); // ' ' and '\n'
    assertEq(pure.control, 1); // '\n'
    assertEq(pure.printable, 8);
    assert(pure.isPureAscii());

    const mixed = scanAsciiStats("A中9");
    assertEq(mixed.total, 5); // 'A' (1) + '中' (3 UTF-8 bytes) + '9' (1)
    assertEq(mixed.ascii, 2);
    assertEq(mixed.non_ascii, 3);
    assertEq(mixed.letters, 1);
    assertEq(mixed.digits, 1);
    assert(!mixed.isPureAscii());
});
