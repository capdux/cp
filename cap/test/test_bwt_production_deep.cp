// Source: cp/cap/test/test_bwt_production_deep.cp (Production deep tests for cap:bwt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BwtResult,
    bwtEncode,
    bwtDecode,
    mtfEncode,
    mtfDecode,
    bwtMtfEncode,
    bwtMtfDecode,
    zeroByteRatioPermille,
} from "cap:pack";

fn slicesEq(a: []const u8, b: []const u8): bool {
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

test("1. Classic BWT vectors ('banana' -> 'nnbaaa', primary_index=3 and '^BANANA|') exact forward & inverse", () => {
    let dst_arr: [32]u8 = std.mem.zeroes([32]u8);
    let dst: []u8 = dst_arr[0..];
    let rec_arr: [32]u8 = std.mem.zeroes([32]u8);
    let rec: []u8 = rec_arr[0..];
    let scr_arr: [32]u32 = std.mem.zeroes([32]u32);
    let scr: []u32 = scr_arr[0..];

    // 1a. "banana" -> "nnbaaa", primary_index = 3
    if (res := bwtEncode("banana", *dst, *scr)) {
        expect_eq(res.len, 6 as usize);
        expect_eq(res.primary_index, 3 as usize);
        expect_eq(slicesEq(dst_arr[0..6], "nnbaaa"), true);

        if (dlen := bwtDecode(dst_arr[0..6], res.primary_index, *rec, *scr)) {
            expect_eq(dlen, 6 as usize);
            expect_eq(slicesEq(rec_arr[0..6], "banana"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 1b. "^BANANA|" (ASCII: 'A'=65, 'B'=66, 'N'=78, '^'=94, '|'=124)
    // Sorted rotations:
    //   0: "A|^BANAN" -> 'N'
    //   1: "ANA|^BAN" -> 'N'
    //   2: "ANANA|^B" -> 'B'
    //   3: "BANANA|^" -> '^'
    //   4: "NA|^BANA" -> 'A'
    //   5: "NANA|^BA" -> 'A'
    //   6: "^BANANA|" -> '|' (primary_index = 6)
    //   7: "|^BANANA" -> 'A'
    // Output: "NNB^AA|A", primary_index = 6
    if (res := bwtEncode("^BANANA|", *dst, *scr)) {
        expect_eq(res.len, 8 as usize);
        expect_eq(res.primary_index, 6 as usize);
        expect_eq(slicesEq(dst_arr[0..8], "NNB^AA|A"), true);

        if (dlen := bwtDecode(dst_arr[0..8], res.primary_index, *rec, *scr)) {
            expect_eq(dlen, 8 as usize);
            expect_eq(slicesEq(rec_arr[0..8], "^BANANA|"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. Move-To-Front (MTF) exact round-trip and zero-run generation on repeated symbols", () => {
    const input = "aaabbbcccdddaaa";
    let enc_arr: [32]u8 = std.mem.zeroes([32]u8);
    let enc: []u8 = enc_arr[0..];
    let dec_arr: [32]u8 = std.mem.zeroes([32]u8);
    let out_dec: []u8 = dec_arr[0..];

    if (elen := mtfEncode(input, *enc)) {
        expect_eq(elen, input.len);
        // 'a' (97), 0, 0, 'b' (98), 0, 0, 'c' (99), 0, 0, 'd' (100), 0, 0, 'a' (3), 0, 0
        expect_eq(enc_arr[0], 'a' as u8);
        expect_eq(enc_arr[1], 0 as u8);
        expect_eq(enc_arr[2], 0 as u8);
        expect_eq(enc_arr[12], 3 as u8);
        expect_eq(enc_arr[13], 0 as u8);
        expect_eq(enc_arr[14], 0 as u8);

        if (dlen := mtfDecode(enc_arr[0..elen], *out_dec)) {
            expect_eq(dlen, input.len);
            expect_eq(slicesEq(dec_arr[0..dlen], input), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. Combined BWT + MTF pipeline achieves high zeroByteRatioPermille on repetitive English text and lossless decode", () => {
    const text = "the quick brown fox jumps over the lazy dog. the quick brown fox jumps over the lazy dog. the quick brown fox jumps over the lazy dog. the quick brown fox jumps over the lazy dog. ";
    const n = text.len;

    let enc_arr: [256]u8 = std.mem.zeroes([256]u8);
    let enc: []u8 = enc_arr[0..];
    let tmp_arr: [256]u8 = std.mem.zeroes([256]u8);
    let tmp: []u8 = tmp_arr[0..];
    let dec_arr: [256]u8 = std.mem.zeroes([256]u8);
    let out_dec: []u8 = dec_arr[0..];
    let scr_arr: [256]u32 = std.mem.zeroes([256]u32);
    let scr: []u32 = scr_arr[0..];

    expect_eq(zeroByteRatioPermille(text), 0 as u32);

    if (res := bwtMtfEncode(text, *enc, *tmp, *scr)) {
        expect_eq(res.len, n);
        const zero_permille = zeroByteRatioPermille(enc_arr[0..res.len]);
        // 4x repeated English sentence clusters into >70% zero bytes after BWT+MTF!
        expect_eq(zero_permille >= 700, true);

        if (dlen := bwtMtfDecode(enc_arr[0..res.len], res.primary_index, *out_dec, *tmp, *scr)) {
            expect_eq(dlen, n);
            expect_eq(slicesEq(dec_arr[0..dlen], text), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. Edge cases: empty input, single byte, all-identical bytes, and full 256-byte alphabet round-trip", () => {
    let dst_arr: [300]u8 = std.mem.zeroes([300]u8);
    let dst: []u8 = dst_arr[0..];
    let tmp_arr: [300]u8 = std.mem.zeroes([300]u8);
    let tmp: []u8 = tmp_arr[0..];
    let dec_arr: [300]u8 = std.mem.zeroes([300]u8);
    let out_dec: []u8 = dec_arr[0..];
    let scr_arr: [300]u32 = std.mem.zeroes([300]u32);
    let scr: []u32 = scr_arr[0..];

    // 4a. Empty
    if (res := bwtMtfEncode("", *dst, *tmp, *scr)) {
        expect_eq(res.len, 0 as usize);
        expect_eq(res.primary_index, 0 as usize);
        expect_eq(zeroByteRatioPermille(""), 0 as u32);
        if (dlen := bwtMtfDecode(dst_arr[0..0], 0, *out_dec, *tmp, *scr)) {
            expect_eq(dlen, 0 as usize);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 4b. Single byte
    let one: [1]u8 = [0xFF];
    if (res := bwtMtfEncode(one[0..], *dst, *tmp, *scr)) {
        expect_eq(res.len, 1 as usize);
        expect_eq(res.primary_index, 0 as usize);
        if (dlen := bwtMtfDecode(dst_arr[0..1], res.primary_index, *out_dec, *tmp, *scr)) {
            expect_eq(dlen, 1 as usize);
            expect_eq(dec_arr[0], 0xFF as u8);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 4c. All-identical bytes (64 x 'Q')
    let same: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        same[i] = 'Q';
        i += 1;
    }
    if (res := bwtMtfEncode(same[0..], *dst, *tmp, *scr)) {
        // 63 of 64 bytes are 0 -> 984 permille
        expect_eq(zeroByteRatioPermille(dst_arr[0..64]), 984 as u32);
        if (dlen := bwtMtfDecode(dst_arr[0..64], res.primary_index, *out_dec, *tmp, *scr)) {
            expect_eq(dlen, 64 as usize);
            expect_eq(slicesEq(dec_arr[0..64], same[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 4d. All 256 distinct byte values (0x00..0xFF reversed)
    let all256: [256]u8 = undefined;
    i = 0;
    while (i < 256) {
        all256[i] = (255 - i) as u8;
        i += 1;
    }
    if (res := bwtMtfEncode(all256[0..], *dst, *tmp, *scr)) {
        if (dlen := bwtMtfDecode(dst_arr[0..256], res.primary_index, *out_dec, *tmp, *scr)) {
            expect_eq(dlen, 256 as usize);
            expect_eq(slicesEq(dec_arr[0..256], all256[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed input rejection: primary_index >= src.len and undersized output/scratch buffers", () => {
    let dst_arr: [16]u8 = std.mem.zeroes([16]u8);
    let dst: []u8 = dst_arr[0..];
    let scr_arr: [16]u32 = std.mem.zeroes([16]u32);
    let scr: []u32 = scr_arr[0..];

    // 5a. primary_index >= src.len
    if (_ := bwtDecode("nnbaaa", 6, *dst, *scr)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Non-zero primary_index on empty input
    if (_ := bwtDecode("", 1, *dst, *scr)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Undersized dst / scratch buffers
    let tiny_u8: [3]u8 = std.mem.zeroes([3]u8);
    let tiny_dst: []u8 = tiny_u8[0..];
    let tiny_u32: [3]u32 = std.mem.zeroes([3]u32);
    let tiny_scr: []u32 = tiny_u32[0..];

    if (_ := bwtEncode("banana", *tiny_dst, *scr)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    if (_ := bwtEncode("banana", *dst, *tiny_scr)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    if (_ := mtfEncode("banana", *tiny_dst)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    if (_ := mtfDecode("banana", *tiny_dst)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
