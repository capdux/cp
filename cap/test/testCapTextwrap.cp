// cap/test/testCapTextwrap.cp
// Comprehensive unit tests for @cap/textwrap

import { test, expect_eq } from "cap:test";
import {
    WrapOptions,
    displayWidth,
    stripAnsi,
    wrapText,
    wrapTextWith,
    indent,
    dedent,
    truncate,
} from "../../packages/@cap/textwrap/src/index.cp";

fn main() {
    test("1. @cap/textwrap — English word-boundary wrapping & explicit newlines", () => {
        let out: [512]u8 = undefined;
        expect_eq(
            wrapText("hello world foo bar", 11, out[0..512]),
            "hello world\nfoo bar",
        );
        expect_eq(
            wrapText("first line\n\nsecond line wraps here", 12, out[0..512]),
            "first line\n\nsecond line\nwraps here",
        );
        // Reflow mode (preserve_newlines = false)
        const reflow_opts = WrapOptions{
            width: 15,
            break_words: true,
            preserve_newlines: false,
        };
        expect_eq(
            wrapTextWith("hello\nworld\nfrom cp", reflow_opts, out[0..512]),
            "hello world\nfrom cp",
        );
    });

    test("2. @cap/textwrap — CJK wide-character wrapping & mixed Latin/CJK", () => {
        let out: [512]u8 = undefined;
        expect_eq(displayWidth("你好世界"), 8 as usize);
        expect_eq(displayWidth("Hi你好"), 6 as usize);

        expect_eq(
            wrapText("你好世界和平", 6, out[0..512]),
            "你好世\n界和平",
        );
        expect_eq(
            wrapText("Hello世界你好", 7, out[0..512]),
            "Hello世\n界你好",
        );
    });

    test("3. @cap/textwrap — ANSI color displayWidth, stripAnsi & colored wrapping", () => {
        let out: [512]u8 = undefined;
        const colored = "\x1b[31mError\x1b[0m: \x1b[1;33m你好\x1b[0m";
        expect_eq(displayWidth(colored), 11 as usize);
        expect_eq(stripAnsi(colored, out[0..512]), "Error: 你好");

        const ansi_words = "\x1b[32mhello\x1b[0m \x1b[34mworld\x1b[0m";
        expect_eq(
            wrapText(ansi_words, 5, out[0..512]),
            "\x1b[32mhello\x1b[0m\n\x1b[34mworld\x1b[0m",
        );
    });

    test("4. @cap/textwrap — Long word hard-breaking vs break_words=false", () => {
        let out: [512]u8 = undefined;
        expect_eq(
            wrapText("supercalifragilistic", 8, out[0..512]),
            "supercal\nifragili\nstic",
        );

        const no_break = WrapOptions{
            width: 8,
            break_words: false,
            preserve_newlines: true,
        };
        expect_eq(
            wrapTextWith("hi supercalifragilistic ok", no_break, out[0..512]),
            "hi\nsupercalifragilistic\nok",
        );
    });

    test("5. @cap/textwrap — indent & dedent round-trip and common whitespace stripping", () => {
        let buf1: [512]u8 = undefined;
        let buf2: [512]u8 = undefined;

        const src = "fn main() {\n    print();\n\n    return;\n}";
        const indented = indent(src, "    ", buf1[0..512]);
        expect_eq(
            indented,
            "    fn main() {\n        print();\n\n        return;\n    }",
        );
        const restored = dedent(indented, buf2[0..512]);
        expect_eq(restored, src);

        // Mixed indentation levels with blank whitespace-only line
        const block = "      alpha\n        beta\n   \n      gamma";
        expect_eq(
            dedent(block, buf1[0..512]),
            "alpha\n  beta\n\ngamma",
        );
    });

    test("6. @cap/textwrap — display-width truncate (ASCII, CJK, ANSI, no-op)", () => {
        let out: [512]u8 = undefined;

        // No truncation needed
        expect_eq(truncate("hello", 10, "...", out[0..512]), "hello");

        // ASCII truncation
        expect_eq(truncate("hello world", 8, "...", out[0..512]), "hello...");

        // CJK wide-char boundary truncation (budget = 7 - 3 = 4 => 2 CJK chars)
        expect_eq(truncate("你好世界测试", 7, "...", out[0..512]), "你好...");
        // Budget = 8 - 3 = 5 => still only 2 CJK chars fit (4 cols), never splits a wide char
        expect_eq(truncate("你好世界测试", 8, "...", out[0..512]), "你好...");

        // ANSI-colored truncation safely emits reset before ellipsis
        const red_text = "\x1b[31mHello World\x1b[0m";
        const trunc_red = truncate(red_text, 8, "...", out[0..512]);
        expect_eq(trunc_red, "\x1b[31mHello\x1b[0m...");
        expect_eq(displayWidth(trunc_red), 8 as usize);
    });
}
