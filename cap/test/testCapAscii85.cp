// cap/test/testCapAscii85.cp
// Comprehensive unit tests for @cap/ascii85

import { test, expect_eq } from "cap:test";
import {
    encode,
    decode,
    maxEncodedLen,
} from "../../packages/ascii85/src/index.cp";

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
    test("1. @cap/ascii85 — Classic Adobe/Wikipedia vector 'Man ' -> '9jqo^' & sentence round-trip", () => {
        let enc: [64]u8 = undefined;
        const n1 = encode("Man ", false, enc[0..64]);
        expect_eq(sliceEq(enc[0..n1], "9jqo^"), true);

        let decBuf: [64]u8 = undefined;
        const m1 = decode("9jqo^", decBuf[0..64]);
        expect_eq(sliceEq(decBuf[0..(m1 as usize)], "Man "), true);

        const phrase = "Man is distinguished";
        const n2 = encode(phrase, false, enc[0..64]);
        const m2 = decode(enc[0..n2], decBuf[0..64]);
        expect_eq(sliceEq(decBuf[0..(m2 as usize)], phrase), true);
    });

    test("2. @cap/ascii85 — All-zero 4-byte block compresses to 'z' & decodes back", () => {
        let zeros: [4]u8 = undefined;
        zeros[0] = 0 as u8;
        zeros[1] = 0 as u8;
        zeros[2] = 0 as u8;
        zeros[3] = 0 as u8;

        let enc: [16]u8 = undefined;
        const n = encode(zeros[0..4], false, enc[0..16]);
        expect_eq(n, 1 as usize);
        expect_eq(enc[0], 'z' as u8);

        let decBuf: [16]u8 = undefined;
        const m = decode("z", decBuf[0..16]);
        expect_eq(m, 4 as i64);
        expect_eq(decBuf[0], 0 as u8);
        expect_eq(decBuf[3], 0 as u8);
    });

    test("3. @cap/ascii85 — Unaligned 1, 2, 3 byte tail blocks round-trip", () => {
        let enc: [32]u8 = undefined;
        let decBuf: [32]u8 = undefined;

        const n1 = encode("A", false, enc[0..32]);
        expect_eq(n1, 2 as usize);
        expect_eq(sliceEq(decBuf[0..(decode(enc[0..n1], decBuf[0..32]) as usize)], "A"), true);

        const n2 = encode("AB", false, enc[0..32]);
        expect_eq(n2, 3 as usize);
        expect_eq(sliceEq(decBuf[0..(decode(enc[0..n2], decBuf[0..32]) as usize)], "AB"), true);

        const n3 = encode("ABC", false, enc[0..32]);
        expect_eq(n3, 4 as usize);
        expect_eq(sliceEq(decBuf[0..(decode(enc[0..n3], decBuf[0..32]) as usize)], "ABC"), true);
    });

    test("4. @cap/ascii85 — <~...~> delimiters & embedded whitespace decoding", () => {
        let enc: [32]u8 = undefined;
        const n = encode("Man ", true, enc[0..32]);
        expect_eq(sliceEq(enc[0..n], "<~9jqo^~>"), true);

        let decBuf: [32]u8 = undefined;
        const m = decode("  <~ 9j\nqo^\t ~>  ", decBuf[0..32]);
        expect_eq(sliceEq(decBuf[0..(m as usize)], "Man "), true);
    });

    test("5. @cap/ascii85 — Reject mid-block 'z', u32 overflow ('s8W-\"' / 'uuuuu') & invalid chars", () => {
        let decBuf: [16]u8 = undefined;
        // 'z' inside a 5-char group is illegal.
        expect_eq(decode("9jzqo", decBuf[0..16]), -1 as i64);
        // 's8W-!' is exact u32::MAX (0xFFFFFFFF) -> valid; 's8W-"' is u32::MAX + 1 -> overflow!
        expect_eq(decode("s8W-!", decBuf[0..16]), 4 as i64);
        expect_eq(decBuf[0], 0xFF as u8);
        expect_eq(decBuf[3], 0xFF as u8);
        expect_eq(decode("s8W-\"", decBuf[0..16]), -1 as i64);
        expect_eq(decode("uuuuu", decBuf[0..16]), -1 as i64);
        // Invalid character 'v' (> 'u') and single-char trailing group.
        expect_eq(decode("9jqov", decBuf[0..16]), -1 as i64);
        expect_eq(decode("9", decBuf[0..16]), -1 as i64);
    });

    test("6. @cap/ascii85 — Empty input, maxEncodedLen & small buffer protection", () => {
        expect_eq(maxEncodedLen(0, false), 0 as usize);
        expect_eq(maxEncodedLen(0, true), 4 as usize);
        expect_eq(maxEncodedLen(4, false), 5 as usize);
        expect_eq(maxEncodedLen(5, true), 11 as usize);

        let enc: [16]u8 = undefined;
        expect_eq(encode("", false, enc[0..16]), 0 as usize);
        const ne = encode("", true, enc[0..16]);
        expect_eq(sliceEq(enc[0..ne], "<~~>"), true);

        let decBuf: [16]u8 = undefined;
        expect_eq(decode("<~~>", decBuf[0..16]), 0 as i64);

        let tiny: [2]u8 = undefined;
        expect_eq(encode("Man ", false, tiny[0..2]), 0 as usize);
        expect_eq(decode("9jqo^", tiny[0..2]), -1 as i64);
    });
}
