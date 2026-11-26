import { test, expect_eq } from "cap:test";
import {
    BOLD, ITALIC, FG_RED, BG_BLUE, FG_CYAN_BRIGHT,
    wrapStyle, formatAnsi256, formatAnsi16m, formatRgbToAnsi16,
    formatRgbToAnsi256, formatHexToAnsi16m, formatKeywordToAnsi16m
} from "../../packages/@cap/ansi-styles/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ansi-styles — wrapStyle applies open and close SGR codes", () => {
        let buf: [64]u8 = undefined;
        let n1 = wrapStyle(FG_RED, "error", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "\x1b[31merror\x1b[39m"), true);

        let n2 = wrapStyle(BOLD, "CP", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "\x1b[1mCP\x1b[22m"), true);

        let n3 = wrapStyle(BG_BLUE, "ok", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "\x1b[44mok\x1b[49m"), true);
    });

    test("2. @cap/ansi-styles — formatAnsi256 for foreground and background", () => {
        let buf: [32]u8 = undefined;
        let n1 = formatAnsi256(196, false, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n1, "\x1b[38;5;196m"), true);

        let n2 = formatAnsi256(21, true, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n2, "\x1b[48;5;21m"), true);
    });

    test("3. @cap/ansi-styles — formatAnsi16m Truecolor foreground and background", () => {
        let buf: [32]u8 = undefined;
        let n1 = formatAnsi16m(255, 136, 0, false, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n1, "\x1b[38;2;255;136;0m"), true);

        let n2 = formatAnsi16m(10, 20, 30, true, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n2, "\x1b[48;2;10;20;30m"), true);
    });

    test("4. @cap/ansi-styles — formatHexToAnsi16m delegates to @cap/color-convert", () => {
        let buf: [32]u8 = undefined;
        let n1 = formatHexToAnsi16m("#FF8800", false, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n1, "\x1b[38;2;255;136;0m"), true);

        let n2 = formatHexToAnsi16m("invalid", false, buf[0..32]);
        expect_eq(n2, 0);
    });

    test("5. @cap/ansi-styles — formatKeywordToAnsi16m delegates via @cap/color-convert -> @cap/color-name", () => {
        let buf: [32]u8 = undefined;
        let n1 = formatKeywordToAnsi16m("rebeccapurple", false, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n1, "\x1b[38;2;102;51;153m"), true);

        let n2 = formatKeywordToAnsi16m("notacolor", false, buf[0..32]);
        expect_eq(n2, 0);
    });

    test("6. @cap/ansi-styles — formatRgbToAnsi16 and formatRgbToAnsi256", () => {
        let buf: [32]u8 = undefined;
        let n1 = formatRgbToAnsi16(255, 0, 0, false, buf[0..32]);
        expect_eq(n1 > 0, true);
        expect_eq(buf[0], 27);

        let n2 = formatRgbToAnsi256(255, 0, 0, false, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n2, "\x1b[38;5;196m"), true);

        let n3 = wrapStyle(FG_CYAN_BRIGHT, "hi", buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n3, "\x1b[96mhi\x1b[39m"), true);
    });
}
