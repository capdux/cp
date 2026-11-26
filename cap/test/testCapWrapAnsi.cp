// cap/test/testCapWrapAnsi.cp
// Comprehensive unit tests for @cap/wrap-ansi

import { test, expect_eq } from "cap:test";
import {
    visibleWidth,
    wrapAnsi,
    sliceAnsi,
} from "../../packages/@cap/wrap-ansi/src/index.cp";

fn main() {
    test("1. @cap/wrap-ansi — visibleWidth with ANSI CSI & CJK double-width", () => {
        expect_eq(visibleWidth("Hello"), 5 as usize);
        expect_eq(visibleWidth("\x1b[31mHello\x1b[0m"), 5 as usize);
        expect_eq(visibleWidth("\x1b[1m\x1b[36m你好CP\x1b[0m"), 6 as usize);
        expect_eq(visibleWidth(""), 0 as usize);
    });

    test("2. @cap/wrap-ansi — plain text word-boundary wrapping", () => {
        let buf: [256]u8 = undefined;
        const wrapped = wrapAnsi("The quick brown fox jumps over", 10 as usize, false, buf[0..256]);
        expect_eq(wrapped, "The quick\nbrown fox\njumps over");
    });

    test("3. @cap/wrap-ansi — ANSI SGR styles closed before wrap & re-injected on next line", () => {
        let buf: [256]u8 = undefined;
        const wrapped = wrapAnsi("\x1b[31mHello World\x1b[0m", 7 as usize, false, buf[0..256]);
        expect_eq(wrapped, "\x1b[31mHello\x1b[0m\n\x1b[31mWorld\x1b[0m");

        // Nested styles: bold + green across wrap
        const nested = wrapAnsi("\x1b[1m\x1b[32mCP Fast Runtime\x1b[0m", 8 as usize, false, buf[0..256]);
        expect_eq(nested, "\x1b[1m\x1b[32mCP Fast\x1b[0m\n\x1b[1m\x1b[32mRuntime\x1b[0m");
    });

    test("4. @cap/wrap-ansi — hard_wrap splits overlong words at column boundary", () => {
        let buf: [256]u8 = undefined;
        // Soft wrap keeps overlong word intact
        expect_eq(wrapAnsi("abcdefghij", 4 as usize, false, buf[0..256]), "abcdefghij");
        // Hard wrap splits strictly every 4 columns, preserving ANSI color
        expect_eq(
            wrapAnsi("\x1b[33mabcdefghij\x1b[0m", 4 as usize, true, buf[0..256]),
            "\x1b[33mabcd\x1b[0m\n\x1b[33mefgh\x1b[0m\n\x1b[33mij\x1b[0m",
        );
    });

    test("5. @cap/wrap-ansi — CJK double-width hard wrapping respects column budget", () => {
        let buf: [256]u8 = undefined;
        // Each Chinese char is 2 columns -> max_cols=5 fits at most 2 chars (4 cols) per line
        const wrapped = wrapAnsi("你好世界中文", 5 as usize, true, buf[0..256]);
        expect_eq(wrapped, "你好\n世界\n中文");
    });

    test("6. @cap/wrap-ansi — sliceAnsi slices by visible column & preserves/closes styles", () => {
        let buf: [256]u8 = undefined;
        // Slice inside red text: injects \x1b[31m at start and closes with \x1b[0m
        expect_eq(
            sliceAnsi("\x1b[31mHello World\x1b[0m", 6 as usize, 11 as usize, buf[0..256]),
            "\x1b[31mWorld\x1b[0m",
        );

        // Slice across style change
        expect_eq(
            sliceAnsi("\x1b[31mAB\x1b[32mCD\x1b[0mEF", 1 as usize, 5 as usize, buf[0..256]),
            "\x1b[31mB\x1b[32mCD\x1b[0mE",
        );

        // CJK double-width slice: "你好世界" (cols 0..2, 2..4, 4..6, 6..8)
        expect_eq(sliceAnsi("你好世界", 2 as usize, 6 as usize, buf[0..256]), "好世");

        // Empty range
        expect_eq(sliceAnsi("\x1b[31mHello\x1b[0m", 5 as usize, 5 as usize, buf[0..256]), "");
    });
}
