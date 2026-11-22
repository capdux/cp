// Source: cp/cap/test/test_brotli_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:brotli & cap.brotli

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BrotliError,
    BrotliHeader,
    parseHeader,
    windowSizeFromBits,
    compress,
    compressUncompressed,
    decompress,
    decompressWithLimits,
} from "../../lib/pack/brotli_.cp";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Empty input roundtrip -> canonical 1-byte [0x06] for WBITS=16
    const empty_comp = compress(alloc, "")?;
    defer alloc.free(empty_comp);
    expect_eq(empty_comp.len, 1 as usize);
    expect_eq(empty_comp[0], 0x06 as u8);

    const empty_dec = decompress(alloc, empty_comp)?;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0 as usize);

    // 2. Single-byte roundtrip
    const single_comp = compress(alloc, "A")?;
    defer alloc.free(single_comp);
    const single_dec = decompress(alloc, single_comp)?;
    defer alloc.free(single_dec);
    expect_eq(std.mem.eql(u8, single_dec, "A"), true);

    // 3. Short text roundtrip
    const short_text = "The quick brown fox jumps over the lazy dog.";
    const short_comp = compress(alloc, short_text)?;
    defer alloc.free(short_comp);
    const short_dec = decompress(alloc, short_comp)?;
    defer alloc.free(short_dec);
    expect_eq(std.mem.eql(u8, short_dec, short_text), true);

    // 4. Long payload (> 64KB, exercises 5-nibble MNIBBLES=1 framing)
    const total_len: usize = 80000;
    let long_buf = alloc.alloc(u8, total_len) catch unreachable;
    defer alloc.free(long_buf);
    let i: usize = 0;
    while (i < total_len) : (i += 1) {
        long_buf[i] = ((i * 31 + 7) & 0xff) as u8;
    }

    const long_comp = compress(alloc, long_buf)?;
    defer alloc.free(long_comp);
    const long_dec = decompress(alloc, long_comp)?;
    defer alloc.free(long_dec);
    expect_eq(std.mem.eql(u8, long_dec, long_buf), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const sample = "BrotliWindowBitsVerificationPayload_0123456789";

    // 1. Full WBITS range 10..24 roundtrip & header validation
    let wb: u8 = 10;
    while (wb <= 24) : (wb += 1) {
        const comp = compressUncompressed(alloc, sample, wb)?;
        defer alloc.free(comp);

        const hdr: BrotliHeader = parseHeader(comp)?;
        expect_eq(hdr.window_bits, wb);
        const expected_wsize = windowSizeFromBits(wb)?;
        expect_eq(hdr.window_size, expected_wsize);

        const decoded = decompress(alloc, comp)?;
        defer alloc.free(decoded);
        expect_eq(std.mem.eql(u8, decoded, sample), true);
    }

    // 2. Out-of-range WBITS rejected
    let got_invalid_low = false;
    * = compressUncompressed(alloc, sample, 9) catch e => errLow: {
        got_invalid_low = (e == BrotliError.InvalidWindowBits);
        break :errLow emptyMutSlice();
    };
    expect_eq(got_invalid_low, true);

    let got_invalid_high = false;
    * = compressUncompressed(alloc, sample, 25) catch e => errHigh: {
        got_invalid_high = (e == BrotliError.InvalidWindowBits);
        break :errHigh emptyMutSlice();
    };
    expect_eq(got_invalid_high, true);

    // 3. Metadata skip block (MNIBBLES=3, MSKIPBITS=1, 3 bytes "XYZ") + "OK" + ISLASTEMPTY
    const meta_stream: [11]u8 = [
        0x2c, 0x01, 0x58, 0x59, 0x5a,
        0x08, 0x00, 0x08, 0x4f, 0x4b,
        0x03,
    ];
    const meta_dec = decompress(alloc, &meta_stream)?;
    defer alloc.free(meta_dec);
    expect_eq(std.mem.eql(u8, meta_dec, "OK"), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // Handcrafted RFC 7932 Compressed Meta-Block vector: "AAAA"
    // WBITS=16 (0), ISLAST=1 (1), ISLASTEMPTY=0 (0), MNIBBLES=0 (00), MLEN-1=3,
    // Simple 1-symbol Literal='A', Simple 1-symbol IAC=32 (insert=4, copy=2), Simple 1-symbol Dist=0.
    const compressed_aaaa: [9]u8 = [
        0x62, 0x00, 0x00, 0x00, 0x44, 0x50, 0x80, 0x10, 0x00,
    ];
    const dec_aaaa = decompress(alloc, &compressed_aaaa)?;
    defer alloc.free(dec_aaaa);
    expect_eq(std.mem.eql(u8, dec_aaaa, "AAAA"), true);

    // Standard RFC 7932 empty stream with WBITS=24:
    // WBITS=24 (1 + 111 = 0x0f), ISLAST=1 (bit4=1), ISLASTEMPTY=1 (bit5=1) -> 0x3f
    const empty_w24: [1]u8 = [0x3f];
    const hdr24 = parseHeader(&empty_w24)?;
    expect_eq(hdr24.window_bits, 24 as u8);
    expect_eq(hdr24.window_size, 16777200 as usize);
    const dec_w24 = decompress(alloc, &empty_w24)?;
    defer alloc.free(dec_w24);
    expect_eq(dec_w24.len, 0 as usize);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. Empty slice -> UnexpectedEof
    let got_empty_eof = false;
    * = decompress(alloc, "") catch e => errEmpty: {
        got_empty_eof = (e == BrotliError.UnexpectedEof);
        break :errEmpty emptyMutSlice();
    };
    expect_eq(got_empty_eof, true);

    // 2. Truncated stream -> UnexpectedEof
    const payload = "TruncatedPayloadTestString";
    const full_comp = compressUncompressed(alloc, payload, 16)?;
    defer alloc.free(full_comp);
    let got_trunc_eof = false;
    * = decompress(alloc, full_comp[0..5]) catch e => errTrunc: {
        got_trunc_eof = (e == BrotliError.UnexpectedEof);
        break :errTrunc emptyMutSlice();
    };
    expect_eq(got_trunc_eof, true);

    // 3. WindowTooLarge guard
    const large_win_comp = compressUncompressed(alloc, payload, 20)?;
    defer alloc.free(large_win_comp);
    let got_win_too_large = false;
    * = decompressWithLimits(alloc, large_win_comp, 4096, 65536) catch e => errWin: {
        got_win_too_large = (e == BrotliError.WindowTooLarge);
        break :errWin emptyMutSlice();
    };
    expect_eq(got_win_too_large, true);

    // 4. OutputTooLarge guard
    let big_buf: [1024]u8 = [0x42; 1024];
    const big_comp = compress(alloc, &big_buf)?;
    defer alloc.free(big_comp);
    let got_out_too_large = false;
    * = decompressWithLimits(alloc, big_comp, 1048576, 512) catch e => errOut: {
        got_out_too_large = (e == BrotliError.OutputTooLarge);
        break :errOut emptyMutSlice();
    };
    expect_eq(got_out_too_large, true);

    // 5. Reserved WBITS=9 -> InvalidWindowBits (bit0=1, n=000, m=001 => 0x11)
    const bad_wbits: [2]u8 = [0x11, 0x00];
    let got_bad_wbits = false;
    * = decompress(alloc, &bad_wbits) catch e => errBadWb: {
        got_bad_wbits = (e == BrotliError.InvalidWindowBits);
        break :errBadWb emptyMutSlice();
    };
    expect_eq(got_bad_wbits, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;
    const msg = "Dynamic cap.brotli mount verification with zero-leak allocator!";

    const enc = compress(alloc, msg)?;
    defer alloc.free(enc);

    const hdr = parseHeader(enc)?;
    expect_eq(hdr.window_bits, 16 as u8);

    const decoded = decompressWithLimits(alloc, enc, 131072, 4096)?;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, msg), true);
}

test("cap:brotli roundtrip: empty, single-byte, short text, and >64KB multi-nibble chunk", () => {
    runTest1() catch unreachable;
});

test("cap:brotli WBITS 10..24 header validation, invalid WBITS rejection, and metadata skip block", () => {
    runTest2() catch unreachable;
});

test("cap:brotli handcrafted RFC 7932 compressed meta-block & WBITS=24 empty vector", () => {
    runTest3() catch unreachable;
});

test("cap:brotli bomb & corruption guards: UnexpectedEof, WindowTooLarge, OutputTooLarge, InvalidWindowBits", () => {
    runTest4() catch unreachable;
});

test("cap.brotli dynamic hub mount and std.testing.allocator zero-leak check", () => {
    runTest5() catch unreachable;
});
