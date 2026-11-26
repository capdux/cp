import { test, expect_eq } from "cap:test";
import {
    isDecimalDigit,
    isHexDigit,
    isOctalDigit,
    isWhiteSpace,
    isLineTerminator,
    isKeywordES6,
    isReservedWordES6,
    isRestrictedWord,
    isIdentifierNameES6,
    isIdentifierES6,
} from "../../packages/@cap/esutils/src/index.cp";

fn main() {
    test("1. @cap/esutils — digit helpers (decimal, hex, octal)", () => {
        expect_eq(isDecimalDigit(0x30), true);
        expect_eq(isDecimalDigit(0x39), true);
        expect_eq(isDecimalDigit(0x41), false);
        expect_eq(isHexDigit(0x46), true); // 'F'
        expect_eq(isHexDigit(0x66), true); // 'f'
        expect_eq(isHexDigit(0x67), false); // 'g'
        expect_eq(isOctalDigit(0x37), true);
        expect_eq(isOctalDigit(0x38), false);
    });

    test("2. @cap/esutils — WhiteSpace and LineTerminator", () => {
        expect_eq(isWhiteSpace(0x20), true);
        expect_eq(isWhiteSpace(0x09), true);
        expect_eq(isWhiteSpace(0xFEFF), true);
        expect_eq(isWhiteSpace(0x3000), true);
        expect_eq(isWhiteSpace(0x0A), false);
        expect_eq(isLineTerminator(0x0A), true);
        expect_eq(isLineTerminator(0x0D), true);
        expect_eq(isLineTerminator(0x2028), true);
        expect_eq(isLineTerminator(0x2029), true);
    });

    test("3. @cap/esutils — ES6 keywords in sloppy vs strict mode", () => {
        expect_eq(isKeywordES6("const", false), true);
        expect_eq(isKeywordES6("class", false), true);
        expect_eq(isKeywordES6("let", false), false);
        expect_eq(isKeywordES6("let", true), true);
        expect_eq(isKeywordES6("interface", true), true);
        expect_eq(isKeywordES6("foo", true), false);
    });

    test("4. @cap/esutils — reserved words and restricted words", () => {
        expect_eq(isReservedWordES6("null", false), true);
        expect_eq(isReservedWordES6("true", false), true);
        expect_eq(isReservedWordES6("false", false), true);
        expect_eq(isReservedWordES6("return", false), true);
        expect_eq(isRestrictedWord("eval"), true);
        expect_eq(isRestrictedWord("arguments"), true);
        expect_eq(isRestrictedWord("value"), false);
    });

    test("5. @cap/esutils — isIdentifierNameES6 vs isIdentifierES6", () => {
        expect_eq(isIdentifierNameES6("class"), true);
        expect_eq(isIdentifierES6("class", false), false);
        expect_eq(isIdentifierES6("$foo_123", true), true);
        expect_eq(isIdentifierES6("123abc", false), false);
        expect_eq(isIdentifierES6("", false), false);
    });

    test("6. @cap/esutils — supports Unicode identifiers", () => {
        expect_eq(isIdentifierES6("变量1", true), true);
        expect_eq(isIdentifierES6("π_val", true), true);
        expect_eq(isIdentifierNameES6("café"), true);
    });
}
