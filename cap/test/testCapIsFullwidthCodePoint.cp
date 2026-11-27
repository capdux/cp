import { test, expect_eq } from "cap:test";
import {
    isFullwidthCodePoint,
    codePointWidth,
    countFullwidthChars,
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

    test("4. @cap/is-fullwidth-code-point — Supplementary CJK Ext-B and Emoji", () => {
        expect_eq(isFullwidthCodePoint(0x20000), true); // CJK Ext B
        expect_eq(isFullwidthCodePoint(0x1F680), true); // 🚀
        expect_eq(isFullwidthCodePoint(0x2329), true);  // 〈
    });

    test("5. @cap/is-fullwidth-code-point — codePointWidth handles control, narrow, wide", () => {
        expect_eq(codePointWidth(0x00), 0 as usize);
        expect_eq(codePointWidth(0x1B), 0 as usize);
        expect_eq(codePointWidth(0x41), 1 as usize);
        expect_eq(codePointWidth(0x4E2D), 2 as usize);
    });

    test("6. @cap/is-fullwidth-code-point — countFullwidthChars scans UTF-8 strings", () => {
        expect_eq(countFullwidthChars("hello"), 0 as usize);
        expect_eq(countFullwidthChars("hello中文!"), 2 as usize);
        expect_eq(countFullwidthChars("CP语言🚀"), 3 as usize);
    });
}
