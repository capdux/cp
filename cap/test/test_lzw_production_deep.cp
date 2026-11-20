// Source: cp/cap/test/test_lzw_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:lzw & cap.lzw

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    LzwError,
    LzwBitOrder,
    LzwConfig,
    compress,
    decompress,
    decompressWithMaxSize,
} from "cap:lzw";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn verifyRoundtrip(alloc: std.mem.Allocator, input: []const u8, cfg: LzwConfig): !void {
    const comp = compress(alloc, input, cfg)?;
    defer alloc.free(comp);
    const dec_out = decompress(alloc, comp, cfg)?;
    defer alloc.free(dec_out);
    expect_eq(std.mem.eql(u8, dec_out, input), true);
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    const lsb_cfg = LzwConfig{ min_code_size: 8, bit_order: LzwBitOrder.LsbFirst, max_code_size: 12 };
    const msb_cfg = LzwConfig{ min_code_size: 8, bit_order: LzwBitOrder.MsbFirst, max_code_size: 12 };

    // 1. Empty input
    verifyRoundtrip(alloc, "", lsb_cfg)?;
    verifyRoundtrip(alloc, "", msb_cfg)?;

    // 2. Single byte
    verifyRoundtrip(alloc, "Z", lsb_cfg)?;
    verifyRoundtrip(alloc, "\x00", msb_cfg)?;

    // 3. High-repetition text
    const rep = "LZW_HIGH_REPETITION_PATTERN_0123456789_LZW_HIGH_REPETITION_PATTERN_0123456789_LZW_HIGH_REPETITION_PATTERN!";
    verifyRoundtrip(alloc, rep, lsb_cfg)?;
    verifyRoundtrip(alloc, rep, msb_cfg)?;

    // 4. Pseudo-random data (LCG)
    let rnd_buf = alloc.alloc(u8, 1024) catch unreachable;
    defer alloc.free(rnd_buf);
    let state: u32 = 0x12345678;
    let i: usize = 0;
    while (i < rnd_buf.len) : (i += 1) {
        state = state *% 1664525 +% 1013904223;
        rnd_buf[i] = ((state >> 16) & 0xff) as u8;
    }
    verifyRoundtrip(alloc, rnd_buf, lsb_cfg)?;
    verifyRoundtrip(alloc, rnd_buf, msb_cfg)?;
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const lsb_cfg = LzwConfig{ min_code_size: 8, bit_order: LzwBitOrder.LsbFirst, max_code_size: 12 };
    const msb_cfg = LzwConfig{ min_code_size: 8, bit_order: LzwBitOrder.MsbFirst, max_code_size: 12 };

    // 1. Classic KwKwK sequences: "abababa..." and long run of 'A'
    const ab_pat = "abababababababababababababababababababababababababababababababab";
    verifyRoundtrip(alloc, ab_pat, lsb_cfg)?;
    verifyRoundtrip(alloc, ab_pat, msb_cfg)?;

    let aaa_buf = alloc.alloc(u8, 512) catch unreachable;
    defer alloc.free(aaa_buf);
    @memset(aaa_buf, 'A');
    verifyRoundtrip(alloc, aaa_buf, lsb_cfg)?;
    verifyRoundtrip(alloc, aaa_buf, msb_cfg)?;

    // 2. Large pseudo-random + structured payload (> 16KB) to force > 4096 dict entries and ClearCode resets
    let big_buf = alloc.alloc(u8, 16384) catch unreachable;
    defer alloc.free(big_buf);
    let seed: u32 = 0x9e3779b9;
    let i: usize = 0;
    while (i < big_buf.len) : (i += 1) {
        if ((i % 16) < 4) {
            big_buf[i] = ((i / 16) & 0xff) as u8;
        } else {
            seed = seed *% 1103515245 +% 12345;
            big_buf[i] = ((seed >> 16) & 0xff) as u8;
        }
    }
    verifyRoundtrip(alloc, big_buf, lsb_cfg)?;
    verifyRoundtrip(alloc, big_buf, msb_cfg)?;
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;
    const gif_2bit_lsb = LzwConfig{ min_code_size: 2, bit_order: LzwBitOrder.LsbFirst, max_code_size: 12 };
    const gif_2bit_msb = LzwConfig{ min_code_size: 2, bit_order: LzwBitOrder.MsbFirst, max_code_size: 12 };

    // 2-bit pixel indices (0..3)
    let pixels = alloc.alloc(u8, 600) catch unreachable;
    defer alloc.free(pixels);
    let i: usize = 0;
    while (i < pixels.len) : (i += 1) {
        pixels[i] = ((i * 7 + (i / 13)) % 4) as u8;
    }

    verifyRoundtrip(alloc, pixels, gif_2bit_lsb)?;
    verifyRoundtrip(alloc, pixels, gif_2bit_msb)?;
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;
    const valid_cfg = LzwConfig{ min_code_size: 8, bit_order: LzwBitOrder.LsbFirst, max_code_size: 12 };

    // 1. Invalid min_code_size = 1 and 9
    const bad_cfg_1 = LzwConfig{ min_code_size: 1, bit_order: LzwBitOrder.LsbFirst, max_code_size: 12 };
    let got_min_1 = false;
    * = compress(alloc, "abc", bad_cfg_1) catch e => errMin1: {
        got_min_1 = (e == LzwError.InvalidMinCodeSize);
        break :errMin1 emptyMutSlice();
    };
    expect_eq(got_min_1, true);

    const bad_cfg_9 = LzwConfig{ min_code_size: 9, bit_order: LzwBitOrder.LsbFirst, max_code_size: 12 };
    let got_min_9 = false;
    * = decompress(alloc, "abc", bad_cfg_9) catch e => errMin9: {
        got_min_9 = (e == LzwError.InvalidMinCodeSize);
        break :errMin9 emptyMutSlice();
    };
    expect_eq(got_min_9, true);

    // 2. InvalidCode: first code >= clear_code (e.g. code 300 in 9-bit LSB: 0x2c, 0x01)
    // 9-bit code 300 = 0b1_0010_1100 -> byte0 = 0x2c, byte1 = 0x01 (plus EOI 257)
    const bad_code_stream: [3]u8 = [0x2c, 0x01, 0x00];
    let got_invalid_code = false;
    * = decompress(alloc, &bad_code_stream, valid_cfg) catch e => errBadCode: {
        got_invalid_code = (e == LzwError.InvalidCode);
        break :errBadCode emptyMutSlice();
    };
    expect_eq(got_invalid_code, true);

    // 3. MissingEoiCode / TruncatedInput on truncated compressed stream
    const sample = "Hello LZW truncation test!";
    const comp = compress(alloc, sample, valid_cfg)?;
    defer alloc.free(comp);

    let got_trunc_or_eoi = false;
    * = decompress(alloc, comp[0..(comp.len - 1)], valid_cfg) catch e => errTrunc: {
        got_trunc_or_eoi = (e == LzwError.TruncatedInput || e == LzwError.MissingEoiCode);
        break :errTrunc emptyMutSlice();
    };
    expect_eq(got_trunc_or_eoi, true);

    // Empty compressed stream -> MissingEoiCode
    let got_missing_eoi = false;
    * = decompress(alloc, "", valid_cfg) catch e => errEmpty: {
        got_missing_eoi = (e == LzwError.MissingEoiCode);
        break :errEmpty emptyMutSlice();
    };
    expect_eq(got_missing_eoi, true);

    // 4. OutputTooLarge anti-bomb protection
    let bomb_in = alloc.alloc(u8, 4096) catch unreachable;
    defer alloc.free(bomb_in);
    @memset(bomb_in, 'X');
    const bomb_comp = compress(alloc, bomb_in, valid_cfg)?;
    defer alloc.free(bomb_comp);

    let got_bomb = false;
    * = decompressWithMaxSize(alloc, bomb_comp, valid_cfg, 512) catch e => errBomb: {
        got_bomb = (e == LzwError.OutputTooLarge);
        break :errBomb emptyMutSlice();
    };
    expect_eq(got_bomb, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;
    const cfg = cap.lzw.LzwConfig{
        min_code_size: 8,
        bit_order: cap.lzw.LzwBitOrder.MsbFirst,
        max_code_size: 12,
    };

    const payload = "cap.lzw dynamic mount verification payload -- abababababab -- zero leak!";
    const comp = cap.lzw.compress(alloc, payload, cfg)?;
    defer alloc.free(comp);

    const dec_out = cap.lzw.decompressWithMaxSize(alloc, comp, cfg, 4096)?;
    defer alloc.free(dec_out);
    expect_eq(std.mem.eql(u8, dec_out, payload), true);
}

test("1. LsbFirst (GIF) and MsbFirst (TIFF/PDF) empty, single-byte, repetitive, and random roundtrips", () => {
    runTest1() catch unreachable;
});

test("2. Classic KwKwK sequences and > 4096 dictionary overflow ClearCode reset roundtrips", () => {
    runTest2() catch unreachable;
});

test("3. Small alphabet min_code_size=2 (2-bit image indices 0..3) roundtrip", () => {
    runTest3() catch unreachable;
});

test("4. Error guards: InvalidMinCodeSize, InvalidCode, MissingEoiCode/TruncatedInput, and OutputTooLarge", () => {
    runTest4() catch unreachable;
});

test("5. import cap from \"cap\"; cap.lzw dynamic mount and std.testing.allocator zero-leak check", () => {
    runTest5() catch unreachable;
});
