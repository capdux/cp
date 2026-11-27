import { test, expect_eq } from "cap:test";
import {
    countChars,
    padStart,
    padEnd,
    padLeft,
    padRight,
    padCenter,
    pad,
    padCenterWith,
    PadOptions,
    truncateChars,
    repeatStr,
} from "../../packages/pad/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/pad-str — padStart zero-padding '42' -> '00042'", () => {
        let buf: [32]u8 = undefined;
        const n = padStart("42", 5, 48, buf[0..32]); // '0'
        expect_eq(n, 5 as usize);
        expect_eq(testEq(buf[0..n], "00042"), true);
    });

    test("2. @cap/pad-str — padEnd table column alignment", () => {
        let buf: [32]u8 = undefined;
        const n = padEnd("cpu", 8, 32, buf[0..32]); // ' '
        expect_eq(n, 8 as usize);
        expect_eq(testEq(buf[0..n], "cpu     "), true);
    });

    test("3. @cap/pad-str — padCenter even & odd padding symmetry", () => {
        let buf: [32]u8 = undefined;
        // Even padding: 2 left, 2 right
        const n1 = padCenter("CP", 6, 45, buf[0..32]); // '-'
        expect_eq(testEq(buf[0..n1], "--CP--"), true);

        // Odd padding (3 total): 1 left, 2 right
        const n2 = padCenter("CP", 5, 45, buf[0..32]);
        expect_eq(testEq(buf[0..n2], "-CP--"), true);
    });

    test("4. @cap/pad-str — UTF-8 multibyte codepoint-aware padding ('你好' = 2 chars, 6 bytes)", () => {
        const cn = "你好";
        expect_eq(cn.len, 6 as usize);
        expect_eq(countChars(cn), 2 as usize);

        let buf: [32]u8 = undefined;
        // Target 4 chars -> adds 2 '*' (total 8 bytes)
        const n1 = padStart(cn, 4, 42, buf[0..32]);
        expect_eq(n1, 8 as usize);
        expect_eq(testEq(buf[0..n1], "**你好"), true);

        const n2 = padCenter(cn, 5, 42, buf[0..32]); // 1 left, 2 right -> 9 bytes
        expect_eq(n2, 9 as usize);
        expect_eq(testEq(buf[0..n2], "*你好**"), true);
    });

    test("5. @cap/pad-str — repeatStr & count == 0", () => {
        let buf: [32]u8 = undefined;
        const n1 = repeatStr("ab", 4, buf[0..32]);
        expect_eq(testEq(buf[0..n1], "abababab"), true);

        expect_eq(repeatStr("ab", 0, buf[0..32]), 0 as usize);
        expect_eq(repeatStr("", 5, buf[0..32]), 0 as usize);
    });

    test("6. @cap/pad-str — Already-wide string verbatim copy & undersized buffer guard", () => {
        let buf: [32]u8 = undefined;
        const n = padStart("longer", 3, 48, buf[0..32]);
        expect_eq(testEq(buf[0..n], "longer"), true);

        let tiny: [4]u8 = undefined;
        expect_eq(padStart("42", 8, 48, tiny[0..4]), 0 as usize);
        expect_eq(padEnd("42", 8, 48, tiny[0..4]), 0 as usize);
        expect_eq(padCenter("42", 8, 48, tiny[0..4]), 0 as usize);
        expect_eq(repeatStr("abc", 3, tiny[0..4]), 0 as usize);
    });

    test("7. @cap/pad-str — padLeft & padRight aliases", () => {
        let buf: [32]u8 = undefined;
        const n1 = padLeft("7", 4, 48, buf[0..32]);
        expect_eq(testEq(buf[0..n1], "0007"), true);

        const n2 = padRight("ok", 5, 46, buf[0..32]);
        expect_eq(testEq(buf[0..n2], "ok..."), true);
    });

    test("8. @cap/pad-str — pad() & padCenterWith() with custom pad_str and truncate option", () => {
        let buf: [64]u8 = undefined;

        let o1 = PadOptions.defaults();
        o1.left = true;
        o1.pad_str = "ab";
        const n1 = pad("X", 6, o1, buf[0..64]);
        expect_eq(testEq(buf[0..n1], "ababaX"), true);

        let o2 = PadOptions.defaults();
        o2.truncate = true;
        const n2 = pad("你好世界", 2, o2, buf[0..64]);
        expect_eq(testEq(buf[0..n2], "你好"), true);
        expect_eq(truncateChars("abcdef", 3), "abc");

        let o3 = PadOptions.defaults();
        o3.pad_str = "-=";
        const n3 = padCenterWith("CP", 7, o3, buf[0..64]);
        expect_eq(testEq(buf[0..n3], "-=CP-=-"), true);

        o3.truncate = true;
        const n4 = padCenterWith("abcdef", 4, o3, buf[0..64]);
        expect_eq(testEq(buf[0..n4], "abcd"), true);
    });
}
