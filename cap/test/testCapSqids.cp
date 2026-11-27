// cap/test/testCapSqids.cp
// Comprehensive unit tests for @cap/sqids:
// 1. Single-number & multi-number lossless round-trip
// 2. Boundary values (0, u64::MAX, large combinations)
// 3. min_length padding & exact round-trip decoding
// 4. Custom alphabet & option validation (too short / duplicate / non-ASCII)
// 5. Profanity blocklist avoidance with increment retry & lossless round-trip
// 6. Invalid character / empty / out-of-alphabet decode safety (returns 0 / null)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_ALPHABET,
    Sqids,
    SqidsOptions,
    SqidsError,
} from "../../packages/sqids/src/index.cp";

fn main() {
    test("1. @cap/sqids — Single & Multi-Number Lossless Round-Trip", () => {
        const sqids = Sqids.initDefault();
        expect_eq(sqids.isValid(), true);

        let buf: [128]u8 = undefined;
        const id1 = sqids.encodeOne(42, buf[0..]);
        expect_eq(id1.len > 0, true);
        if (v1 := sqids.decodeOne(id1)) {
            expect_eq(v1, 42 as u64);
        } else {
            expect_eq(false, true);
        }

        // Deterministic output for identical inputs
        let buf2: [128]u8 = undefined;
        const id1_again = sqids.encodeOne(42, buf2[0..]);
        expect_eq(std.mem.eql(u8, id1, id1_again), true);

        // Multi-number array round-trip
        const nums: [5]u64 = [1, 2, 3, 100, 999999];
        const multi_id = sqids.encode(nums[0..], buf[0..]);
        expect_eq(multi_id.len > 0, true);

        let decoded: [8]u64 = undefined;
        const count = sqids.decode(multi_id, decoded[0..]);
        expect_eq(count, 5 as usize);
        expect_eq(decoded[0], 1 as u64);
        expect_eq(decoded[1], 2 as u64);
        expect_eq(decoded[2], 3 as u64);
        expect_eq(decoded[3], 100 as u64);
        expect_eq(decoded[4], 999999 as u64);

        // decodeOne returns null when ID encodes multiple numbers
        expect_eq(sqids.decodeOne(multi_id) == null, true);
    });

    test("2. @cap/sqids — Zero & u64::MAX Boundary Values", () => {
        const sqids = Sqids.initDefault();
        const max_u64: u64 = std.math.maxInt(u64);
        let buf: [256]u8 = undefined;
        let out: [4]u64 = undefined;

        // Zero
        const id_zero = sqids.encodeOne(0, buf[0..]);
        expect_eq(id_zero.len > 0, true);
        if (z := sqids.decodeOne(id_zero)) {
            expect_eq(z, 0 as u64);
        } else {
            expect_eq(false, true);
        }

        // u64::MAX
        const id_max = sqids.encodeOne(max_u64, buf[0..]);
        expect_eq(id_max.len > 0, true);
        if (m := sqids.decodeOne(id_max)) {
            expect_eq(m, max_u64);
        } else {
            expect_eq(false, true);
        }

        // Mixed [0, u64::MAX, 0, 1]
        const edge_nums: [4]u64 = [0, max_u64, 0, 1];
        const id_edge = sqids.encode(edge_nums[0..], buf[0..]);
        const cnt = sqids.decode(id_edge, out[0..]);
        expect_eq(cnt, 4 as usize);
        expect_eq(out[0], 0 as u64);
        expect_eq(out[1], max_u64);
        expect_eq(out[2], 0 as u64);
        expect_eq(out[3], 1 as u64);
    });

    test("3. @cap/sqids — Minimum Length Padding & Exact Decoding", () => {
        let buf: [256]u8 = undefined;
        let out: [8]u64 = undefined;

        // Test various min_lengths including exact +1 boundary and long padding (40)
        const lengths: [5]u8 = [3, 4, 8, 16, 40];
        let li: usize = 0;
        while (li < lengths.len) : (li += 1) {
            const min_len = lengths[li];
            const sq = Sqids.init(SqidsOptions{
                alphabet: DEFAULT_ALPHABET,
                min_length: min_len,
            });
            expect_eq(sq.isValid(), true);

            const id_single = sq.encodeOne(7, buf[0..]);
            expect_eq(id_single.len >= @as(usize, min_len), true);
            if (v := sq.decodeOne(id_single)) {
                expect_eq(v, 7 as u64);
            } else {
                expect_eq(false, true);
            }

            const pair: [2]u64 = [12, 345];
            const id_pair = sq.encode(pair[0..], buf[0..]);
            expect_eq(id_pair.len >= @as(usize, min_len), true);
            const cnt = sq.decode(id_pair, out[0..]);
            expect_eq(cnt, 2 as usize);
            expect_eq(out[0], 12 as u64);
            expect_eq(out[1], 345 as u64);
        }
    });

    test("4. @cap/sqids — Custom Alphabet & Validation Guards", () => {
        // Minimal 3-char alphabet works losslessly
        const tiny = Sqids.init(SqidsOptions{ alphabet: "abc", min_length: 0 });
        expect_eq(tiny.isValid(), true);
        let buf: [256]u8 = undefined;
        let out: [4]u64 = undefined;
        const nums: [3]u64 = [0, 5, 42];
        const id_tiny = tiny.encode(nums[0..], buf[0..]);
        expect_eq(id_tiny.len > 0, true);
        const cnt = tiny.decode(id_tiny, out[0..]);
        expect_eq(cnt, 3 as usize);
        expect_eq(out[0], 0 as u64);
        expect_eq(out[1], 5 as u64);
        expect_eq(out[2], 42 as u64);

        // Hex alphabet
        const hex = Sqids.init(SqidsOptions{ alphabet: "0123456789ABCDEF", min_length: 6 });
        expect_eq(hex.isValid(), true);
        const id_hex = hex.encodeOne(2026, buf[0..]);
        expect_eq(id_hex.len >= 6, true);
        if (hv := hex.decodeOne(id_hex)) {
            expect_eq(hv, 2026 as u64);
        } else {
            expect_eq(false, true);
        }

        // Invalid options: too short, duplicate chars
        const bad_short = Sqids.init(SqidsOptions{ alphabet: "ab", min_length: 0 });
        expect_eq(bad_short.isValid(), false);
        expect_eq(bad_short.err == SqidsError.AlphabetTooShort, true);

        const bad_dup = Sqids.init(SqidsOptions{ alphabet: "abca", min_length: 0 });
        expect_eq(bad_dup.isValid(), false);
        expect_eq(bad_dup.err == SqidsError.AlphabetDuplicateChar, true);
    });

    test("5. @cap/sqids — Blocklist Avoidance & Lossless Round-Trip", () => {
        // Construct an alphabet containing letters of blocked words and verify
        // generated IDs never contain blocked words and still decode cleanly.
        const sq = Sqids.init(SqidsOptions{ alphabet: "fuckshitab", min_length: 4 });
        expect_eq(sq.isValid(), true);

        let buf: [64]u8 = undefined;
        let n: u64 = 0;
        while (n < 60) : (n += 1) {
            const id = sq.encodeOne(n, buf[0..]);
            expect_eq(id.len >= 4, true);

            // Must not contain blocked 4-letter words
            expect_eq(std.mem.indexOf(u8, id, "fuck") == null, true);
            expect_eq(std.mem.indexOf(u8, id, "shit") == null, true);

            if (decoded_val := sq.decodeOne(id)) {
                expect_eq(decoded_val, n);
            } else {
                expect_eq(false, true);
            }
        }
    });

    test("6. @cap/sqids — Invalid Characters & Empty Input Safety", () => {
        const sqids = Sqids.initDefault();
        let out: [8]u64 = undefined;

        // Empty string
        expect_eq(sqids.decode("", out[0..]), 0 as usize);
        expect_eq(sqids.decodeOne("") == null, true);

        // Characters outside default alphanumeric alphabet
        expect_eq(sqids.decode("abc-def", out[0..]), 0 as usize);
        expect_eq(sqids.decode("hello!", out[0..]), 0 as usize);
        expect_eq(sqids.decode("123 456", out[0..]), 0 as usize);
        expect_eq(sqids.decodeOne("$$$") == null, true);

        // Empty numbers slice returns empty string
        let buf: [32]u8 = undefined;
        let dummy: [1]u64 = [0];
        expect_eq(sqids.encode(dummy[0..0], buf[0..]).len, 0 as usize);
    });
}
