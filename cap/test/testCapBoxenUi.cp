// cap/test/testCapBoxenUi.cp
// Comprehensive unit tests for @cap/boxen-ui (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    BORDER_SINGLE,
    BORDER_DOUBLE,
    BORDER_ROUND,
    BORDER_BOLD,
    BORDER_CLASSIC,
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
    BoxenOptions,
    renderBox,
} from "../../packages/@cap/boxen-ui/src/index.cp";

fn main() {
    test("1. @cap/boxen-ui — default single-line box and classic/double/round/bold border styles", () => {
        let buf: [512]u8 = undefined;

        const n1 = renderBox("hello", BoxenOptions.defaultOpts(), buf[0..512]);
        expect_eq(buf[0..n1], "┌─────┐\n│hello│\n└─────┘");

        const dblOpts = BoxenOptions {
            border_style: BORDER_DOUBLE,
            padding_x: 0,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "",
        };
        const n2 = renderBox("CP", dblOpts, buf[0..512]);
        expect_eq(buf[0..n2], "╔══╗\n║CP║\n╚══╝");

        const rndOpts = BoxenOptions {
            border_style: BORDER_ROUND,
            padding_x: 0,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "",
        };
        const n3 = renderBox("ok", rndOpts, buf[0..512]);
        expect_eq(buf[0..n3], "╭──╮\n│ok│\n╰──╯");

        const boldOpts = BoxenOptions {
            border_style: BORDER_BOLD,
            padding_x: 0,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "",
        };
        const n4 = renderBox("hi", boldOpts, buf[0..512]);
        expect_eq(buf[0..n4], "┏━━┓\n┃hi┃\n┗━━┛");

        const clsOpts = BoxenOptions {
            border_style: BORDER_CLASSIC,
            padding_x: 0,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "",
        };
        const n5 = renderBox("box", clsOpts, buf[0..512]);
        expect_eq(buf[0..n5], "+---+\n|box|\n+---+");
    });

    test("2. @cap/boxen-ui — horizontal and vertical padding (padding_x & padding_y)", () => {
        let buf: [512]u8 = undefined;
        const opts = BoxenOptions {
            border_style: BORDER_SINGLE,
            padding_x: 2,
            padding_y: 1,
            text_align: ALIGN_LEFT,
            title: "",
        };
        const n = renderBox("CP", opts, buf[0..512]);
        expect_eq(
            buf[0..n],
            "┌──────┐\n│      │\n│  CP  │\n│      │\n└──────┘"
        );
    });

    test("3. @cap/boxen-ui — multiline text with LEFT, CENTER, and RIGHT alignment", () => {
        let buf: [512]u8 = undefined;

        const leftOpts = BoxenOptions {
            border_style: BORDER_SINGLE,
            padding_x: 1,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "",
        };
        const n1 = renderBox("hi\nworld!\r\nok", leftOpts, buf[0..512]);
        expect_eq(
            buf[0..n1],
            "┌────────┐\n│ hi     │\n│ world! │\n│ ok     │\n└────────┘"
        );

        const centerOpts = BoxenOptions {
            border_style: BORDER_SINGLE,
            padding_x: 1,
            padding_y: 0,
            text_align: ALIGN_CENTER,
            title: "",
        };
        const n2 = renderBox("hi\nworld!\nok", centerOpts, buf[0..512]);
        expect_eq(
            buf[0..n2],
            "┌────────┐\n│   hi   │\n│ world! │\n│   ok   │\n└────────┘"
        );

        const rightOpts = BoxenOptions {
            border_style: BORDER_SINGLE,
            padding_x: 1,
            padding_y: 0,
            text_align: ALIGN_RIGHT,
            title: "",
        };
        const n3 = renderBox("hi\nworld!\nok", rightOpts, buf[0..512]);
        expect_eq(
            buf[0..n3],
            "┌────────┐\n│     hi │\n│ world! │\n│     ok │\n└────────┘"
        );
    });

    test("4. @cap/boxen-ui — embedded top-border title with auto-widening when title > content", () => {
        let buf: [512]u8 = undefined;

        // Content wider than title
        const opts1 = BoxenOptions {
            border_style: BORDER_SINGLE,
            padding_x: 1,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "Title",
        };
        const n1 = renderBox("Hello World", opts1, buf[0..512]);
        expect_eq(
            buf[0..n1],
            "┌─ Title ─────┐\n│ Hello World │\n└─────────────┘"
        );

        // Title wider than content -> box expands to "┌─ LongTitle ─┐" (inner_w = 9 + 4 = 13)
        const opts2 = BoxenOptions {
            border_style: BORDER_ROUND,
            padding_x: 0,
            padding_y: 0,
            text_align: ALIGN_CENTER,
            title: "LongTitle",
        };
        const n2 = renderBox("hi", opts2, buf[0..512]);
        expect_eq(
            buf[0..n2],
            "╭─ LongTitle ─╮\n│     hi      │\n╰─────────────╯"
        );
    });

    test("5. @cap/boxen-ui — CJK double-width characters & ANSI SGR color code column alignment", () => {
        let buf: [512]u8 = undefined;

        // "你好" = 4 cols, "\x1b[32mCP\x1b[0m" = 2 visible cols, "测试!" = 5 cols -> max = 5
        const opts = BoxenOptions {
            border_style: BORDER_SINGLE,
            padding_x: 1,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "中文",
        };
        const n = renderBox("你好\n\x1b[32mCP\x1b[0m\n测试!", opts, buf[0..512]);
        // title "中文" = 4 cols -> min_title_inner = 8; content_w + 2 = 7 -> inner_w expands to 8
        expect_eq(
            buf[0..n],
            "┌─ 中文 ─┐\n│ 你好   │\n│ \x1b[32mCP\x1b[0m     │\n│ 测试!  │\n└────────┘"
        );
    });

    test("6. @cap/boxen-ui — empty text & small output buffer truncation safety", () => {
        let buf: [256]u8 = undefined;

        const n1 = renderBox("", BoxenOptions.defaultOpts(), buf[0..256]);
        expect_eq(buf[0..n1], "┌┐\n││\n└┘");

        // Tight buffer must never overflow
        let tiny: [10]u8 = undefined;
        const n2 = renderBox("hello world", BoxenOptions.defaultOpts(), tiny[0..10]);
        expect_eq(n2, 10 as usize);
    });
}
