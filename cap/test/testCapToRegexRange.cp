import { test, expect_eq } from "cap:test";
import {
    toRegexRange,
    matchNumericRange,
    countDigits,
} from "../../packages/@cap/to-regex-range/src/index.cp";

fn eqStr(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/to-regex-range — single value, single-digit & countDigits", () => {
        let buf: [128]u8 = [0; 128];

        let n1: usize = toRegexRange(5, 5, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n1, "5"), true);

        let n2: usize = toRegexRange(-7, -7, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n2, "-7"), true);

        let n3: usize = toRegexRange(0, 9, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n3, "[0-9]"), true);

        let n4: usize = toRegexRange(1, 9, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n4, "[1-9]"), true);

        expect_eq(countDigits(0), 1 as usize);
        expect_eq(countDigits(9), 1 as usize);
        expect_eq(countDigits(100), 3 as usize);
        expect_eq(countDigits(1234567890), 10 as usize);
    });

    test("2. @cap/to-regex-range — multi-digit spans 1..100 & 15..95", () => {
        let buf: [128]u8 = [0; 128];

        let n1: usize = toRegexRange(1, 100, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n1, "[1-9]|[1-9][0-9]|100"), true);

        let n2: usize = toRegexRange(15, 95, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n2, "1[5-9]|[2-8][0-9]|9[0-5]"), true);

        let n3: usize = toRegexRange(10, 99, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n3, "[1-9][0-9]"), true);
    });

    test("3. @cap/to-regex-range — mixed negative-to-positive range -10..10", () => {
        let buf: [128]u8 = [0; 128];
        let n1: usize = toRegexRange(-10, 10, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n1, "-([1-9]|10)|[0-9]|10"), true);

        let n2: usize = toRegexRange(-5, 5, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n2, "-[1-5]|[0-5]"), true);
    });

    test("4. @cap/to-regex-range — pure negative range -99..-10 & -100..-1", () => {
        let buf: [128]u8 = [0; 128];
        let n1: usize = toRegexRange(-99, -10, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n1, "-[1-9][0-9]"), true);

        let n2: usize = toRegexRange(-100, -1, buf[0..128]);
        expect_eq(eqStr(buf[0..128], n2, "-([1-9]|[1-9][0-9]|100)"), true);
    });

    test("5. @cap/to-regex-range — reversed bounds auto-normalized", () => {
        let buf1: [128]u8 = [0; 128];
        let buf2: [128]u8 = [0; 128];
        let n1: usize = toRegexRange(10, 1, buf1[0..128]);
        let n2: usize = toRegexRange(1, 10, buf2[0..128]);
        expect_eq(n1, n2);
        expect_eq(eqStr(buf1[0..128], n1, "[1-9]|10"), true);

        let n3: usize = toRegexRange(5, -5, buf1[0..128]);
        expect_eq(eqStr(buf1[0..128], n3, "-[1-5]|[0-5]"), true);
    });

    test("6. @cap/to-regex-range — matchNumericRange & small buffer safety", () => {
        expect_eq(matchNumericRange("-10", -10, 10), true);
        expect_eq(matchNumericRange("0", -10, 10), true);
        expect_eq(matchNumericRange("10", -10, 10), true);
        expect_eq(matchNumericRange("11", -10, 10), false);
        expect_eq(matchNumericRange("-11", -10, 10), false);
        expect_eq(matchNumericRange("5", 10, 1), true);
        expect_eq(matchNumericRange("", -10, 10), false);
        expect_eq(matchNumericRange("-", -10, 10), false);
        expect_eq(matchNumericRange("-0", -10, 10), false);
        expect_eq(matchNumericRange("01", 0, 10), false);
        expect_eq(matchNumericRange("12a", 0, 100), false);

        // Buffer too small returns 0 safely
        let tiny: [4]u8 = [0; 4];
        expect_eq(toRegexRange(1, 100, tiny[0..4]), 0 as usize);
    });
}
