import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    isFullwidthCodePoint,
    isZeroWidthCodePoint,
    codePointWidth,
    countFullwidthChars,
    hasFullwidthChar,
    stringDisplayWidth,
    truncateToDisplayWidth,
} from "../../packages/is-fullwidth-code-point/src/index.cp";

fn main() {
    test("1. @cap/is-fullwidth-code-point — ASCII and Latin are narrow", () => {
        expect_eq(isFullwidthCodePoint(0x41), false); // 'A'
        expect_eq(isFullwidthCodePoint(0x7A), false); // 'z'
        expect_eq(isFullwidthCodePoint(0x00E9), false); // 'é'
        expect_eq(isFullwidthCodePoint(0x303F), false); // IDEOGRAPHIC HALF FILL SPACE
    });

    test("2. @cap/is-fullwidth-code-point — CJK Unified Ideographs and Hiragana/Katakana", () => {
        expect_eq(isFullwidthCodePoint(0x4E2D), true); // '中'
        expect_eq(isFullwidthCodePoint(0x6587), true); // '文'
        expect_eq(isFullwidthCodePoint(0x3042), true); // 'あ'
        expect_eq(isFullwidthCodePoint(0x30A2), true); // 'ア'
    });

    test("3. @cap/is-fullwidth-code-point — Hangul and Fullwidth ASCII forms", () => {
        expect_eq(isFullwidthCodePoint(0xAC00), true); // '가'
        expect_eq(isFullwidthCodePoint(0x1100), true); // Hangul Jamo
        expect_eq(isFullwidthCodePoint(0xFF01), true); // '！'
        expect_eq(isFullwidthCodePoint(0xFFE5), true); // '￥'
    });

    test("4. @cap/is-fullwidth-code-point — Supplementary CJK Ext-B, Emoji & Extended-A", () => {
        expect_eq(isFullwidthCodePoint(0x20000), true); // CJK Ext B
        expect_eq(isFullwidthCodePoint(0x1F680), true); // 🚀
        expect_eq(isFullwidthCodePoint(0x1FA75), true); // Emoji Extended-A
        expect_eq(isFullwidthCodePoint(0x2329), true);  // 〈
        expect_eq(isFullwidthCodePoint(0x2615), true);  // ☕
        expect_eq(isFullwidthCodePoint(0x2705), true);  // ✅
    });

    test("5. @cap/is-fullwidth-code-point — isZeroWidthCodePoint & codePointWidth", () => {
        expect_eq(isZeroWidthCodePoint(0x00), true);
        expect_eq(isZeroWidthCodePoint(0x1B), true);
        expect_eq(isZeroWidthCodePoint(0x0301), true); // Combining acute accent
        expect_eq(isZeroWidthCodePoint(0x200D), true); // ZWJ
        expect_eq(isZeroWidthCodePoint(0xFE0F), true); // VS16
        expect_eq(isZeroWidthCodePoint(0x41), false);

        expect_eq(codePointWidth(0x00), 0 as usize);
        expect_eq(codePointWidth(0x0301), 0 as usize);
        expect_eq(codePointWidth(0x41), 1 as usize);
        expect_eq(codePointWidth(0x4E2D), 2 as usize);
    });

    test("6. @cap/is-fullwidth-code-point — countFullwidthChars & hasFullwidthChar", () => {
        expect_eq(countFullwidthChars("hello"), 0 as usize);
        expect_eq(hasFullwidthChar("hello"), false);
        expect_eq(countFullwidthChars("hello中文!"), 2 as usize);
        expect_eq(hasFullwidthChar("hello中文!"), true);
        expect_eq(countFullwidthChars("CP语言🚀"), 3 as usize);
    });

    test("7. @cap/is-fullwidth-code-point — stringDisplayWidth computes terminal columns", () => {
        expect_eq(stringDisplayWidth(""), 0 as usize);
        expect_eq(stringDisplayWidth("hello"), 5 as usize);
        expect_eq(stringDisplayWidth("hello中文!"), 10 as usize); // 5 + 4 + 1
        expect_eq(stringDisplayWidth("CP语言🚀"), 8 as usize);    // 2 + 4 + 2
        expect_eq(stringDisplayWidth("a\nb\t"), 2 as usize);       // control chars = 0
    });

    test("8. @cap/is-fullwidth-code-point — truncateToDisplayWidth never splits UTF-8 or overflows", () => {
        expect_eq(strEq(truncateToDisplayWidth("hello中文!", 7), "hello中"), true); // width 7
        expect_eq(strEq(truncateToDisplayWidth("hello中文!", 6), "hello"), true);  // '中' needs 2, so stops at 5
        expect_eq(strEq(truncateToDisplayWidth("hello", 10), "hello"), true);
        expect_eq(strEq(truncateToDisplayWidth("中文", 1), ""), true);
    });
}
