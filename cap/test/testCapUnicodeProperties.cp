// cap/test/testCapUnicodeProperties.cp
// Comprehensive unit tests for @cap/unicode-properties

import { test, expect_eq } from "cap:test";
import {
    isXidStart,
    isXidContinue,
    isUnicodeWhitespace,
    isEmoji,
    isEmojiPresentation,
    isDefaultIgnorable,
    isValidIdentifier,
} from "../../packages/@cap/unicode-properties/src/index.cp";

fn main() {
    test("1. @cap/unicode-properties — isXidStart multilingual scripts & exclusions", () => {
        // ASCII letters
        expect_eq(isXidStart('A' as u32), true);
        expect_eq(isXidStart('z' as u32), true);
        expect_eq(isXidStart('_' as u32), false); // '_' is XID_Continue, not XID_Start
        expect_eq(isXidStart('0' as u32), false);
        expect_eq(isXidStart(' ' as u32), false);

        // Latin Extended, Greek, Cyrillic
        expect_eq(isXidStart(0x00E9 as u32), true);  // é
        expect_eq(isXidStart(0x00D7 as u32), false); // × multiplication sign
        expect_eq(isXidStart(0x00F7 as u32), false); // ÷ division sign
        expect_eq(isXidStart(0x0152 as u32), true);  // Œ
        expect_eq(isXidStart(0x03B1 as u32), true);  // α
        expect_eq(isXidStart(0x0416 as u32), true);  // Ж

        // CJK, Hiragana, Katakana, Hangul
        expect_eq(isXidStart(0x4E2D as u32), true);  // 中
        expect_eq(isXidStart(0x3042 as u32), true);  // あ
        expect_eq(isXidStart(0x30A2 as u32), true);  // ア
        expect_eq(isXidStart(0xAC00 as u32), true);  // 가
        expect_eq(isXidStart(0x20000 as u32), true); // CJK Ext B
    });

    test("2. @cap/unicode-properties — isXidContinue digits, underscore & combining marks", () => {
        expect_eq(isXidContinue('a' as u32), true);
        expect_eq(isXidContinue('Z' as u32), true);
        expect_eq(isXidContinue('0' as u32), true);
        expect_eq(isXidContinue('9' as u32), true);
        expect_eq(isXidContinue('_' as u32), true);
        expect_eq(isXidContinue(0x0301 as u32), true);  // Combining Acute Accent
        expect_eq(isXidContinue(0x036F as u32), true);  // Combining Latin Small Letter X
        expect_eq(isXidContinue(0x3099 as u32), true);  // Combining Voiced Sound Mark
        expect_eq(isXidContinue(0x4E2D as u32), true);  // 中
        expect_eq(isXidContinue('-' as u32), false);
        expect_eq(isXidContinue('.' as u32), false);
        expect_eq(isXidContinue(' ' as u32), false);
    });

    test("3. @cap/unicode-properties — isUnicodeWhitespace exact 25 codepoints", () => {
        // Verify exact count across the entire BMP (0x0000..0xFFFF)
        let count: u32 = 0;
        let cp: u32 = 0;
        while (cp <= (0xFFFF as u32)) {
            if (isUnicodeWhitespace(cp)) {
                count = count + 1;
            }
            cp = cp + 1;
        }
        expect_eq(count, 25 as u32);

        // Spot-check key whitespace and non-whitespace boundaries
        expect_eq(isUnicodeWhitespace(0x0009 as u32), true);  // TAB
        expect_eq(isUnicodeWhitespace(0x000A as u32), true);  // LF
        expect_eq(isUnicodeWhitespace(0x000D as u32), true);  // CR
        expect_eq(isUnicodeWhitespace(0x0020 as u32), true);  // SPACE
        expect_eq(isUnicodeWhitespace(0x0085 as u32), true);  // NEL
        expect_eq(isUnicodeWhitespace(0x00A0 as u32), true);  // NBSP
        expect_eq(isUnicodeWhitespace(0x1680 as u32), true);  // OGHAM SPACE
        expect_eq(isUnicodeWhitespace(0x2000 as u32), true);  // EN QUAD
        expect_eq(isUnicodeWhitespace(0x200A as u32), true);  // HAIR SPACE
        expect_eq(isUnicodeWhitespace(0x200B as u32), false); // ZWSP is NOT White_Space
        expect_eq(isUnicodeWhitespace(0x2028 as u32), true);  // LINE SEPARATOR
        expect_eq(isUnicodeWhitespace(0x2029 as u32), true);  // PARAGRAPH SEPARATOR
        expect_eq(isUnicodeWhitespace(0x202F as u32), true);  // NNBSP
        expect_eq(isUnicodeWhitespace(0x205F as u32), true);  // MMSP
        expect_eq(isUnicodeWhitespace(0x3000 as u32), true);  // IDEOGRAPHIC SPACE
        expect_eq(isUnicodeWhitespace(0xFEFF as u32), false); // BOM is NOT White_Space
    });

    test("4. @cap/unicode-properties — isEmoji & isEmojiPresentation", () => {
        // Emoji_Presentation codepoints (both true)
        expect_eq(isEmojiPresentation(0x1F600 as u32), true); // 😀
        expect_eq(isEmoji(0x1F600 as u32), true);
        expect_eq(isEmojiPresentation(0x1F680 as u32), true); // 🚀
        expect_eq(isEmoji(0x1F680 as u32), true);
        expect_eq(isEmojiPresentation(0x2615 as u32), true);  // ☕
        expect_eq(isEmoji(0x2615 as u32), true);
        expect_eq(isEmojiPresentation(0x2B50 as u32), true);  // ⭐
        expect_eq(isEmoji(0x2B50 as u32), true);
        expect_eq(isEmojiPresentation(0x1FA75 as u32), true); // 🩵

        // Text-default Emoji (isEmoji=true, isEmojiPresentation=false)
        expect_eq(isEmoji(0x00A9 as u32), true);              // ©
        expect_eq(isEmojiPresentation(0x00A9 as u32), false);
        expect_eq(isEmoji(0x2764 as u32), true);              // ❤
        expect_eq(isEmojiPresentation(0x2764 as u32), false);
        expect_eq(isEmoji(0x2122 as u32), true);              // ™
        expect_eq(isEmojiPresentation(0x2122 as u32), false);

        // Non-emoji characters
        expect_eq(isEmoji('A' as u32), false);
        expect_eq(isEmojiPresentation('A' as u32), false);
        expect_eq(isEmoji(0x4E2D as u32), false);             // 中
    });

    test("5. @cap/unicode-properties — isDefaultIgnorable", () => {
        expect_eq(isDefaultIgnorable(0x00AD as u32), true);  // Soft Hyphen
        expect_eq(isDefaultIgnorable(0x200B as u32), true);  // ZWSP
        expect_eq(isDefaultIgnorable(0x200C as u32), true);  // ZWNJ
        expect_eq(isDefaultIgnorable(0x200D as u32), true);  // ZWJ
        expect_eq(isDefaultIgnorable(0x200F as u32), true);  // RLM
        expect_eq(isDefaultIgnorable(0xFE00 as u32), true);  // VS1
        expect_eq(isDefaultIgnorable(0xFE0F as u32), true);  // VS16
        expect_eq(isDefaultIgnorable(0xFEFF as u32), true);  // BOM
        expect_eq(isDefaultIgnorable(0xE0100 as u32), true); // VS17

        expect_eq(isDefaultIgnorable('A' as u32), false);
        expect_eq(isDefaultIgnorable(0x0020 as u32), false);
        expect_eq(isDefaultIgnorable(0x4E2D as u32), false);
    });

    test("6. @cap/unicode-properties — isValidIdentifier UTF-8 validation", () => {
        // Valid identifiers: ASCII, underscore prefix, Latin, Greek, CJK, Japanese, Korean, combining marks
        expect_eq(isValidIdentifier("hello"), true);
        expect_eq(isValidIdentifier("_private_123"), true);
        expect_eq(isValidIdentifier("_"), true);
        expect_eq(isValidIdentifier("café_2026"), true);
        expect_eq(isValidIdentifier("αβγ"), true);
        expect_eq(isValidIdentifier("变量名1"), true);
        expect_eq(isValidIdentifier("こんにちは_世界"), true);
        expect_eq(isValidIdentifier("변수명"), true);
        expect_eq(isValidIdentifier("e\xCC\x81clair"), true); // 'e' + U+0301 combining acute accent

        // Invalid identifiers: empty, leading digit, whitespace, punctuation, emoji, malformed UTF-8
        expect_eq(isValidIdentifier(""), false);
        expect_eq(isValidIdentifier("1abc"), false);
        expect_eq(isValidIdentifier("hello world"), false);
        expect_eq(isValidIdentifier("foo-bar"), false);
        expect_eq(isValidIdentifier("foo.bar"), false);
        expect_eq(isValidIdentifier("🚀rocket"), false);
        expect_eq(isValidIdentifier("a🚀"), false);
        expect_eq(isValidIdentifier("\xFF\xFE"), false);
    });
}
