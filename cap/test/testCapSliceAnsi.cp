import { test, expect_eq } from "cap:test";
import {
    ansiVisibleWidth,
    sliceAnsi,
    sliceAnsiFrom,
    truncateAnsi,
} from "../../packages/slice-ansi/src/index.cp";

fn saTestEq(actual: []const u8, expected: []const u8): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < actual.len) {
        if (actual[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn saAppend(dst: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < dst.len) {
        dst[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn saAppendEsc(dst: []u8, pos: usize, suffix: string): usize {
    let p: usize = pos;
    if (p < dst.len) {
        dst[p] = 27;
        p += 1;
    }
    return saAppend(dst, p, suffix);
}

fn main() {
    test("1. @cap/slice-ansi — Plain ASCII slicing & sliceAnsiFrom", () => {
        const plain: string = "Hello, Terminal!";
        expect_eq(ansiVisibleWidth(plain), 16);

        let out: [64]u8 = undefined;
        const n1: usize = sliceAnsi(plain, 0, 5, &out);
        expect_eq(saTestEq(out[0..n1], "Hello"), true);

        const n2: usize = sliceAnsi(plain, 7, 15, &out);
        expect_eq(saTestEq(out[0..n2], "Terminal"), true);

        const n3: usize = sliceAnsiFrom(plain, 7, &out);
        expect_eq(saTestEq(out[0..n3], "Terminal!"), true);
    });

    test("2. @cap/slice-ansi — Foreground & background SGR preserved across slice with auto-close", () => {
        // Build: \x1b[31m\x1b[44mHello World\x1b[0m
        let src: [64]u8 = undefined;
        let sp: usize = 0;
        sp = saAppendEsc(&src, sp, "[31m");
        sp = saAppendEsc(&src, sp, "[44m");
        sp = saAppend(&src, sp, "Hello World");
        sp = saAppendEsc(&src, sp, "[0m");

        expect_eq(ansiVisibleWidth(src[0..sp]), 11);

        let out: [64]u8 = undefined;
        const n: usize = sliceAnsi(src[0..sp], 6, 11, &out);

        // Expected: \x1b[31;44mWorld\x1b[39m\x1b[49m
        let exp: [64]u8 = undefined;
        let ep: usize = 0;
        ep = saAppendEsc(&exp, ep, "[31;44m");
        ep = saAppend(&exp, ep, "World");
        ep = saAppendEsc(&exp, ep, "[39m");
        ep = saAppendEsc(&exp, ep, "[49m");

        expect_eq(saTestEq(out[0..n], exp[0..ep]), true);
        expect_eq(ansiVisibleWidth(out[0..n]), 5);
    });

    test("3. @cap/slice-ansi — Nested & multi-param SGR (ESC[1;31m) with full reset ESC[0m", () => {
        // Build: \x1b[1;31mBoldRed\x1b[32mBoldGreen\x1b[0m
        let src: [64]u8 = undefined;
        let sp: usize = 0;
        sp = saAppendEsc(&src, sp, "[1;31m");
        sp = saAppend(&src, sp, "BoldRed");
        sp = saAppendEsc(&src, sp, "[32m");
        sp = saAppend(&src, sp, "BoldGreen");
        sp = saAppendEsc(&src, sp, "[0m");

        expect_eq(ansiVisibleWidth(src[0..sp]), 16);

        let out: [64]u8 = undefined;
        // Slice columns [4, 11) -> "Red" (1;31) + "\x1b[32m" + "Bold" + "\x1b[0m"
        const n: usize = sliceAnsi(src[0..sp], 4, 11, &out);

        let exp: [64]u8 = undefined;
        let ep: usize = 0;
        ep = saAppendEsc(&exp, ep, "[1;31m");
        ep = saAppend(&exp, ep, "Red");
        ep = saAppendEsc(&exp, ep, "[32m");
        ep = saAppend(&exp, ep, "Bold");
        ep = saAppendEsc(&exp, ep, "[0m");

        expect_eq(saTestEq(out[0..n], exp[0..ep]), true);
        expect_eq(ansiVisibleWidth(out[0..n]), 7);
    });

    test("4. @cap/slice-ansi — CJK double-width column boundary slicing", () => {
        // "A你好B": 'A'(col 0, w=1), '你'(col 1..3, w=2), '好'(col 3..5, w=2), 'B'(col 5, w=1) -> total 6 cols
        const text: string = "A你好B";
        expect_eq(ansiVisibleWidth(text), 6);

        let out: [64]u8 = undefined;
        // [0, 3) -> "A你" (width 3)
        const n1: usize = sliceAnsi(text, 0, 3, &out);
        expect_eq(saTestEq(out[0..n1], "A你"), true);
        expect_eq(ansiVisibleWidth(out[0..n1]), 3);

        // [0, 2) -> "A" ('你' does not fit in remaining 1 col)
        const n2: usize = sliceAnsi(text, 0, 2, &out);
        expect_eq(saTestEq(out[0..n2], "A"), true);

        // [1, 5) -> "你好" (width 4)
        const n3: usize = sliceAnsi(text, 1, 5, &out);
        expect_eq(saTestEq(out[0..n3], "你好"), true);

        // [2, 6) -> skips '你' straddling col 2, yields "好B" (width 3)
        const n4: usize = sliceAnsi(text, 2, 6, &out);
        expect_eq(saTestEq(out[0..n4], "好B"), true);
    });

    test("5. @cap/slice-ansi — OSC 8 hyperlinks and non-SGR control codes skipped cleanly", () => {
        // Build: \x1b]8;;https://example.com\x07\x1b[36mClickMe\x1b[39m\x1b]8;;\x07
        let src: [96]u8 = undefined;
        let sp: usize = 0;
        sp = saAppendEsc(&src, sp, "]8;;https://example.com");
        src[sp] = 7; // BEL
        sp += 1;
        sp = saAppendEsc(&src, sp, "[36m");
        sp = saAppend(&src, sp, "ClickMe");
        sp = saAppendEsc(&src, sp, "[39m");
        sp = saAppendEsc(&src, sp, "]8;;");
        src[sp] = 7; // BEL
        sp += 1;

        expect_eq(ansiVisibleWidth(src[0..sp]), 7);

        let out: [64]u8 = undefined;
        const n: usize = sliceAnsi(src[0..sp], 0, 5, &out);

        // Expected: \x1b[36mClick\x1b[39m (OSC 8 skipped, fg 36 preserved & closed with 39)
        let exp: [64]u8 = undefined;
        let ep: usize = 0;
        ep = saAppendEsc(&exp, ep, "[36m");
        ep = saAppend(&exp, ep, "Click");
        ep = saAppendEsc(&exp, ep, "[39m");

        expect_eq(saTestEq(out[0..n], exp[0..ep]), true);
    });

    test("6. @cap/slice-ansi — truncateAnsi with ellipsis, no-op fit, and empty/edge bounds", () => {
        // Styled string: \x1b[33mHello, World!\x1b[0m (13 cols)
        let src: [64]u8 = undefined;
        let sp: usize = 0;
        sp = saAppendEsc(&src, sp, "[33m");
        sp = saAppend(&src, sp, "Hello, World!");
        sp = saAppendEsc(&src, sp, "[0m");

        let out: [64]u8 = undefined;
        // Truncate to 8 cols with "..." (3 cols) -> 5 cols of content ("\x1b[33mHello\x1b[39m") + "..."
        const n1: usize = truncateAnsi(src[0..sp], 8, "...", &out);
        let exp: [64]u8 = undefined;
        let ep: usize = 0;
        ep = saAppendEsc(&exp, ep, "[33m");
        ep = saAppend(&exp, ep, "Hello");
        ep = saAppendEsc(&exp, ep, "[39m");
        ep = saAppend(&exp, ep, "...");
        expect_eq(saTestEq(out[0..n1], exp[0..ep]), true);
        expect_eq(ansiVisibleWidth(out[0..n1]), 8);

        // Fits without truncation
        const n2: usize = truncateAnsi("Short", 10, "...", &out);
        expect_eq(saTestEq(out[0..n2], "Short"), true);

        // Empty input & invalid range boundaries
        expect_eq(sliceAnsi("", 0, 5, &out), 0);
        expect_eq(sliceAnsi("abc", 3, 2, &out), 0);
        expect_eq(truncateAnsi("abc", 0, "...", &out), 0);
    });
}
