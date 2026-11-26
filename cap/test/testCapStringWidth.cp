// cap/test/testCapStringWidth.cp
// Comprehensive unit tests for @cap/string-width (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    StringWidthOptions,
    stringWidth,
    stringWidthWithOptions,
    maxLineWidth,
    padEndVisible,
    padStartVisible,
} from "../../packages/@cap/string-width/src/index.cp";

fn main() {
    test("1. @cap/string-width — pure ASCII and CJK (Chinese/Japanese/Korean) double-width columns", () => {
        expect_eq(stringWidth("hello"), 5 as usize);
        expect_eq(stringWidth("你好"), 4 as usize);
        expect_eq(stringWidth("こんにちは"), 10 as usize);
        expect_eq(stringWidth("안녕하세요"), 10 as usize);
        expect_eq(stringWidth("CP语言2026"), 10 as usize); // 2 + 4 + 4
        expect_eq(stringWidth("ＡＢＣ"), 6 as usize); // Fullwidth Latin
    });

    test("2. @cap/string-width — strips ANSI SGR color codes and OSC 8 hyperlinks", () => {
        // SGR bold green + reset
        expect_eq(stringWidth("\x1b[1;32mhello\x1b[0m"), 5 as usize);
        expect_eq(stringWidth("\x1b[31m你好\x1b[0m world"), 10 as usize);

        // OSC 8 hyperlink with BEL terminator (\x07)
        const oscBel = "\x1b]8;;https://example.com\x07Click\x1b]8;;\x07";
        expect_eq(stringWidth(oscBel), 5 as usize);

        // OSC 8 hyperlink with ST terminator (\x1b\\) + CJK label
        const oscSt = "\x1b]8;;https://cp-lang.org\x1b\\文档\x1b]8;;\x1b\\";
        expect_eq(stringWidth(oscSt), 4 as usize);
    });

    test("3. @cap/string-width — count_ansi_escape_codes and ambiguous_is_narrow options", () => {
        const colored = "\x1b[31mabc\x1b[0m";
        const defOpts = StringWidthOptions.defaults();
        expect_eq(stringWidthWithOptions(colored, defOpts), 3 as usize);

        // Counting ANSI escape codes: ESC (0x1B) = 0 cols, "[31m" = 4, "abc" = 3, "[0m" = 3 -> 10
        const countAnsiOpts = StringWidthOptions {
            ambiguous_is_narrow: true,
            count_ansi_escape_codes: true,
        };
        expect_eq(stringWidthWithOptions(colored, countAnsiOpts), 10 as usize);

        // East Asian Ambiguous characters: ± × ÷ α Ω ─ │ (7 chars)
        const ambig = "±×÷αΩ─│";
        expect_eq(stringWidthWithOptions(ambig, defOpts), 7 as usize);

        const cjkOpts = StringWidthOptions {
            ambiguous_is_narrow: false,
            count_ansi_escape_codes: false,
        };
        expect_eq(stringWidthWithOptions(ambig, cjkOpts), 14 as usize);
    });

    test("4. @cap/string-width — Emoji, ZWJ sequences, variation selectors, flags, combining marks & controls", () => {
        // Single emoji
        expect_eq(stringWidth("🚀"), 2 as usize);
        // Emoji ZWJ sequence (woman technologist: U+1F469 U+200D U+1F4BB) -> 2 columns
        expect_eq(stringWidth("👩‍💻"), 2 as usize);
        // Family ZWJ sequence -> 2 columns
        expect_eq(stringWidth("👨‍👩‍👧‍👦"), 2 as usize);
        // Emoji with skin-tone modifier (U+1F44D U+1F3FD) -> 2 columns
        expect_eq(stringWidth("👍🏽"), 2 as usize);
        // Variation selector U+FE0F promotes heart U+2764 (1 col) to emoji presentation (2 cols)
        expect_eq(stringWidth("❤️"), 2 as usize);
        // Regional Indicator flag pair (U+1F1E8 U+1F1F3) -> 2 columns
        expect_eq(stringWidth("🇨🇳"), 2 as usize);
        // Combining acute accent e + U+0301 -> 1 column
        expect_eq(stringWidth("e\u{0301}"), 1 as usize);
        // Zero-width space U+200B & control chars -> 0 columns
        expect_eq(stringWidth("\u{200B}\x00\x07\x1F"), 0 as usize);
    });

    test("5. @cap/string-width — maxLineWidth across \\n and \\r\\n multiline text", () => {
        expect_eq(maxLineWidth(""), 0 as usize);
        expect_eq(maxLineWidth("short\n你好世界!\r\n\x1b[36mANSI\x1b[0m"), 9 as usize);
        expect_eq(maxLineWidth("abc\r\ndefgh\r\nij"), 5 as usize);
    });

    test("6. @cap/string-width — padEndVisible / padStartVisible alignment & empty string safety", () => {
        expect_eq(stringWidth(""), 0 as usize);

        let buf: [64]u8 = undefined;

        // Pad ANSI + CJK string (visible width = 4) to 8 cols -> adds 4 spaces
        const styled = "\x1b[32m中文\x1b[0m";
        const n1 = padEndVisible(styled, 8, 32, buf[0..64]);
        expect_eq(buf[0..n1], "\x1b[32m中文\x1b[0m    ");
        expect_eq(stringWidth(buf[0..n1]), 8 as usize);

        // Left-pad "hi" (2 cols) to 5 cols with '.'
        const n2 = padStartVisible("hi", 5, 46, buf[0..64]);
        expect_eq(buf[0..n2], "...hi");
        expect_eq(stringWidth(buf[0..n2]), 5 as usize);

        // Empty string padded to 3 spaces
        const n3 = padEndVisible("", 3, 0, buf[0..64]);
        expect_eq(buf[0..n3], "   ");

        // Already wider than target_cols -> unchanged
        const n4 = padStartVisible("你好世界", 4, 32, buf[0..64]);
        expect_eq(buf[0..n4], "你好世界");
    });
}
