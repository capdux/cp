// Source: cp/cap/test/test_lzw_production_deep.cp (Production deep tests for cap:lzw)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LZW_MAX_BITS,
    LZW_TABLE_SIZE,
    LZW_MIN_CODE_BITS,
    LZW_MAX_INITIAL_BITS,
    LzwBitOrder,
    lzwCompress,
    lzwDecompress,
    lzwCompressGif,
    lzwDecompressGif,
    lzwCompressTiff,
    lzwDecompressTiff,
} from "cap:lzw";

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

test("1. LSB-first (GIF) and MSB-first (TIFF/PDF) round-trip with KwKwK edge case and compression ratio", () => {
    expect_eq(LZW_MAX_BITS, 12 as u8);
    expect_eq(LZW_TABLE_SIZE, 4096 as usize);
    expect_eq(LZW_MIN_CODE_BITS, 2 as u8);
    expect_eq(LZW_MAX_INITIAL_BITS, 8 as u8);

    // Classic KwKwK trigger: long runs of "ABABAB..." and "AAAAAA..."
    let src: [512]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        src[i] = if ((i % 2) == 0) { 'A' as u8 } else { 'B' as u8 };
        i += 1;
    }
    while (i < 512) {
        src[i] = 'Z';
        i += 1;
    }

    let comp_arr: [1024]u8 = std.mem.zeroes([1024]u8);
    let comp_slice: []u8 = comp_arr[0..];
    let dec_arr: [512]u8 = std.mem.zeroes([512]u8);
    let dec_slice: []u8 = dec_arr[0..];

    // 1a. GIF LSB-first
    if (c_len := lzwCompressGif(src[0..], *comp_slice, 8)) {
        expect_eq(c_len < 120, true); // high compression ratio
        if (d_len := lzwDecompressGif(comp_arr[0..c_len], *dec_slice, 8)) {
            expect_eq(d_len, 512 as usize);
            expect_eq(slicesEq(dec_arr[0..d_len], src[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 1b. TIFF/PDF MSB-first
    if (c_len := lzwCompressTiff(src[0..], *comp_slice)) {
        expect_eq(c_len < 120, true);
        if (d_len := lzwDecompressTiff(comp_arr[0..c_len], *dec_slice)) {
            expect_eq(d_len, 512 as usize);
            expect_eq(slicesEq(dec_arr[0..d_len], src[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. Empty input and single-byte input in both LSB and MSB bit orders", () => {
    let comp_arr: [32]u8 = std.mem.zeroes([32]u8);
    let comp_slice: []u8 = comp_arr[0..];
    let dec_arr: [32]u8 = std.mem.zeroes([32]u8);
    let dec_slice: []u8 = dec_arr[0..];

    // 2a. Empty input (LSB & MSB)
    if (c_len := lzwCompressGif("", *comp_slice, 8)) {
        if (d_len := lzwDecompressGif(comp_arr[0..c_len], *dec_slice, 8)) {
            expect_eq(d_len, 0 as usize);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    if (c_len := lzwCompressTiff("", *comp_slice)) {
        if (d_len := lzwDecompressTiff(comp_arr[0..c_len], *dec_slice)) {
            expect_eq(d_len, 0 as usize);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 2b. Single-byte input
    let one: [1]u8 = [0x5A];
    if (c_len := lzwCompressGif(one[0..], *comp_slice, 8)) {
        if (d_len := lzwDecompressGif(comp_arr[0..c_len], *dec_slice, 8)) {
            expect_eq(d_len, 1 as usize);
            expect_eq(dec_arr[0], 0x5A as u8);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    if (c_len := lzwCompressTiff(one[0..], *comp_slice)) {
        if (d_len := lzwDecompressTiff(comp_arr[0..c_len], *dec_slice)) {
            expect_eq(d_len, 1 as usize);
            expect_eq(dec_arr[0], 0x5A as u8);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. Small lit_width (2-bit 4-color and 4-bit 16-color GIF palette index streams)", () => {
    let comp_arr: [1024]u8 = std.mem.zeroes([1024]u8);
    let comp_slice: []u8 = comp_arr[0..];
    let dec_arr: [512]u8 = std.mem.zeroes([512]u8);
    let dec_slice: []u8 = dec_arr[0..];

    // 3a. 2-bit palette (values 0..3, exercises 3->4->5->6->7->8-bit widening)
    let pal2: [300]u8 = undefined;
    let i: usize = 0;
    while (i < 300) {
        pal2[i] = ((i * 7 + (i / 13)) % 4) as u8;
        i += 1;
    }
    if (c_len := lzwCompressGif(pal2[0..], *comp_slice, 2)) {
        if (d_len := lzwDecompressGif(comp_arr[0..c_len], *dec_slice, 2)) {
            expect_eq(d_len, 300 as usize);
            expect_eq(slicesEq(dec_arr[0..d_len], pal2[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 3b. 4-bit palette (values 0..15)
    let pal4: [400]u8 = undefined;
    i = 0;
    while (i < 400) {
        pal4[i] = ((i * 11 + (i / 5)) % 16) as u8;
        i += 1;
    }
    if (c_len := lzwCompress(pal4[0..], *comp_slice, 4, LzwBitOrder.msb)) {
        if (d_len := lzwDecompress(comp_arr[0..c_len], *dec_slice, 4, LzwBitOrder.msb)) {
            expect_eq(d_len, 400 as usize);
            expect_eq(slicesEq(dec_arr[0..d_len], pal4[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. >4096 dictionary entries triggering 9->10->11->12-bit widening and automatic Clear reset (LSB & MSB)", () => {
    // 10000 bytes of pseudo-random LCG data fills 4096-entry dictionary multiple times
    let src: [10000]u8 = undefined;
    let seed: u32 = 0x12345678;
    let i: usize = 0;
    while (i < 10000) {
        seed = seed *% 1664525 +% 1013904223;
        src[i] = ((seed >> 16) & 0xFF) as u8;
        i += 1;
    }

    let comp_arr: [16384]u8 = std.mem.zeroes([16384]u8);
    let comp_slice: []u8 = comp_arr[0..];
    let dec_arr: [10000]u8 = std.mem.zeroes([10000]u8);
    let dec_slice: []u8 = dec_arr[0..];

    // 4a. LSB-first (GIF)
    if (c_len := lzwCompressGif(src[0..], *comp_slice, 8)) {
        if (d_len := lzwDecompressGif(comp_arr[0..c_len], *dec_slice, 8)) {
            expect_eq(d_len, 10000 as usize);
            expect_eq(slicesEq(dec_arr[0..d_len], src[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 4b. MSB-first (TIFF/PDF)
    if (c_len := lzwCompressTiff(src[0..], *comp_slice)) {
        if (d_len := lzwDecompressTiff(comp_arr[0..c_len], *dec_slice)) {
            expect_eq(d_len, 10000 as usize);
            expect_eq(slicesEq(dec_arr[0..d_len], src[0..]), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed input rejection: invalid lit_width, invalid code > next_code, output overflow, out-of-range symbol", () => {
    let comp_arr: [64]u8 = std.mem.zeroes([64]u8);
    let comp_slice: []u8 = comp_arr[0..];
    let dec_arr: [64]u8 = std.mem.zeroes([64]u8);
    let dec_slice: []u8 = dec_arr[0..];

    // 5a. Invalid lit_width (< 2 or > 8)
    if (_ := lzwCompressGif("abc", *comp_slice, 1)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    if (_ := lzwCompressGif("abc", *comp_slice, 9)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Symbol >= (1 << lit_width)
    let bad_sym: [2]u8 = [0, 4]; // 4 is out of range for lit_width=2 (max 3)
    if (_ := lzwCompressGif(bad_sym[0..], *comp_slice, 2)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Output buffer too small during decompression
    if (c_len := lzwCompressTiff("Hello, LZW world!", *comp_slice)) {
        let tiny_arr: [5]u8 = std.mem.zeroes([5]u8);
        let tiny_slice: []u8 = tiny_arr[0..];
        if (_ := lzwDecompressTiff(comp_arr[0..c_len], *tiny_slice)) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }

    // 5d. Invalid code > next_code (MSB 9-bit stream: Clear=256 (0x80 0x0), then invalid code 300 (> 258))
    // 256 (9b): 100000000
    // 300 (9b): 100101100 -> bits: 10000000 01001011 00000000 = [0x80, 0x4B, 0x00]
    let bad_code: [3]u8 = [0x80, 0x4B, 0x00];
    if (_ := lzwDecompressTiff(bad_code[0..], *dec_slice)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
