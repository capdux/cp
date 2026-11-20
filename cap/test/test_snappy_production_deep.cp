// Source: cp/cap/test/test_snappy_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:snappy & cap.snappy

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SnappyError,
    compress,
    decompress,
    decompressWithMaxSize,
    uncompressedLength,
} from "cap:snappy";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Empty input roundtrip
    const empty_comp = compress(alloc, "")?;
    defer alloc.free(empty_comp);
    expect_eq(empty_comp.len, 1 as usize);
    expect_eq(empty_comp[0], 0x00 as u8);
    expect_eq(uncompressedLength(empty_comp)?, 0 as usize);

    const empty_dec = decompress(alloc, empty_comp)?;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0 as usize);

    // 2. Single byte roundtrip
    const single = "A";
    const single_comp = compress(alloc, single)?;
    defer alloc.free(single_comp);
    expect_eq(uncompressedLength(single_comp)?, 1 as usize);

    const single_dec = decompress(alloc, single_comp)?;
    defer alloc.free(single_dec);
    expect_eq(std.mem.eql(u8, single_dec, single), true);

    // 3. Short text roundtrip
    const short_text = "The quick brown fox jumps over the lazy dog.";
    const short_comp = compress(alloc, short_text)?;
    defer alloc.free(short_comp);
    const short_dec = decompress(alloc, short_comp)?;
    defer alloc.free(short_dec);
    expect_eq(std.mem.eql(u8, short_dec, short_text), true);

    // 4. Highly repetitive long text (compression ratio < 20%)
    const pattern = "SnappyCompressionBenchmarkPattern_0123456789_";
    const repeat_count: usize = 200;
    const total_len = pattern.len * repeat_count;
    let long_buf = alloc.alloc(u8, total_len) catch unreachable;
    defer alloc.free(long_buf);

    let r: usize = 0;
    while (r < repeat_count) : (r += 1) {
        const start = r * pattern.len;
        @memcpy(long_buf[start..(start + pattern.len)], pattern);
    }

    const long_comp = compress(alloc, long_buf)?;
    defer alloc.free(long_comp);
    expect_eq(long_comp.len * 5 < total_len, true);
    expect_eq(uncompressedLength(long_comp)?, total_len);

    const long_dec = decompress(alloc, long_comp)?;
    defer alloc.free(long_dec);
    expect_eq(std.mem.eql(u8, long_dec, long_buf), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 1. Overlapping RLE copy (offset = 1, e.g. 4096 x 'Z')
    let rle_buf = alloc.alloc(u8, 4096) catch unreachable;
    defer alloc.free(rle_buf);
    @memset(rle_buf, 'Z');

    const rle_comp = compress(alloc, rle_buf)?;
    defer alloc.free(rle_comp);
    expect_eq(rle_comp.len < 256, true);

    const rle_dec = decompress(alloc, rle_comp)?;
    defer alloc.free(rle_dec);
    expect_eq(std.mem.eql(u8, rle_dec, rle_buf), true);

    // 2. Long literal (> 60 bytes extended length encoding) + incompressible pseudo-random data
    let rand_buf = alloc.alloc(u8, 2048) catch unreachable;
    defer alloc.free(rand_buf);
    let state: u32 = 0x9e3779b9;
    let i: usize = 0;
    while (i < rand_buf.len) : (i += 1) {
        state = state *% 1664525 +% 1013904223;
        rand_buf[i] = ((state >> 24) ^ (state >> 16) ^ (state >> 8)) as u8;
    }

    const rand_comp = compress(alloc, rand_buf)?;
    defer alloc.free(rand_comp);
    expect_eq(uncompressedLength(rand_comp)?, 2048 as usize);

    const rand_dec = decompress(alloc, rand_comp)?;
    defer alloc.free(rand_dec);
    expect_eq(std.mem.eql(u8, rand_dec, rand_buf), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 1. Hand-crafted Copy-1 vector:
    // Output: "ABCDABCD" (len=8)
    // Varint(8) = 0x08
    // Literal("ABCD", len=4): tag = (4-1)<<2 | 0 = 0x0c, "ABCD"
    // Copy-1(len=4, offset=4): tag = ((4-4)<<2) | ((4>>8)<<5) | 0x01 = 0x01, offset_low = 0x04
    const copy1_vec: [8]u8 = [0x08, 0x0c, 'A', 'B', 'C', 'D', 0x01, 0x04];
    const dec1 = decompress(alloc, &copy1_vec)?;
    defer alloc.free(dec1);
    expect_eq(std.mem.eql(u8, dec1, "ABCDABCD"), true);

    // 2. Hand-crafted Copy-2 vector with overlapping RLE (offset=1, len=5):
    // Output: "XXXXXX" (len=6)
    // Varint(6) = 0x06
    // Literal("X", len=1): tag = 0x00, 'X'
    // Copy-2(len=5, offset=1): tag = ((5-1)<<2) | 0x02 = 0x12, offset_le = 0x01, 0x00
    const copy2_vec: [6]u8 = [0x06, 0x00, 'X', 0x12, 0x01, 0x00];
    const dec2 = decompress(alloc, &copy2_vec)?;
    defer alloc.free(dec2);
    expect_eq(std.mem.eql(u8, dec2, "XXXXXX"), true);

    // 3. Hand-crafted Copy-4 vector:
    // Output: "HELLO_HELLO_" (len=12)
    // Varint(12) = 0x0c
    // Literal("HELLO_", len=6): tag = (6-1)<<2 | 0 = 0x14, "HELLO_"
    // Copy-4(len=6, offset=6): tag = ((6-1)<<2) | 0x03 = 0x17, offset_le32 = 0x06, 0x00, 0x00, 0x00
    const copy4_vec: [13]u8 = [0x0c, 0x14, 'H', 'E', 'L', 'L', 'O', '_', 0x17, 0x06, 0x00, 0x00, 0x00];
    const dec4 = decompress(alloc, &copy4_vec)?;
    defer alloc.free(dec4);
    expect_eq(std.mem.eql(u8, dec4, "HELLO_HELLO_"), true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. InvalidOffset: offset == 0 (Copy-1)
    const zero_off: [5]u8 = [0x05, 0x00, 'A', 0x01, 0x00];
    let got_zero_off = false;
    * = decompress(alloc, &zero_off) catch e => errZeroOff: {
        got_zero_off = (e == SnappyError.InvalidOffset);
        break :errZeroOff emptyMutSlice();
    };
    expect_eq(got_zero_off, true);

    // 2. InvalidOffset: offset > current_out_pos (Copy-2 offset=10 when out_pos=2)
    const oob_off: [7]u8 = [0x06, 0x04, 'A', 'B', 0x0e, 0x0a, 0x00];
    let got_oob_off = false;
    * = decompress(alloc, &oob_off) catch e => errOobOff: {
        got_oob_off = (e == SnappyError.InvalidOffset);
        break :errOobOff emptyMutSlice();
    };
    expect_eq(got_oob_off, true);

    // 3. TruncatedInput: literal claims 10 bytes, only 3 bytes remain
    const trunc_lit: [5]u8 = [0x0a, 0x24, 'A', 'B', 'C'];
    let got_trunc = false;
    * = decompress(alloc, &trunc_lit) catch e => errTrunc: {
        got_trunc = (e == SnappyError.TruncatedInput);
        break :errTrunc emptyMutSlice();
    };
    expect_eq(got_trunc, true);

    // 4. OutputTooLarge: forged large varint header (100 MB) with small max_bytes
    // 100 * 1024 * 1024 = 104857600 = 0x06400000 -> varint: 0x80, 0x80, 0x80, 0x32
    const huge_hdr: [4]u8 = [0x80, 0x80, 0x80, 0x32];
    expect_eq(uncompressedLength(&huge_hdr)?, 104857600 as usize);

    let got_too_large = false;
    * = decompressWithMaxSize(alloc, &huge_hdr, 1024 * 1024) catch e => errLarge: {
        got_too_large = (e == SnappyError.OutputTooLarge);
        break :errLarge emptyMutSlice();
    };
    expect_eq(got_too_large, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    const payload = "cap.snappy dynamic mount verification payload -- repeated repeated repeated!";
    const comp = cap.snappy.compress(alloc, payload)?;
    defer alloc.free(comp);

    const len = cap.snappy.uncompressedLength(comp)?;
    expect_eq(len, payload.len);

    const dec_out = cap.snappy.decompressWithMaxSize(alloc, comp, 4096)?;
    defer alloc.free(dec_out);
    expect_eq(std.mem.eql(u8, dec_out, payload), true);

    const dec2 = cap.snappy.decompress(alloc, comp)?;
    defer alloc.free(dec2);
    expect_eq(std.mem.eql(u8, dec2, payload), true);
}

test("1. Empty input, single byte, short text, and highly repetitive long text (< 20% ratio) roundtrip", () => {
    runTest1() catch unreachable;
});

test("2. Overlapping RLE copy (offset=1), long literal (> 60B), and incompressible pseudo-random roundtrip", () => {
    runTest2() catch unreachable;
});

test("3. Hand-crafted Copy-1, Copy-2, and Copy-4 standard vector decoding", () => {
    runTest3() catch unreachable;
});

test("4. Malformed input guards: InvalidOffset (0 & OOB), TruncatedInput, and OutputTooLarge", () => {
    runTest4() catch unreachable;
});

test("5. import cap from \"cap\"; cap.snappy dynamic mount and std.testing.allocator zero leak", () => {
    runTest5() catch unreachable;
});
