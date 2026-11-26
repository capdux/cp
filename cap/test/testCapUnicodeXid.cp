// cap/test/testCapUnicodeXid.cp
// Unit tests for @cap/unicode-xid (UAX #31 Unicode Identifier classification & scanner)

import { test, expect_eq } from "cap:test";
import {
    isXidStart,
    isXidContinue,
    isIdentStart,
    isValidIdent,
    scanIdentLen,
    isAsciiIdent,
} from "../../packages/@cap/unicode-xid/src/index.cp";

fn main() {
    test("1. @cap/unicode-xid — ASCII XID_Start / XID_Continue / IdentStart fast paths", () => {
        expect_eq(isXidStart('A' as u32), true);
        expect_eq(isXidStart('z' as u32), true);
        expect_eq(isXidStart('_' as u32), false);
        expect_eq(isXidStart('0' as u32), false);
        expect_eq(isXidStart('-' as u32), false);

        expect_eq(isIdentStart('_' as u32), true);
        expect_eq(isIdentStart('a' as u32), true);
        expect_eq(isIdentStart('9' as u32), false);

        expect_eq(isXidContinue('0' as u32), true);
        expect_eq(isXidContinue('_' as u32), true);
        expect_eq(isXidContinue('Z' as u32), true);
        expect_eq(isXidContinue(' ' as u32), false);
        expect_eq(isXidContinue('$' as u32), false);
    });

    test("2. @cap/unicode-xid — Unicode multi-script XID_Start coverage", () => {
        expect_eq(isXidStart(0x00E9), true);  // Latin 'é'
        expect_eq(isXidStart(0x03B1), true);  // Greek 'α'
        expect_eq(isXidStart(0x0416), true);  // Cyrillic 'Ж'
        expect_eq(isXidStart(0x0531), true);  // Armenian 'Ա'
        expect_eq(isXidStart(0x05D0), true);  // Hebrew 'א'
        expect_eq(isXidStart(0x0627), true);  // Arabic 'ا'
        expect_eq(isXidStart(0x0905), true);  // Devanagari 'अ'
        expect_eq(isXidStart(0x3042), true);  // Hiragana 'あ'
        expect_eq(isXidStart(0x30A2), true);  // Katakana 'ア'
        expect_eq(isXidStart(0xAC00), true);  // Hangul '가'
        expect_eq(isXidStart(0x4E2D), true);  // CJK '中'
        expect_eq(isXidStart(0x3400), true);  // CJK Ext-A
        expect_eq(isXidStart(0x1F600), false); // Emoji 😀 is not XID_Start
        expect_eq(isXidStart(0x00D7), false); // Multiplication sign ×
    });

    test("3. @cap/unicode-xid — combining marks, Unicode digits & connectors in XID_Continue", () => {
        expect_eq(isXidStart(0x0301), false);    // Combining acute accent
        expect_eq(isXidContinue(0x0301), true);
        expect_eq(isXidStart(0x0660), false);    // Arabic-Indic digit 0
        expect_eq(isXidContinue(0x0660), true);
        expect_eq(isXidStart(0x0966), false);    // Devanagari digit 0
        expect_eq(isXidContinue(0x0966), true);
        expect_eq(isXidContinue(0x203F), true);  // Undertie connector
        expect_eq(isXidContinue(0xFF15), true);  // Fullwidth digit '５'
        expect_eq(isXidContinue(0xFF3F), true);  // Fullwidth low line '＿'
        expect_eq(isXidContinue(0x3002), false); // Ideographic full stop '。'
    });

    test("4. @cap/unicode-xid — isValidIdent valid & rejected inputs (incl. invalid UTF-8)", () => {
        expect_eq(isValidIdent("_private_123"), true);
        expect_eq(isValidIdent("αβγ_1"), true);
        expect_eq(isValidIdent("中文变量99"), true);
        expect_eq(isValidIdent("cafe\xCC\x81"), true); // e + U+0301 combining mark
        expect_eq(isValidIdent("한글_変数"), true);

        expect_eq(isValidIdent(""), false);
        expect_eq(isValidIdent("123abc"), false);
        expect_eq(isValidIdent("foo-bar"), false);
        expect_eq(isValidIdent("foo bar"), false);
        expect_eq(isValidIdent("hi😀"), false);
        // Invalid UTF-8 byte sequence
        expect_eq(isValidIdent("abc\xFFdef"), false);
        expect_eq(isValidIdent("\xC0\xAF"), false);
    });

    test("5. @cap/unicode-xid — scanIdentLen lexer scanning at arbitrary offsets", () => {
        const src = "let 变量_1 + αβ2 = 42;";
        expect_eq(scanIdentLen(src, 0), 3 as usize);  // "let"
        expect_eq(scanIdentLen(src, 3), 0 as usize);  // space -> 0
        expect_eq(scanIdentLen(src, 4), 8 as usize);  // "变量_1" (3+3+1+1 = 8 bytes)
        expect_eq(scanIdentLen(src, 13), 0 as usize); // '+' -> 0
        expect_eq(scanIdentLen(src, 15), 5 as usize); // "αβ2" (2+2+1 = 5 bytes)
        expect_eq(scanIdentLen(src, 23), 0 as usize); // '4' (digit) -> 0
        expect_eq(scanIdentLen(src, 99), 0 as usize); // out of bounds -> 0
    });

    test("6. @cap/unicode-xid — isAsciiIdent fast-path validation", () => {
        expect_eq(isAsciiIdent("foo"), true);
        expect_eq(isAsciiIdent("_bar_99"), true);
        expect_eq(isAsciiIdent("_"), true);
        expect_eq(isAsciiIdent(""), false);
        expect_eq(isAsciiIdent("0abc"), false);
        expect_eq(isAsciiIdent("foo.bar"), false);
        expect_eq(isAsciiIdent("αβ"), false);
        expect_eq(isAsciiIdent("中文"), false);
    });
}
