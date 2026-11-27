// cap/test/testCapAnsiStrip.cp
// Comprehensive unit tests for @cap/ansi-strip (strip-ansi / ansi-regex parity)

import { test, expect_eq } from "cap:test";
import {
    hasAnsi,
    stripAnsi,
    strippedLen,
    visibleAsciiCount,
} from "../../packages/strip-ansi/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ansi-strip — plain text fast-path hasAnsi == false", () => {
        const plain = "Hello, Cap Terminal!";
        expect_eq(hasAnsi(plain), false);
        expect_eq(strippedLen(plain), plain.len);
        expect_eq(visibleAsciiCount(plain), plain.len);

        let out: [64]u8 = undefined;
        const n: usize = stripAnsi(plain, out[0..64]);
        expect_eq(sliceEq(out[0..n], plain), true);
    });

    test("2. @cap/ansi-strip — SGR colors & 24-bit TrueColor stripping", () => {
        const colored = "\x1b[1;32mOK\x1b[0m \x1b[38;2;255;0;0mRed\x1b[0m";
        expect_eq(hasAnsi(colored), true);

        let out: [64]u8 = undefined;
        const n: usize = stripAnsi(colored, out[0..64]);
        expect_eq(sliceEq(out[0..n], "OK Red"), true);
        expect_eq(visibleAsciiCount(colored), 6 as usize);
    });

    test("3. @cap/ansi-strip — OSC 8 hyperlinks with BEL & ESC \\ ST terminators", () => {
        const bel_link = "\x1b]8;;https://capdux.org\x07Cap\x1b]8;;\x07";
        const st_link = "\x1b]8;;https://capdux.org\x1b\\Docs\x1b]8;;\x1b\\";

        let out: [64]u8 = undefined;
        const n1: usize = stripAnsi(bel_link, out[0..64]);
        expect_eq(sliceEq(out[0..n1], "Cap"), true);

        const n2: usize = stripAnsi(st_link, out[0..64]);
        expect_eq(sliceEq(out[0..n2], "Docs"), true);
    });

    test("4. @cap/ansi-strip — strippedLen & visibleAsciiCount with UTF-8", () => {
        const s = "\x1b[36m中文\x1b[0m Hi";
        expect_eq(strippedLen(s), 9 as usize); // "中文" (6 bytes) + " Hi" (3 bytes)
        expect_eq(visibleAsciiCount(s), 5 as usize); // 中, 文, ' ', H, i

        let out: [32]u8 = undefined;
        const n: usize = stripAnsi(s, out[0..32]);
        expect_eq(n, strippedLen(s));
        expect_eq(sliceEq(out[0..n], "中文 Hi"), true);
    });

    test("5. @cap/ansi-strip — truncated incomplete ESC sequences handled safely", () => {
        const trunc1 = "abc\x1b";
        const trunc2 = "abc\x1b[38;2;100";
        const trunc3 = "abc\x1b]0;title";
        let out: [32]u8 = undefined;

        const n1: usize = stripAnsi(trunc1, out[0..32]);
        expect_eq(sliceEq(out[0..n1], "abc"), true);

        const n2: usize = stripAnsi(trunc2, out[0..32]);
        expect_eq(sliceEq(out[0..n2], "abc"), true);

        const n3: usize = stripAnsi(trunc3, out[0..32]);
        expect_eq(sliceEq(out[0..n3], "abc"), true);
    });

    test("6. @cap/ansi-strip — empty input & tiny buffer protection", () => {
        let out: [16]u8 = undefined;
        expect_eq(hasAnsi(""), false);
        expect_eq(strippedLen(""), 0 as usize);
        expect_eq(stripAnsi("", out[0..16]), 0 as usize);

        let tiny: [2]u8 = undefined;
        expect_eq(stripAnsi("\x1b[31mHello\x1b[0m", tiny[0..2]), 0 as usize);
    });
}
