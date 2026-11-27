// cap/test/testCapNanoid.cp
// Comprehensive unit tests for @cap/nanoid

import { test, expect_eq } from "cap:test";
import {
    URL_ALPHABET,
    DEFAULT_SIZE,
    NanoIdGenerator,
    nanoidFromBytes,
    customAlphabetFromBytes,
    nanoidSeeded,
    isValidNanoId,
} from "../../packages/nanoid/src/index.cp";

fn main() {
    test("1. @cap/nanoid — constants & deterministic nanoidFromBytes (& 63 mapping)", () => {
        expect_eq(URL_ALPHABET.len, 64 as usize);
        expect_eq(DEFAULT_SIZE, 21 as usize);

        // 0 -> 'u', 1 -> 's', 63 -> 't', 64 (64 & 63 == 0) -> 'u', 127 (127 & 63 == 63) -> 't'
        let raw: [5]u8 = [0, 1, 63, 64, 127];
        let out: [5]u8 = [0; 5];
        let n = nanoidFromBytes(raw[0..5], 5, out[0..5]);
        expect_eq(n, 5 as usize);
        expect_eq(out[0..5], "ustut");
    });

    test("2. @cap/nanoid — customAlphabetFromBytes bitmask rejection sampling eliminates bias", () => {
        // Alphabet len 10 -> mask = 15 (0b1111).
        // Bytes 10..15 have (b & 15) >= 10 and MUST be rejected.
        // Bytes 16..18 have (b & 15) == 0, 1, 2 -> '0', '1', '2'.
        let raw: [10]u8 = [0, 9, 10, 11, 15, 16, 17, 18, 31, 5];
        let out: [6]u8 = [0; 6];
        let n = customAlphabetFromBytes("0123456789", raw[0..10], 6, out[0..6]);
        expect_eq(n, 6 as usize);
        expect_eq(out[0..6], "090125");
    });

    test("3. @cap/nanoid — nanoidSeeded reproducibility & seed separation", () => {
        let a: [21]u8 = [0; 21];
        let b: [21]u8 = [0; 21];
        let c: [21]u8 = [0; 21];

        let na = nanoidSeeded(123456789, DEFAULT_SIZE, a[0..21]);
        let nb = nanoidSeeded(123456789, DEFAULT_SIZE, b[0..21]);
        let nc = nanoidSeeded(987654321, DEFAULT_SIZE, c[0..21]);

        expect_eq(na, 21 as usize);
        expect_eq(nb, 21 as usize);
        expect_eq(nc, 21 as usize);
        expect_eq(a[0..21], b[0..21]);
        expect_eq(isValidNanoId(a[0..21], ""), true);
        expect_eq(isValidNanoId(c[0..21], URL_ALPHABET), true);

        // Different seeds produce different IDs
        let same = true;
        let i: usize = 0;
        while (i < 21) {
            if (a[i] != c[i]) {
                same = false;
                break;
            }
            i = i + 1;
        }
        expect_eq(same, false);
    });

    test("4. @cap/nanoid — NanoIdGenerator.nextInto produces distinct sequential IDs", () => {
        let gen = NanoIdGenerator.new(42);
        let id1: [21]u8 = [0; 21];
        let id2: [21]u8 = [0; 21];
        let id3: [21]u8 = [0; 21];

        expect_eq(gen.nextInto(DEFAULT_SIZE, id1[0..21]), 21 as usize);
        expect_eq(gen.nextInto(DEFAULT_SIZE, id2[0..21]), 21 as usize);
        expect_eq(gen.nextInto(DEFAULT_SIZE, id3[0..21]), 21 as usize);

        expect_eq(isValidNanoId(id1[0..21], ""), true);
        expect_eq(isValidNanoId(id2[0..21], ""), true);
        expect_eq(isValidNanoId(id3[0..21], ""), true);

        let diff12 = false;
        let diff23 = false;
        let i: usize = 0;
        while (i < 21) {
            if (id1[i] != id2[i]) { diff12 = true; }
            if (id2[i] != id3[i]) { diff23 = true; }
            i = i + 1;
        }
        expect_eq(diff12, true);
        expect_eq(diff23, true);
    });

    test("5. @cap/nanoid — NanoIdGenerator.nextCustomInto with hex, numeric & single-char alphabets", () => {
        let gen = NanoIdGenerator.new(0);
        let hex_buf: [16]u8 = [0; 16];
        let num_buf: [10]u8 = [0; 10];
        let one_buf: [4]u8 = [0; 4];

        let nh = gen.nextCustomInto("0123456789abcdef", 16, hex_buf[0..16]);
        let nn = gen.nextCustomInto("0123456789", 10, num_buf[0..10]);
        let no = gen.nextCustomInto("x", 4, one_buf[0..4]);

        expect_eq(nh, 16 as usize);
        expect_eq(nn, 10 as usize);
        expect_eq(no, 4 as usize);

        expect_eq(isValidNanoId(hex_buf[0..16], "0123456789abcdef"), true);
        expect_eq(isValidNanoId(num_buf[0..10], "0123456789"), true);
        expect_eq(one_buf[0..4], "xxxx");
    });

    test("6. @cap/nanoid — isValidNanoId & empty/small-buffer edge cases", () => {
        expect_eq(isValidNanoId("V1StGXR8_Z5jdHi6B-myT", ""), true);
        expect_eq(isValidNanoId("invalid+char/here=", ""), false);
        expect_eq(isValidNanoId("", ""), false);
        expect_eq(isValidNanoId("012345", "0123456789"), true);
        expect_eq(isValidNanoId("01234a", "0123456789"), false);

        let small: [4]u8 = [0; 4];
        let n = nanoidSeeded(7, 21, small[0..4]);
        expect_eq(n, 4 as usize);
        expect_eq(isValidNanoId(small[0..4], ""), true);

        let empty_out: [4]u8 = [0; 4];
        expect_eq(customAlphabetFromBytes("", small[0..4], 4, empty_out[0..4]), 0 as usize);
    });
}
