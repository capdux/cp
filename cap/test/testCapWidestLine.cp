// cap/test/testCapWidestLine.cp
// Comprehensive unit tests for @cap/widest-line (6 test suites)

import { test, expect_eq } from "cap:test";
import { StringWidthOptions } from "../../packages/string-width/src/index.cp";
import {
    widestLine,
    widestLineWithOptions,
    lineWidths,
    lineWidthsWithOptions,
    widestLineWithTab,
    countLines,
} from "../../packages/widest-line/src/index.cp";

fn main() {
    test("1. @cap/widest-line — empty string, single line, \\n and \\r\\n line endings", () => {
        expect_eq(widestLine(""), 0 as usize);
        expect_eq(countLines(""), 0 as usize);
        expect_eq(widestLine("a"), 1 as usize);
        expect_eq(countLines("a"), 1 as usize);
        expect_eq(widestLine("a\nbe\ncat\ndog"), 3 as usize);
        expect_eq(widestLine("short\r\nlongest line\r\nmid"), 12 as usize);
        expect_eq(widestLine("\n\n\r\n"), 0 as usize);
        expect_eq(countLines("a\nb\nc"), 3 as usize);
    });

    test("2. @cap/widest-line — ANSI SGR color codes and OSC 8 hyperlinks", () => {
        expect_eq(widestLine("\x1b[31mred\x1b[0m\n\x1b[1;32mhello world\x1b[0m"), 11 as usize);
        const osc = "\x1b]8;;https://example.com\x07Click\x1b]8;;\x07\nshort";
        expect_eq(widestLine(osc), 5 as usize);
        expect_eq(widestLine("\x1b[36m中文\x1b[0m\n\x1b[33mABC\x1b[0m"), 4 as usize);
    });

    test("3. @cap/widest-line — CJK double-width characters, Emoji ZWJ, flags & combining marks", () => {
        expect_eq(widestLine("hi\n你好世界\nok"), 8 as usize);
        expect_eq(widestLine("hello\nこんにちは\nworld"), 10 as usize);
        expect_eq(widestLine("a\n🚀\n👩‍💻\n🇨🇳"), 2 as usize);
        expect_eq(widestLine("e\u{0301}\nab"), 2 as usize);
        expect_eq(widestLine("CP语言\r\n👨‍👩‍👧‍👦🚀\r\n12345"), 6 as usize);
    });

    test("4. @cap/widest-line — lineWidths per-line collection and bounded output buffer", () => {
        let buf: [8]usize = undefined;
        expect_eq(lineWidths("", buf[0..8]), 0 as usize);

        const n1 = lineWidths("ab\n你好\r\n\x1b[31mxyz\x1b[0m\n", buf[0..8]);
        expect_eq(n1, 4 as usize);
        expect_eq(buf[0], 2 as usize);
        expect_eq(buf[1], 4 as usize);
        expect_eq(buf[2], 3 as usize);
        expect_eq(buf[3], 0 as usize);

        // Truncated output slice still returns total line count without out-of-bounds write
        let small: [2]usize = undefined;
        const n2 = lineWidths("one\ntwo2\nthree", small[0..2]);
        expect_eq(n2, 3 as usize);
        expect_eq(small[0], 3 as usize);
        expect_eq(small[1], 4 as usize);
    });

    test("5. @cap/widest-line — widestLineWithTab expands tab stops across ANSI & CJK", () => {
        expect_eq(widestLineWithTab("", 4), 0 as usize);
        // "\t" -> 4, "a\tb" -> 4 + 1 = 5
        expect_eq(widestLineWithTab("\t\na\tb", 4), 5 as usize);
        // "1234\tx" -> 4 -> next stop 8 + 1 = 9
        expect_eq(widestLineWithTab("a\tb\n1234\tx", 4), 9 as usize);
        // CJK "中" (2 cols) + "\t" (to 4) + "文" (2 cols) = 6
        expect_eq(widestLineWithTab("中\t文\nhello", 4), 6 as usize);
        // ANSI colored segment before tab: "ab" (2 cols) + "\t" (to 8) + "c" (1 col) = 9
        expect_eq(widestLineWithTab("\x1b[32mab\x1b[0m\tc", 8), 9 as usize);
        // tab_width == 0 treats tab as 0 width
        expect_eq(widestLineWithTab("ab\tcd", 0), 4 as usize);
    });

    test("6. @cap/widest-line — widestLineWithOptions & lineWidthsWithOptions (ambiguous & count ANSI)", () => {
        const ambig = "ab\n±×÷";
        const defOpts = StringWidthOptions.defaults();
        expect_eq(widestLineWithOptions(ambig, defOpts), 3 as usize);

        const cjkOpts = StringWidthOptions {
            ambiguous_is_narrow: false,
            count_ansi_escape_codes: false,
        };
        expect_eq(widestLineWithOptions(ambig, cjkOpts), 6 as usize);

        let widths: [4]usize = undefined;
        const n = lineWidthsWithOptions(ambig, cjkOpts, widths[0..4]);
        expect_eq(n, 2 as usize);
        expect_eq(widths[0], 2 as usize);
        expect_eq(widths[1], 6 as usize);

        const countAnsiOpts = StringWidthOptions {
            ambiguous_is_narrow: true,
            count_ansi_escape_codes: true,
        };
        // "\x1b[31mhi\x1b[0m": ESC(0) + "[31m"(4) + "hi"(2) + ESC(0) + "[0m"(3) = 9
        expect_eq(widestLineWithOptions("hello\n\x1b[31mhi\x1b[0m", countAnsiOpts), 9 as usize);
    });
}
