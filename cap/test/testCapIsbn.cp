// cap/test/testCapIsbn.cp
// Comprehensive unit tests for @cap/isbn

import { test, expect_eq } from "cap:test";
import {
    stripHyphens,
    isValidIsbn10,
    isValidIsbn13,
    isValidIssn,
    isValidEan13,
    isValidIsbn,
    isbn10To13,
    isbn13To10,
    computeIsbn13CheckDigit,
} from "../../packages/isbn/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/isbn — ISBN-10 validation including trailing 'X'/'x'", () => {
        expect_eq(isValidIsbn10("0-306-40615-2"), true);
        expect_eq(isValidIsbn10("0306406152"), true);
        expect_eq(isValidIsbn10("0-8044-2957-X"), true);
        expect_eq(isValidIsbn10("0 8044 2957 x"), true);
        expect_eq(isValidIsbn("0-8044-2957-X"), true);
        expect_eq(isValidIsbn10("0-306-40615-3"), false);
        expect_eq(isValidIsbn10("X-306-40615-2"), false);
    });

    test("2. @cap/isbn — ISBN-13 validation for 978 & 979 prefixes", () => {
        expect_eq(isValidIsbn13("978-0-306-40615-7"), true);
        expect_eq(isValidIsbn13("979-10-90636-07-1"), true);
        expect_eq(isValidIsbn("9791090636071"), true);
        expect_eq(computeIsbn13CheckDigit("978-0-306-40615"), '7' as u8);
        expect_eq(computeIsbn13CheckDigit("979-10-90636-07"), '1' as u8);
        // Valid EAN-13 with non-Bookland prefix 400 is NOT a valid ISBN-13.
        expect_eq(isValidIsbn13("4006381333931"), false);
    });

    test("3. @cap/isbn — ISBN-10 <-> ISBN-13 conversion & 979 rejection", () => {
        let out13: [13]u8 = undefined;
        const n13 = isbn10To13("0-306-40615-2", out13[0..13]);
        expect_eq(n13, 13 as usize);
        expect_eq(sliceEq(out13[0..13], "9780306406157"), true);

        let out10: [10]u8 = undefined;
        const n10 = isbn13To10("978-0-306-40615-7", out10[0..10]);
        expect_eq(n10, 10 as usize);
        expect_eq(sliceEq(out10[0..10], "0306406152"), true);

        // Conversion producing 'X' check digit: 978-0-8044-2957-3 -> 080442957X
        let out13x: [13]u8 = undefined;
        expect_eq(isbn10To13("0-8044-2957-X", out13x[0..13]), 13 as usize);
        expect_eq(sliceEq(out13x[0..13], "9780804429573"), true);
        let out10x: [10]u8 = undefined;
        expect_eq(isbn13To10("9780804429573", out10x[0..10]), 10 as usize);
        expect_eq(sliceEq(out10x[0..10], "080442957X"), true);

        // 979 prefix cannot convert to ISBN-10.
        expect_eq(isbn13To10("979-10-90636-07-1", out10[0..10]), 0 as usize);
    });

    test("4. @cap/isbn — ISSN validation including trailing 'X'/'x'", () => {
        expect_eq(isValidIssn("0317-8471"), true);
        expect_eq(isValidIssn("2434-561X"), true);
        expect_eq(isValidIssn("2434 561x"), true);
        expect_eq(isValidIssn("0317-8472"), false);
        expect_eq(isValidIssn("0317-847"), false);
    });

    test("5. @cap/isbn — General EAN-13 / GTIN-13 validation", () => {
        expect_eq(isValidEan13("4006381333931"), true);
        expect_eq(isValidEan13("5901234123457"), true);
        expect_eq(isValidEan13("4006381333932"), false);
    });

    test("6. @cap/isbn — stripHyphens & tampered / malformed input rejection", () => {
        let stripped: [16]u8 = undefined;
        const slen = stripHyphens(" 0-8044-2957-x ", stripped[0..16]);
        expect_eq(slen, 10 as usize);
        expect_eq(sliceEq(stripped[0..slen], "080442957X"), true);

        expect_eq(isValidIsbn(""), false);
        expect_eq(isValidIsbn("97803064061570"), false);
        expect_eq(isValidIsbn("978-0-306-40615-8"), false);
        expect_eq(computeIsbn13CheckDigit("97803064061"), 0 as u8);
        expect_eq(computeIsbn13CheckDigit("97803064061X"), 0 as u8);
    });
}
