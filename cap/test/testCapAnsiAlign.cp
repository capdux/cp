// cap/test/testCapAnsiAlign.cp
// Comprehensive unit tests for ansi-align (9 test suites)

import { test, expect_eq } from "cap:test";
import {
    AlignMode,
    AlignOptions,
    ansiVisibleWidth,
    ansiMaxLineWidth,
    alignAnsi,
    alignAnsiWithOptions,
    alignAnsiLines,
    alignCenter,
    alignRight,
    alignLeft,
} from "../../packages/ansi-align/src/index.cp";

fn main() {
    test("1. ansi-align — alignCenter & alignRight on plain ASCII multiline text", () => {
        let buf: [128]u8 = undefined;
        const src = "hi\nhello!\nok";
        const nCenter = alignCenter(src, buf[0..128]);
        expect_eq(buf[0..nCenter], "  hi\nhello!\n  ok");

        const nRight = alignRight(src, buf[0..128]);
        expect_eq(buf[0..nRight], "    hi\nhello!\n    ok");
    });

    test("2. ansi-align — alignLeft preserves text verbatim", () => {
        let buf: [128]u8 = undefined;
        const src = "short\n  indented\nlongest line";
        const n = alignLeft(src, buf[0..128]);
        expect_eq(buf[0..n], src);
        expect_eq(alignAnsi(src, AlignMode.Left, buf[0..128]), src.len);
    });

    test("3. ansi-align — ignores ANSI SGR color codes when computing alignment padding", () => {
        let buf: [128]u8 = undefined;
        const src = "\x1b[31mhi\x1b[0m\n\x1b[1;32mfoobar\x1b[0m";
        const nCenter = alignCenter(src, buf[0..128]);
        expect_eq(buf[0..nCenter], "  \x1b[31mhi\x1b[0m\n\x1b[1;32mfoobar\x1b[0m");

        const nRight = alignRight(src, buf[0..128]);
        expect_eq(buf[0..nRight], "    \x1b[31mhi\x1b[0m\n\x1b[1;32mfoobar\x1b[0m");
    });

    test("4. ansi-align — aligns CJK double-width characters and Emoji accurately", () => {
        let buf: [128]u8 = undefined;
        const src = "中文\n12345678\n🚀";
        const nCenter = alignCenter(src, buf[0..128]);
        expect_eq(buf[0..nCenter], "  中文\n12345678\n   🚀");

        const nRight = alignRight(src, buf[0..128]);
        expect_eq(buf[0..nRight], "    中文\n12345678\n      🚀");
    });

    test("5. ansi-align — preserves CRLF (\\r\\n) and keeps blank lines clean", () => {
        let buf: [128]u8 = undefined;
        const src = "ab\r\n\r\nabcdef\r\n";
        const n = alignAnsi(src, AlignMode.Center, buf[0..128]);
        expect_eq(buf[0..n], "  ab\r\n\r\nabcdef\r\n");
    });

    test("6. ansi-align — empty string, single line, and bounded output buffer", () => {
        let buf: [32]u8 = undefined;
        expect_eq(alignCenter("", buf[0..32]), 0 as usize);
        expect_eq(alignRight("", buf[0..32]), 0 as usize);

        const nSingle = alignCenter("solo", buf[0..32]);
        expect_eq(buf[0..nSingle], "solo");

        let tiny: [5]u8 = undefined;
        const nTiny = alignRight("a\n12345", tiny[0..5]);
        expect_eq(nTiny, 5 as usize);
        expect_eq(tiny[0..nTiny], "    a");
    });

    test("7. ansi-align — alignAnsiWithOptions supports custom pad and split", () => {
        let buf: [128]u8 = undefined;
        const opts = AlignOptions {
            mode: AlignMode.Right,
            split: "|",
            pad: ".",
        };
        const n = alignAnsiWithOptions("hi|hello!|ok", opts, buf[0..128]);
        expect_eq(buf[0..n], "....hi|hello!|....ok");

        const def_opts = AlignOptions.default();
        const nDef = alignAnsiWithOptions("hi\nhello!", def_opts, buf[0..128]);
        expect_eq(buf[0..nDef], "  hi\nhello!");
    });

    test("8. ansi-align — alignAnsiLines aligns string array with custom pad", () => {
        let buf: [128]u8 = undefined;
        const lines: [3]string = ["\x1b[33mgo\x1b[0m", "rustlang", "cp"];
        const n = alignAnsiLines(&lines, AlignMode.Center, "-", "\n", buf[0..128]);
        expect_eq(buf[0..n], "---\x1b[33mgo\x1b[0m\nrustlang\n---cp");
    });

    test("9. ansi-align — ansiVisibleWidth and ansiMaxLineWidth helpers", () => {
        expect_eq(ansiVisibleWidth("\x1b[32mhello\x1b[0m"), 5 as usize);
        expect_eq(ansiVisibleWidth("中文"), 4 as usize);
        expect_eq(ansiMaxLineWidth("a|abcd|ab", "|"), 4 as usize);
    });
}
