// cap/test/testCapFillRange.cp
// Comprehensive unit tests for @cap/fill-range (npm fill-range parity)

import { test, expect_eq } from "cap:test";
import {
    countRange,
    expandNumbers,
    expandChars,
    formatRange,
} from "../../packages/fill-range/src/index.cp";

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
    test("1. @cap/fill-range — ascending/descending numbers with step", () => {
        let out: [16]i64 = undefined;

        const n1: i64 = expandNumbers(1, 10, 2, out[0..16]);
        expect_eq(n1, 5 as i64);
        expect_eq(out[0], 1 as i64);
        expect_eq(out[1], 3 as i64);
        expect_eq(out[2], 5 as i64);
        expect_eq(out[3], 7 as i64);
        expect_eq(out[4], 9 as i64);

        // Descending with positive step auto-adjusts direction
        const n2: i64 = expandNumbers(10, 2, 3, out[0..16]);
        expect_eq(n2, 3 as i64);
        expect_eq(out[0], 10 as i64);
        expect_eq(out[1], 7 as i64);
        expect_eq(out[2], 4 as i64);

        // step == 0 defaults to 1
        const n3: i64 = expandNumbers(3, 5, 0, out[0..16]);
        expect_eq(n3, 3 as i64);
        expect_eq(out[0], 3 as i64);
        expect_eq(out[2], 5 as i64);
    });

    test("2. @cap/fill-range — negative numeric range -5..5..2", () => {
        let out: [16]i64 = undefined;
        const n: i64 = expandNumbers(-5, 5, 2, out[0..16]);
        expect_eq(n, 6 as i64);
        expect_eq(out[0], -5 as i64);
        expect_eq(out[1], -3 as i64);
        expect_eq(out[2], -1 as i64);
        expect_eq(out[3], 1 as i64);
        expect_eq(out[4], 3 as i64);
        expect_eq(out[5], 5 as i64);

        let dst: [64]u8 = undefined;
        const w: i64 = formatRange("-5", "5", 2, ",", dst[0..64]);
        expect_eq(w > 0, true);
        expect_eq(sliceEq(dst[0..(w as usize)], "-5,-3,-1,1,3,5"), true);
    });

    test("3. @cap/fill-range — zero-padded ranges '001'..'005' and '-02'..'02'", () => {
        let dst: [64]u8 = undefined;

        const w1: i64 = formatRange("001", "005", 1, ",", dst[0..64]);
        expect_eq(w1 > 0, true);
        expect_eq(sliceEq(dst[0..(w1 as usize)], "001,002,003,004,005"), true);

        const w2: i64 = formatRange("-02", "02", 1, ",", dst[0..64]);
        expect_eq(w2 > 0, true);
        expect_eq(sliceEq(dst[0..(w2 as usize)], "-02,-01,00,01,02"), true);
    });

    test("4. @cap/fill-range — uppercase/lowercase alphabetic ranges", () => {
        let chars: [16]u8 = undefined;
        const c1: i64 = expandChars('a' as u8, 'g' as u8, 1, chars[0..16]);
        expect_eq(c1, 7 as i64);
        expect_eq(sliceEq(chars[0..(c1 as usize)], "abcdefg"), true);

        const c2: i64 = expandChars('F' as u8, 'A' as u8, 2, chars[0..16]);
        expect_eq(c2, 3 as i64);
        expect_eq(sliceEq(chars[0..(c2 as usize)], "FDB"), true);

        let dst: [32]u8 = undefined;
        const w1: i64 = formatRange("a", "e", 1, ",", dst[0..32]);
        expect_eq(sliceEq(dst[0..(w1 as usize)], "a,b,c,d,e"), true);

        const w2: i64 = formatRange("F", "A", 2, "-", dst[0..32]);
        expect_eq(sliceEq(dst[0..(w2 as usize)], "F-D-B"), true);
    });

    test("5. @cap/fill-range — countRange O(1) accuracy across boundaries", () => {
        expect_eq(countRange(1, 10, 1), 10 as usize);
        expect_eq(countRange(1, 10, 2), 5 as usize);
        expect_eq(countRange(10, 1, 2), 5 as usize);
        expect_eq(countRange(10, 1, -2), 5 as usize);
        expect_eq(countRange(5, 5, 3), 1 as usize);
        expect_eq(countRange(-5, 5, 2), 6 as usize);
        expect_eq(countRange(0, 100, 0), 101 as usize);
    });

    test("6. @cap/fill-range — rejects invalid inputs & buffer overflow with -1", () => {
        let small_nums: [3]i64 = undefined;
        expect_eq(expandNumbers(1, 10, 1, small_nums[0..3]), -1 as i64);

        let chars: [8]u8 = undefined;
        expect_eq(expandChars('0' as u8, '9' as u8, 1, chars[0..8]), -1 as i64);
        expect_eq(expandChars('a' as u8, 'z' as u8, 1, chars[0..8]), -1 as i64);

        let tiny: [5]u8 = undefined;
        expect_eq(formatRange("01", "05", 1, ",", tiny[0..5]), -1 as i64);
        expect_eq(formatRange("", "5", 1, ",", tiny[0..5]), -1 as i64);
        expect_eq(formatRange("abc", "def", 1, ",", tiny[0..5]), -1 as i64);
        expect_eq(formatRange("a", "5", 1, ",", tiny[0..5]), -1 as i64);
    });
}
