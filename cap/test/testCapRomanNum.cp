import { test, expect_eq } from "cap:test";
import { toRoman, parseRoman, isValidRoman } from "../../packages/romans/src/index.cp";

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
    test("1. @cap/roman-num — Classic years 1994, 2026 & 3999", () => {
        let buf: [16]u8 = undefined;
        const n1 = toRoman(1994, buf[0..16]);
        expect_eq(testEq(buf[0..n1], "MCMXCIV"), true);

        const n2 = toRoman(2026, buf[0..16]);
        expect_eq(testEq(buf[0..n2], "MMXXVI"), true);

        const n3 = toRoman(3999, buf[0..16]);
        expect_eq(testEq(buf[0..n3], "MMMCMXCIX"), true);
    });

    test("2. @cap/roman-num — All 6 subtractive pairs IV, IX, XL, XC, CD, CM", () => {
        expect_eq(parseRoman("IV"), 4 as i32);
        expect_eq(parseRoman("IX"), 9 as i32);
        expect_eq(parseRoman("XL"), 40 as i32);
        expect_eq(parseRoman("XC"), 90 as i32);
        expect_eq(parseRoman("CD"), 400 as i32);
        expect_eq(parseRoman("CM"), 900 as i32);
    });

    test("3. @cap/roman-num — Case-insensitive parsing", () => {
        expect_eq(parseRoman("mcmxciv"), 1994 as i32);
        expect_eq(parseRoman("MmXxVi"), 2026 as i32);
        expect_eq(isValidRoman("mmmcmxcix"), true);
    });

    test("4. @cap/roman-num — Exhaustive 1..3999 round-trip verification", () => {
        let buf: [16]u8 = undefined;
        let num: u16 = 1;
        while (num <= 3999) {
            const n = toRoman(num, buf[0..16]);
            expect_eq(n > 0, true);
            expect_eq(parseRoman(buf[0..n]), num as i32);
            num += 1;
        }
    });

    test("5. @cap/roman-num — Strict rejection of non-canonical forms & out-of-range values", () => {
        expect_eq(parseRoman("IIII"), -1 as i32);
        expect_eq(parseRoman("VV"), -1 as i32);
        expect_eq(parseRoman("IL"), -1 as i32);
        expect_eq(parseRoman("IC"), -1 as i32);
        expect_eq(parseRoman("XM"), -1 as i32);
        expect_eq(parseRoman(""), -1 as i32);
        expect_eq(isValidRoman("MMMM"), false);

        let buf: [16]u8 = undefined;
        expect_eq(toRoman(0, buf[0..16]), 0 as usize);
        expect_eq(toRoman(4000, buf[0..16]), 0 as usize);
    });

    test("6. @cap/roman-num — Undersized buffer protection", () => {
        let tiny: [3]u8 = undefined;
        // 3888 = "MMMDCCCLXXXVIII" (15 bytes)
        expect_eq(toRoman(3888, tiny[0..3]), 0 as usize);
        // 3 = "III" fits exactly in 3 bytes
        expect_eq(toRoman(3, tiny[0..3]), 3 as usize);
        expect_eq(testEq(tiny[0..3], "III"), true);
    });
}
