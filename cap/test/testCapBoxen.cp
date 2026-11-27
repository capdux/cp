// cap/test/testCapBoxen.cp
// Comprehensive unit tests for @cap/boxen

import { test, expect_eq } from "cap:test";
import {
    BorderStyle,
    TextAlign,
    BoxOptions,
    displayWidth,
    renderBox,
    box,
    titledBox,
} from "../../packages/boxen/src/index.cp";

fn allLinesEqualDisplayWidth(rendered: string) -> bool {
    let first_w: usize = 0;
    let has_first = false;
    let cur: usize = 0;
    while (true) {
        const ls = cur;
        while (cur < rendered.len && rendered[cur] != '\n') {
            cur += 1;
        }
        const lw = displayWidth(rendered[ls..cur]);
        if (!has_first) {
            first_w = lw;
            has_first = true;
        } else if (lw != first_w) {
            return false;
        }
        if (cur >= rendered.len) {
            break;
        }
        cur += 1;
    }
    return has_first;
}

fn main() {
    test("1. @cap/boxen — default single-line box & all 6 border styles", () => {
        let buf: [512]u8 = undefined;
        expect_eq(box("Hi", buf[0..512]), "┌────┐\n│ Hi │\n└────┘");

        let opts = BoxOptions.defaults();
        opts.style = BorderStyle.Double;
        expect_eq(renderBox("Hi", opts, buf[0..512]), "╔════╗\n║ Hi ║\n╚════╝");

        opts.style = BorderStyle.Round;
        expect_eq(renderBox("Hi", opts, buf[0..512]), "╭────╮\n│ Hi │\n╰────╯");

        opts.style = BorderStyle.Bold;
        expect_eq(renderBox("Hi", opts, buf[0..512]), "┏━━━━┓\n┃ Hi ┃\n┗━━━━┛");

        opts.style = BorderStyle.Classic;
        expect_eq(renderBox("Hi", opts, buf[0..512]), "+----+\n| Hi |\n+----+");

        opts.style = BorderStyle.SingleDouble;
        expect_eq(renderBox("Hi", opts, buf[0..512]), "╓────╖\n║ Hi ║\n╙────╜");
    });

    test("2. @cap/boxen — multiline Center & Right alignment", () => {
        let buf: [512]u8 = undefined;

        let opts_c = BoxOptions.defaults();
        opts_c.style = BorderStyle.Classic;
        opts_c.text_align = TextAlign.Center;
        const centered = renderBox("CP\nHelloWorld", opts_c, buf[0..512]);
        expect_eq(centered, "+------------+\n|     CP     |\n| HelloWorld |\n+------------+");

        let opts_r = BoxOptions.defaults();
        opts_r.style = BorderStyle.Classic;
        opts_r.text_align = TextAlign.Right;
        const righted = renderBox("CP\nHelloWorld", opts_r, buf[0..512]);
        expect_eq(righted, "+------------+\n|         CP |\n| HelloWorld |\n+------------+");
    });

    test("3. @cap/boxen — embedded top-border title (Left / Center / Right)", () => {
        let buf: [512]u8 = undefined;
        const left_t = titledBox("Status", "OK\nAll systems go", BorderStyle.Round, buf[0..512]);
        expect_eq(left_t, "╭─ Status ───────╮\n│ OK             │\n│ All systems go │\n╰────────────────╯");
        expect_eq(allLinesEqualDisplayWidth(left_t), true);

        let opts = BoxOptions.defaults();
        opts.style = BorderStyle.Classic;
        opts.title = "Info";
        opts.title_align = TextAlign.Center;
        const center_t = renderBox("1234567890", opts, buf[0..512]);
        expect_eq(center_t, "+--- Info ---+\n| 1234567890 |\n+------------+");

        opts.title_align = TextAlign.Right;
        const right_t = renderBox("1234567890", opts, buf[0..512]);
        expect_eq(right_t, "+----- Info -+\n| 1234567890 |\n+------------+");
    });

    test("4. @cap/boxen — padding_x, padding_y & margin_left", () => {
        let buf: [512]u8 = undefined;
        let opts = BoxOptions.defaults();
        opts.style = BorderStyle.Classic;
        opts.padding_x = 2;
        opts.padding_y = 1;
        opts.margin_left = 2;

        const out = renderBox("CP", opts, buf[0..512]);
        expect_eq(out, "  +------+\n  |      |\n  |  CP  |\n  |      |\n  +------+");
    });

    test("5. @cap/boxen — CJK double-width pixel-aligned right border", () => {
        expect_eq(displayWidth("Hello"), 5 as usize);
        expect_eq(displayWidth("你好CP"), 6 as usize); // 2 + 2 + 1 + 1

        let buf: [512]u8 = undefined;
        const out = titledBox("状态", "你好CP\nOK", BorderStyle.Round, buf[0..512]);
        expect_eq(allLinesEqualDisplayWidth(out), true);
        expect_eq(out, "╭─ 状态 ─╮\n│ 你好CP │\n│ OK     │\n╰────────╯");
    });

    test("6. @cap/boxen — ANSI color escape sequences have zero display width", () => {
        const colored = "\x1b[32mPASS\x1b[0m";
        expect_eq(displayWidth(colored), 4 as usize);

        let buf: [512]u8 = undefined;
        let opts = BoxOptions.defaults();
        opts.style = BorderStyle.Classic;
        const out = renderBox("\x1b[32mPASS\x1b[0m\n123456", opts, buf[0..512]);
        expect_eq(allLinesEqualDisplayWidth(out), true);
        expect_eq(out, "+--------+\n| \x1b[32mPASS\x1b[0m   |\n| 123456 |\n+--------+");
    });
}
