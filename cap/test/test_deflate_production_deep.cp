// Source: cp/cap/test/test_deflate_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:deflate & cap.deflate

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DeflateError,
    crc32,
    adler32,
    compressDeflate,
    decompressDeflate,
    decompressDeflateWithMaxSize,
    compressGzip,
    decompressGzip,
    compressZlib,
    decompressZlib,
} from "cap:deflate";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Empty input roundtrip
    const empty_comp = compressDeflate(alloc, "")?;
    defer alloc.free(empty_comp);
    const empty_dec = decompressDeflate(alloc, empty_comp)?;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0 as usize);

    // 2. Short text roundtrip
    const short_text = "The quick brown fox jumps over the lazy dog.";
    const short_comp = compressDeflate(alloc, short_text)?;
    defer alloc.free(short_comp);
    const short_dec = decompressDeflate(alloc, short_comp)?;
    defer alloc.free(short_dec);
    expect_eq(std.mem.eql(u8, short_dec, short_text), true);

    // 3. High-repetition text (compression ratio < 25%)
    const pattern = "DeflateCompressionBenchmarkPattern_0123456789_";
    const repeat_count: usize = 200;
    const total_len = pattern.len * repeat_count;
    let long_buf = alloc.alloc(u8, total_len) catch unreachable;
    defer alloc.free(long_buf);

    let r: usize = 0;
    while (r < repeat_count) : (r += 1) {
        const start = r * pattern.len;
        @memcpy(long_buf[start..(start + pattern.len)], pattern);
    }

    const long_comp = compressDeflate(alloc, long_buf)?;
    defer alloc.free(long_comp);
    expect_eq(long_comp.len * 4 < total_len, true);

    const long_dec = decompressDeflate(alloc, long_comp)?;
    defer alloc.free(long_dec);
    expect_eq(std.mem.eql(u8, long_dec, long_buf), true);

    // 4. Stored Block (BTYPE=00) standard vector: "Hello"
    const stored_vec: [10]u8 = [0x01, 0x05, 0x00, 0xfa, 0xff, 'H', 'e', 'l', 'l', 'o'];
    const stored_dec = decompressDeflate(alloc, &stored_vec)?;
    defer alloc.free(stored_dec);
    expect_eq(std.mem.eql(u8, stored_dec, "Hello"), true);

    // 5. Fixed Huffman (BTYPE=01) overlapping RLE vector: "XXXXXX"
    // BFINAL=1 (1), BTYPE=01 (01), lit 'X' (0x58 -> 8-bit 0x88 -> rev 0x11?):
    // Let's use standard Fixed Huffman for "AAAAAA":
    // BFINAL=1, BTYPE=01 -> 3 bits: 011
    // 'A'=65 -> 8-bit code 0x30+65 = 0x71 (01110001), reversed = 10001110
    // len=5 (sym 259 -> 7-bit 0000011, rev 1100000), dist=1 (sym 0 -> 5-bit 00000), EOB (sym 256 -> 7-bit 0000000)
    // Total: 011 | 10001110 | 1100000 | 00000 | 0000000 -> 30 bits:
    // Byte 0: 01110_011 = 0x73, Byte 1: 11_100011? Wait: bit order LSB-first:
    // bits 0..2: 1,1,0 (0x03)
    // bits 3..10: 'A' rev(0x71)=0x8e -> 0x8e << 3 = 0x470 | 0x03 = 0x473 -> b0=0x73, b1_low3=0x04
    // bits 11..17: sym 259 (3) rev7(3)=0x60 -> 0x60 << 3 = 0x300 | 0x04 -> b1=0x04, b2_low2=0x03
    // bits 18..22: dist 0 (00000)
    // bits 23..29: EOB (0000000) -> b2=0x03, b3=0x00
    const fixed_rle_vec: [4]u8 = [0x73, 0x04, 0x03, 0x00];
    const fixed_rle_dec = decompressDeflate(alloc, &fixed_rle_vec)?;
    defer alloc.free(fixed_rle_dec);
    expect_eq(std.mem.eql(u8, fixed_rle_dec, "AAAAAA"), true);

    // 6. Dynamic Huffman (BTYPE=10) standard vector
    const dyn_vec: [92]u8 = [
        0xb5, 0xcb, 0xd9, 0x15, 0x40, 0x30, 0x14, 0x45, 0xd1, 0x56, 0xae, 0x06, 0x2c, 0xf3, 0xd0, 0x85,
        0x0f, 0x0d, 0x04, 0x41, 0x4c, 0x8f, 0x90, 0x20, 0xd5, 0x7b, 0x4d, 0xf8, 0x3e, 0xfb, 0xd4, 0xa3,
        0xc4, 0x61, 0x54, 0x3b, 0xa3, 0xd1, 0x74, 0x6f, 0xe8, 0xe9, 0xc1, 0x64, 0xd6, 0xfd, 0x04, 0x59,
        0xa9, 0x71, 0x71, 0x5e, 0x84, 0x7b, 0xd1, 0xd1, 0xe0, 0xa3, 0xfe, 0x0d, 0x57, 0x82, 0xdd, 0xfa,
        0xa2, 0x61, 0x74, 0xab, 0x6b, 0x44, 0xaf, 0xac, 0xe4, 0xe4, 0xe4, 0x86, 0x45, 0x1d, 0x86, 0x34,
        0xbf, 0xc3, 0xe9, 0x21, 0x08, 0xa3, 0x38, 0x49, 0xb3, 0xbc, 0x28, 0x3f,
    ];
    const dyn_expected = "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. Pack my box with five dozen liquor jugs! 0123456789";
    const dyn_dec = decompressDeflate(alloc, &dyn_vec)?;
    defer alloc.free(dyn_dec);
    expect_eq(std.mem.eql(u8, dyn_dec, dyn_expected), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 1. GZIP roundtrip (empty & payload)
    const gz_empty = compressGzip(alloc, "")?;
    defer alloc.free(gz_empty);
    const gz_empty_dec = decompressGzip(alloc, gz_empty)?;
    defer alloc.free(gz_empty_dec);
    expect_eq(gz_empty_dec.len, 0 as usize);

    const payload = "GZIP container roundtrip payload -- repeated repeated repeated!";
    const gz = compressGzip(alloc, payload)?;
    defer alloc.free(gz);
    expect_eq(gz[0], 0x1f as u8);
    expect_eq(gz[1], 0x8b as u8);
    expect_eq(gz[2], 0x08 as u8);
    const gz_dec = decompressGzip(alloc, gz)?;
    defer alloc.free(gz_dec);
    expect_eq(std.mem.eql(u8, gz_dec, payload), true);

    // 2. GZIP with FNAME header vector decoding ("hello.txt" -> "gzip with fname header payload")
    const gz_fname_vec: [60]u8 = [
        0x1f, 0x8b, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2e,
        0x74, 0x78, 0x74, 0x00, 0x4b, 0xaf, 0xca, 0x2c, 0x50, 0x28, 0xcf, 0x2c, 0xc9, 0x50, 0x48, 0xcb,
        0x4b, 0xcc, 0x4d, 0x55, 0xc8, 0x48, 0x4d, 0x4c, 0x49, 0x2d, 0x52, 0x28, 0x48, 0xac, 0xcc, 0xc9,
        0x4f, 0x4c, 0x01, 0x00, 0xf6, 0xce, 0xbf, 0x20, 0x1e, 0x00, 0x00, 0x00,
    ];
    const gz_fname_dec = decompressGzip(alloc, &gz_fname_vec)?;
    defer alloc.free(gz_fname_dec);
    expect_eq(std.mem.eql(u8, gz_fname_dec, "gzip with fname header payload"), true);

    // 3. Tamper CRC32 -> ChecksumMismatch
    let gz_bad_crc = alloc.dupe(u8, gz) catch unreachable;
    defer alloc.free(gz_bad_crc);
    gz_bad_crc[gz_bad_crc.len - 8] ^= 0xff;
    let got_crc_err = false;
    * = decompressGzip(alloc, gz_bad_crc) catch e => errCrc: {
        got_crc_err = (e == DeflateError.ChecksumMismatch);
        break :errCrc emptyMutSlice();
    };
    expect_eq(got_crc_err, true);

    // 4. Tamper ISIZE -> ChecksumMismatch
    let gz_bad_isize = alloc.dupe(u8, gz) catch unreachable;
    defer alloc.free(gz_bad_isize);
    gz_bad_isize[gz_bad_isize.len - 1] ^= 0x01;
    let got_isize_err = false;
    * = decompressGzip(alloc, gz_bad_isize) catch e => errIsize: {
        got_isize_err = (e == DeflateError.ChecksumMismatch);
        break :errIsize emptyMutSlice();
    };
    expect_eq(got_isize_err, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 1. ZLIB roundtrip
    const payload = "ZLIB RFC 1950 roundtrip verification -- repeated repeated repeated!";
    const zl = compressZlib(alloc, payload)?;
    defer alloc.free(zl);
    expect_eq(zl[0], 0x78 as u8);
    expect_eq(zl[1], 0x9c as u8);
    const zl_dec = decompressZlib(alloc, zl)?;
    defer alloc.free(zl_dec);
    expect_eq(std.mem.eql(u8, zl_dec, payload), true);

    // 2. Invalid CMF/FLG header -> InvalidHeader
    let zl_bad_hdr = alloc.dupe(u8, zl) catch unreachable;
    defer alloc.free(zl_bad_hdr);
    zl_bad_hdr[1] ^= 0x01; // breaks (cmf * 256 + flg) % 31 == 0
    let got_hdr_err = false;
    * = decompressZlib(alloc, zl_bad_hdr) catch e => errHdr: {
        got_hdr_err = (e == DeflateError.InvalidHeader);
        break :errHdr emptyMutSlice();
    };
    expect_eq(got_hdr_err, true);

    // 3. Tampered Adler-32 -> ChecksumMismatch
    let zl_bad_adler = alloc.dupe(u8, zl) catch unreachable;
    defer alloc.free(zl_bad_adler);
    zl_bad_adler[zl_bad_adler.len - 1] ^= 0xff;
    let got_adler_err = false;
    * = decompressZlib(alloc, zl_bad_adler) catch e => errAdler: {
        got_adler_err = (e == DeflateError.ChecksumMismatch);
        break :errAdler emptyMutSlice();
    };
    expect_eq(got_adler_err, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. InvalidBlockType (BTYPE=11): BFINAL=1, BTYPE=11 -> 0x07
    const bad_btype: [1]u8 = [0x07];
    let got_btype_err = false;
    * = decompressDeflate(alloc, &bad_btype) catch e => errBtype: {
        got_btype_err = (e == DeflateError.InvalidBlockType);
        break :errBtype emptyMutSlice();
    };
    expect_eq(got_btype_err, true);

    // 2. InvalidDistance: Fixed Huffman match with dist=1 when output is empty (0 bytes)
    // BFINAL=1, BTYPE=01 (0x03 -> wait: sym 257 (0000001 rev 1000000), dist 0 (00000))
    // bits 0..2: 1,1,0; bits 3..9: 1,0,0,0,0,0,0; bits 10..14: 0,0,0,0,0 -> 0x0b, 0x00
    const bad_dist: [2]u8 = [0x0b, 0x00];
    let got_dist_err = false;
    * = decompressDeflate(alloc, &bad_dist) catch e => errDist: {
        got_dist_err = (e == DeflateError.InvalidDistance);
        break :errDist emptyMutSlice();
    };
    expect_eq(got_dist_err, true);

    // 3. UnexpectedEof: truncated stored block
    const trunc_stored: [4]u8 = [0x01, 0x05, 0x00, 0xfa];
    let got_eof_err = false;
    * = decompressDeflate(alloc, &trunc_stored) catch e => errEof: {
        got_eof_err = (e == DeflateError.UnexpectedEof);
        break :errEof emptyMutSlice();
    };
    expect_eq(got_eof_err, true);

    // 4. OutputTooLarge: anti-bomb protection via decompressDeflateWithMaxSize
    let big_input = alloc.alloc(u8, 4096) catch unreachable;
    defer alloc.free(big_input);
    @memset(big_input, 'B');
    const big_comp = compressDeflate(alloc, big_input)?;
    defer alloc.free(big_comp);

    let got_bomb_err = false;
    * = decompressDeflateWithMaxSize(alloc, big_comp, 1024) catch e => errBomb: {
        got_bomb_err = (e == DeflateError.OutputTooLarge);
        break :errBomb emptyMutSlice();
    };
    expect_eq(got_bomb_err, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Standard IEEE 802.3 CRC-32 & RFC 1950 Adler-32 vectors for "123456789"
    expect_eq(cap.deflate.crc32("123456789"), 0xcbf43926 as u32);
    expect_eq(cap.deflate.adler32("123456789"), 0x091e01de as u32);

    const payload = "cap.deflate dynamic mount verification payload -- repeated repeated repeated!";
    const comp = cap.deflate.compressDeflate(alloc, payload)?;
    defer alloc.free(comp);
    const dec_out = cap.deflate.decompressDeflateWithMaxSize(alloc, comp, 4096)?;
    defer alloc.free(dec_out);
    expect_eq(std.mem.eql(u8, dec_out, payload), true);

    const gz = cap.deflate.compressGzip(alloc, payload)?;
    defer alloc.free(gz);
    const gz_out = cap.deflate.decompressGzip(alloc, gz)?;
    defer alloc.free(gz_out);
    expect_eq(std.mem.eql(u8, gz_out, payload), true);

    const zl = cap.deflate.compressZlib(alloc, payload)?;
    defer alloc.free(zl);
    const zl_out = cap.deflate.decompressZlib(alloc, zl)?;
    defer alloc.free(zl_out);
    expect_eq(std.mem.eql(u8, zl_out, payload), true);
}

test("1. Raw DEFLATE empty, short, high-repetition (< 25% ratio), and Stored/Fixed/Dynamic Huffman vectors", () => {
    runTest1() catch unreachable;
});

test("2. GZIP roundtrip, FNAME extended header decoding, and CRC32/ISIZE ChecksumMismatch guards", () => {
    runTest2() catch unreachable;
});

test("3. ZLIB roundtrip, InvalidHeader CMF/FLG guard, and Adler-32 ChecksumMismatch guard", () => {
    runTest3() catch unreachable;
});

test("4. Malformed stream guards: InvalidBlockType, InvalidDistance, UnexpectedEof, and OutputTooLarge", () => {
    runTest4() catch unreachable;
});

test("5. import cap from \"cap\"; cap.deflate dynamic mount, CRC32/Adler32 vectors, and zero leak", () => {
    runTest5() catch unreachable;
});
