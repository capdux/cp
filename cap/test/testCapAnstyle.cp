import { test, expect_eq } from "cap:test";
import {
    Color,
    Effects,
    Style,
} from "../../packages/anstyle/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn expectEscSeq(buf: []const u8, suffix: string): bool {
    if (buf.len != suffix.len + 1) {
        return false;
    }
    if (buf[0] != 27) {
        return false;
    }
    return strEq(buf[1..buf.len], suffix);
}

fn main() {
    test("1. @cap/anstyle — Plain style passes text through with zero ANSI codes", () => {
        const plain: Style = Style.new();
        expect_eq(plain.isPlain(), true);

        let buf: [32]u8 = undefined;
        expect_eq(plain.renderStart(&buf), 0);
        expect_eq(plain.renderReset(&buf), 0);

        const n: usize = plain.paint("hello plain", &buf);
        expect_eq(strEq(buf[0..n], "hello plain"), true);
    });

    test("2. @cap/anstyle — Basic 16-color foreground & background rendering", () => {
        const s0: Style = Style.new();
        const s1: Style = s0.fgColor(Color.ansi(1)); // red -> 31
        const s2: Style = s1.bgColor(Color.ansi(14)); // bright cyan -> 106

        let buf: [32]u8 = undefined;
        const n: usize = s2.renderStart(&buf);
        expect_eq(expectEscSeq(buf[0..n], "[31;106m"), true);

        const s_bright: Style = s0.fgColor(Color.ansi(9)); // bright red -> 91
        const nb: usize = s_bright.renderStart(&buf);
        expect_eq(expectEscSeq(buf[0..nb], "[91m"), true);
    });

    test("3. @cap/anstyle — 256-color & TrueColor RGB rendering", () => {
        const s0: Style = Style.new();
        const s256: Style = s0.fgColor(Color.ansi256(196));
        let buf: [64]u8 = undefined;
        const n1: usize = s256.renderStart(&buf);
        expect_eq(expectEscSeq(buf[0..n1], "[38;5;196m"), true);

        const srgb1: Style = s0.fgColor(Color.rgb(255, 128, 0));
        const srgb2: Style = srgb1.bgColor(Color.rgb(10, 20, 30));
        const n2: usize = srgb2.renderStart(&buf);
        expect_eq(expectEscSeq(buf[0..n2], "[38;2;255;128;0;48;2;10;20;30m"), true);
    });

    test("4. @cap/anstyle — Multiple Effects combination (bold + italic + underline)", () => {
        const s0: Style = Style.new();
        const s1: Style = s0.bold();
        const s2: Style = s1.italic();
        const s3: Style = s2.underline();
        const s4: Style = s3.strikethrough();

        let buf: [32]u8 = undefined;
        const n: usize = s4.renderStart(&buf);
        expect_eq(expectEscSeq(buf[0..n], "[1;3;4;9m"), true);

        const e0: Effects = Effects.new();
        expect_eq(e0.isEmpty(), true);
    });

    test("5. @cap/anstyle — paint() wraps text & appends automatic ESC[0m reset", () => {
        const s0: Style = Style.new();
        const s1: Style = s0.bold();
        const s2: Style = s1.fgColor(Color.ansi(2)); // green -> 32

        let buf: [64]u8 = undefined;
        const n: usize = s2.paint("PASS", &buf);
        // Expected: ESC[1;32mPASSESC[0m -> length = 7 + 4 + 4 = 15
        expect_eq(n, 15);
        expect_eq(buf[0], 27);
        expect_eq(strEq(buf[1..7], "[1;32m"), true);
        expect_eq(strEq(buf[7..11], "PASS"), true);
        expect_eq(buf[11], 27);
        expect_eq(strEq(buf[12..15], "[0m"), true);
    });

    test("6. @cap/anstyle — Small buffer truncation boundary safety", () => {
        const s0: Style = Style.new();
        const s1: Style = s0.fgColor(Color.rgb(12, 34, 56));

        let tiny: [5]u8 = undefined;
        const n_start: usize = s1.renderStart(&tiny);
        expect_eq(n_start, 5);
        expect_eq(tiny[0], 27);
        expect_eq(strEq(tiny[1..5], "[38;"), true);

        const empty_out: []u8 = tiny[0..0];
        expect_eq(s1.renderStart(empty_out), 0);
        expect_eq(s1.renderReset(empty_out), 0);
        expect_eq(s1.paint("abc", empty_out), 0);

        let small_paint: [10]u8 = undefined;
        const s_red: Style = s0.fgColor(Color.ansi(1)); // ESC[31m (5 bytes)
        const np: usize = s_red.paint("HelloWorld", &small_paint);
        expect_eq(np, 10);
        expect_eq(strEq(small_paint[5..10], "Hello"), true);
    });
}
