// cap/test/testCapAnsiEscapes.cp
// Comprehensive unit tests for @cap/ansi-escapes

import { test, expect_eq } from "cap:test";
import {
    cursorTo,
    cursorToXY,
    cursorMove,
    cursorUp,
    cursorDown,
    cursorForward,
    cursorBackward,
    cursorLeft,
    cursorSave,
    cursorRestore,
    cursorHide,
    cursorShow,
    eraseLine,
    eraseEndLine,
    eraseStartLine,
    eraseDown,
    eraseScreen,
    clearTerminal,
    eraseLines,
    hyperlink,
    setWindowTitle,
    beep,
} from "../../packages/@cap/ansi-escapes/src/index.cp";

fn main() {
    test("1. @cap/ansi-escapes — absolute cursor positioning (cursorTo & cursorToXY)", () => {
        let buf: [64]u8 = undefined;
        expect_eq(cursorTo(0 as usize, buf[0..64]), "\x1b[1G");
        expect_eq(cursorTo(9 as usize, buf[0..64]), "\x1b[10G");
        expect_eq(cursorToXY(0 as usize, 0 as usize, buf[0..64]), "\x1b[1;1H");
        expect_eq(cursorToXY(4 as usize, 2 as usize, buf[0..64]), "\x1b[3;5H");
    });

    test("2. @cap/ansi-escapes — relative cursor movement (cursorMove, Up/Down/Forward/Backward)", () => {
        let buf: [64]u8 = undefined;
        expect_eq(cursorMove(3, -2, buf[0..64]), "\x1b[3C\x1b[2A");
        expect_eq(cursorMove(-4, 5, buf[0..64]), "\x1b[4D\x1b[5B");
        expect_eq(cursorMove(0, 0, buf[0..64]), "");

        expect_eq(cursorUp(3 as usize, buf[0..64]), "\x1b[3A");
        expect_eq(cursorDown(2 as usize, buf[0..64]), "\x1b[2B");
        expect_eq(cursorForward(5 as usize, buf[0..64]), "\x1b[5C");
        expect_eq(cursorBackward(1 as usize, buf[0..64]), "\x1b[1D");
        expect_eq(cursorUp(0 as usize, buf[0..64]), "");
    });

    test("3. @cap/ansi-escapes — static cursor save/restore/hide/show/left sequences", () => {
        expect_eq(cursorLeft(), "\x1b[G");
        expect_eq(cursorSave(), "\x1b7");
        expect_eq(cursorRestore(), "\x1b8");
        expect_eq(cursorHide(), "\x1b[?25l");
        expect_eq(cursorShow(), "\x1b[?25h");
    });

    test("4. @cap/ansi-escapes — line & screen erase constants", () => {
        expect_eq(eraseLine(), "\x1b[2K");
        expect_eq(eraseEndLine(), "\x1b[K");
        expect_eq(eraseStartLine(), "\x1b[1K");
        expect_eq(eraseDown(), "\x1b[J");
        expect_eq(eraseScreen(), "\x1b[2J");
        expect_eq(clearTerminal(), "\x1b[2J\x1b[3J\x1b[H");
    });

    test("5. @cap/ansi-escapes — eraseLines multi-line upward erase & home", () => {
        let buf: [128]u8 = undefined;
        expect_eq(eraseLines(0 as usize, buf[0..128]), "");
        expect_eq(eraseLines(1 as usize, buf[0..128]), "\x1b[2K\x1b[G");
        expect_eq(eraseLines(3 as usize, buf[0..128]), "\x1b[2K\x1b[1A\x1b[2K\x1b[1A\x1b[2K\x1b[G");
    });

    test("6. @cap/ansi-escapes — OSC-8 hyperlink, OSC-0 window title & beep", () => {
        let buf: [256]u8 = undefined;
        expect_eq(
            hyperlink("CapDux", "https://capdux.org", buf[0..256]),
            "\x1b]8;;https://capdux.org\x07CapDux\x1b]8;;\x07",
        );
        expect_eq(setWindowTitle("CP Build", buf[0..256]), "\x1b]0;CP Build\x07");
        expect_eq(beep(), "\x07");
    });
}
