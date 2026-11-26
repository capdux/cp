// cap/test/testCapChalk.cp
// Comprehensive unit tests for @cap/chalk (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    COLOR_LEVEL_NONE,
    COLOR_LEVEL_BASIC,
    COLOR_LEVEL_256,
    COLOR_LEVEL_16M,
    TerminalEnv,
    FG_RED,
    FG_CYAN,
    Chalk,
} from "../../packages/@cap/chalk/src/index.cp";

fn main() {
    test("1. @cap/chalk — level=0 plain text passthrough with zero ANSI escapes", () => {
        const c = Chalk.new(COLOR_LEVEL_NONE);
        let buf: [128]u8 = undefined;

        const n1 = c.red("hello", buf[0..128]);
        expect_eq(buf[0..n1], "hello");

        const n2 = c.bold("world", buf[0..128]);
        expect_eq(buf[0..n2], "world");

        const n3 = c.rgb(255, 0, 128, "plain", buf[0..128]);
        expect_eq(buf[0..n3], "plain");

        const n4 = c.hex("#FF8800", "hex", buf[0..128]);
        expect_eq(buf[0..n4], "hex");

        const n5 = c.keyword("rebeccapurple", "kw", buf[0..128]);
        expect_eq(buf[0..n5], "kw");
    });

    test("2. @cap/chalk — level=1 basic 16-color styles & bold/underline/strikethrough", () => {
        const c = Chalk.new(COLOR_LEVEL_BASIC);
        let buf: [128]u8 = undefined;

        const n1 = c.red("err", buf[0..128]);
        expect_eq(buf[0..n1], "\x1b[31merr\x1b[39m");

        const n2 = c.green("ok", buf[0..128]);
        expect_eq(buf[0..n2], "\x1b[32mok\x1b[39m");

        const n3 = c.bold("hi", buf[0..128]);
        expect_eq(buf[0..n3], "\x1b[1mhi\x1b[22m");

        const n4 = c.underline("u", buf[0..128]);
        expect_eq(buf[0..n4], "\x1b[4mu\x1b[24m");

        const n5 = c.strikethrough("del", buf[0..128]);
        expect_eq(buf[0..n5], "\x1b[9mdel\x1b[29m");

        const n6 = c.gray("dimmed", buf[0..128]);
        expect_eq(buf[0..n6], "\x1b[90mdimmed\x1b[39m");
    });

    test("3. @cap/chalk — rgb / hex / keyword 4-level downsampling (3 -> 2 -> 1 -> 0)", () => {
        let buf: [128]u8 = undefined;

        // Level 3: Truecolor 24-bit
        const c3 = Chalk.new(COLOR_LEVEL_16M);
        const n3 = c3.rgb(255, 136, 0, "A", buf[0..128]);
        expect_eq(buf[0..n3], "\x1b[38;2;255;136;0mA\x1b[39m");

        const h3 = c3.hex("#FF8800", "A", buf[0..128]);
        expect_eq(buf[0..h3], "\x1b[38;2;255;136;0mA\x1b[39m");

        const k3 = c3.keyword("rebeccapurple", "A", buf[0..128]);
        expect_eq(buf[0..k3], "\x1b[38;2;102;51;153mA\x1b[39m");

        // Level 2: Automatic 256-color downsampling (\x1b[38;5;<n>m)
        const c2 = Chalk.new(COLOR_LEVEL_256);
        const n2 = c2.rgb(255, 0, 0, "R", buf[0..128]);
        expect_eq(buf[0..n2], "\x1b[38;5;196mR\x1b[39m");

        const h2 = c2.hex("#FF0000", "R", buf[0..128]);
        expect_eq(buf[0..h2], "\x1b[38;5;196mR\x1b[39m");

        // Level 1: Automatic 16-color downsampling
        const c1 = Chalk.new(COLOR_LEVEL_BASIC);
        const n1 = c1.rgb(255, 0, 0, "R", buf[0..128]);
        expect_eq(buf[0..n1], "\x1b[91mR\x1b[39m");

        const k1 = c1.keyword("red", "R", buf[0..128]);
        expect_eq(buf[0..k1], "\x1b[91mR\x1b[39m");
    });

    test("4. @cap/chalk — bgRgb background colors across levels 3, 2, 1, 0", () => {
        let buf: [128]u8 = undefined;

        const c3 = Chalk.new(COLOR_LEVEL_16M);
        const n3 = c3.bgRgb(10, 20, 30, "bg", buf[0..128]);
        expect_eq(buf[0..n3], "\x1b[48;2;10;20;30mbg\x1b[49m");

        const c2 = Chalk.new(COLOR_LEVEL_256);
        const n2 = c2.bgRgb(255, 0, 0, "bg", buf[0..128]);
        expect_eq(buf[0..n2], "\x1b[48;5;196mbg\x1b[49m");

        const c1 = Chalk.new(COLOR_LEVEL_BASIC);
        const n1 = c1.bgRgb(255, 0, 0, "bg", buf[0..128]);
        expect_eq(buf[0..n1], "\x1b[101mbg\x1b[49m");

        const c0 = Chalk.new(COLOR_LEVEL_NONE);
        const n0 = c0.bgRgb(255, 0, 0, "bg", buf[0..128]);
        expect_eq(buf[0..n0], "bg");
    });

    test("5. @cap/chalk — Chalk.fromEnv via @cap/supports-color & @cap/has-flag CLI overrides", () => {
        let env = TerminalEnv.tty();
        env.colorterm = "truecolor";

        // --no-color overrides truecolor TTY to level 0
        const noColorArgv: [1]string = ["--no-color"];
        const cNo = Chalk.fromEnv(env, noColorArgv[0..1]);
        expect_eq(cNo.level, COLOR_LEVEL_NONE);

        // --color=16m forces level 3 even on non-TTY env
        const nonTty = TerminalEnv.empty();
        const force16mArgv: [1]string = ["--color=16m"];
        const c16m = Chalk.fromEnv(nonTty, force16mArgv[0..1]);
        expect_eq(c16m.level, COLOR_LEVEL_16M);

        // --color=256 forces level 2
        const force256Argv: [1]string = ["--color=256"];
        const c256 = Chalk.fromEnv(nonTty, force256Argv[0..1]);
        expect_eq(c256.level, COLOR_LEVEL_256);
    });

    test("6. @cap/chalk — styled() composed modifiers & small buffer truncation safety", () => {
        const c = Chalk.new(COLOR_LEVEL_BASIC);
        let buf: [128]u8 = undefined;

        const n1 = c.styled("ALERT", true, true, FG_RED, buf[0..128]);
        expect_eq(buf[0..n1], "\x1b[1m\x1b[4m\x1b[31mALERT\x1b[39m\x1b[24m\x1b[22m");

        const c0 = Chalk.new(COLOR_LEVEL_NONE);
        const n0 = c0.styled("ALERT", true, true, FG_CYAN, buf[0..128]);
        expect_eq(buf[0..n0], "ALERT");

        // Small buffer never overflows
        let tiny: [6]u8 = undefined;
        const nTiny = c.styled("ALERT", true, true, FG_RED, tiny[0..6]);
        expect_eq(nTiny, 6 as usize);
        expect_eq(tiny[0..4], "\x1b[1m");
    });
}
