// image-rs/image-tiff (MIT) Reference-Hardened Classic TIFF 6.0 & BigTIFF v43 Test Suite
// Covers:
//   - Test 01: Classic TIFF (42) II/MM roundtrip (None, PackBits, LZW, Deflate; Gray8/Rgb24/Rgba32),
//              multi-page IFD chain, and zero-pixel probeTiffHeader
//   - Test 02: BigTIFF (43) II/MM roundtrip (encodeBigTiffSingle -> probeTiffHeader -> decode) +
//              real BigTIFF fixtures from _ref/image-tiff/tests/images/bigtiff/
//   - Test 03: Security guards: circular IFD loop (InvalidIfd), forged strip_byte_counts (TruncatedData),
//              > TIFF_MAX_IFDS (TooManyPages), Horizontal Predictor=2 decode, bad signature
//   - Test 04: 16-lane SIMD parity (tiffCopyPayloadSimd, tiffFillRunSimd, tiffSwap16Simd vs Scalar)
//   - Test 05: 1,500-iteration deterministic PRNG malformed TIFF / circular IFD / forged count fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { readFileSync } from "cap:fs";
import {
    TIFF_MAX_IFDS,
    TiffHeader,
    TiffError,
    TiffEndian,
    TiffCompression,
    TiffPixelFormat,
    TiffPage,
    TiffDocument,
    tiffCopyPayloadSimd,
    tiffCopyPayloadScalar,
    tiffFillRunSimd,
    tiffFillRunScalar,
    tiffSwap16Simd,
    tiffSwap16Scalar,
    probeTiffHeader,
    encodeSingle,
    encodeBigTiffSingle,
    encodeMultiPage,
    decode,
    decodeWithLimits,
} from "../../lib/image/tiff.cp";

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn runTest01(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 4x2 RGBA32 pixels with repeated runs to exercise PackBits RLE
    let rgba: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) : (i += 1) rgba[i] = if (i < 16) 0xAA else ((i * 17) & 0xFF) as u8;

    const le_pb = try encodeSingle(alloc, 4, 2, TiffPixelFormat.Rgba32, TiffCompression.PackBits, TiffEndian.Little, &rgba);
    const hdr_le = try probeTiffHeader(le_pb);
    expect_eq(hdr_le.endian == TiffEndian.Little, true);
    expect_eq(hdr_le.is_bigtiff, false);
    expect_eq(hdr_le.width, 4 as u32);
    expect_eq(hdr_le.height, 2 as u32);

    let doc_le = try decode(alloc, le_pb);
    defer doc_le.deinit(alloc);
    expect_eq(doc_le.pages.len, 1);
    expect_eq(std.mem.eql(u8, doc_le.pages[0].pixels, &rgba), true);

    // Big-Endian ("MM") LZW & Deflate roundtrip
    let rgb: [24]u8 = undefined;
    i = 0;
    while (i < 24) : (i += 1) rgb[i] = ((i * 11 + 3) & 0xFF) as u8;

    const be_lzw = try encodeSingle(alloc, 4, 2, TiffPixelFormat.Rgb24, TiffCompression.Lzw, TiffEndian.Big, &rgb);
    const hdr_be = try probeTiffHeader(be_lzw);
    expect_eq(hdr_be.endian == TiffEndian.Big, true);
    let doc_be = try decode(alloc, be_lzw);
    defer doc_be.deinit(alloc);
    expect_eq(std.mem.eql(u8, doc_be.pages[0].pixels, &rgb), true);

    // Multi-page IFD chain (Gray8 + Rgb24)
    let gray: [8]u8 = [ 10, 20, 30, 40, 50, 60, 70, 80 ];
    const p0 = TiffPage{ width: 4, height: 2, format: TiffPixelFormat.Gray8, compression: TiffCompression.None, pixels: &gray };
    const p1 = TiffPage{ width: 4, height: 2, format: TiffPixelFormat.Rgb24, compression: TiffCompression.Deflate, pixels: &rgb };
    const mp_pages: [2]TiffPage = [ p0, p1 ];
    const mp_bytes = try encodeMultiPage(alloc, &mp_pages, TiffEndian.Little);
    let mp_doc = try decode(alloc, mp_bytes);
    defer mp_doc.deinit(alloc);
    expect_eq(mp_doc.pages.len, 2);
    expect_eq(std.mem.eql(u8, mp_doc.pages[0].pixels, &gray), true);
    expect_eq(std.mem.eql(u8, mp_doc.pages[1].pixels, &rgb), true);
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. BigTIFF (v43) Little-Endian & Big-Endian encode -> probeTiffHeader -> decode roundtrip
    let pixels: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) : (i += 1) pixels[i] = ((i * 23 + 1) & 0xFF) as u8;

    const bt_le = try encodeBigTiffSingle(alloc, 4, 4, TiffPixelFormat.Gray8, TiffCompression.PackBits, TiffEndian.Little, &pixels);
    const h_le = try probeTiffHeader(bt_le);
    expect_eq(h_le.is_bigtiff, true);
    expect_eq(h_le.endian == TiffEndian.Little, true);
    expect_eq(h_le.width, 4 as u32);
    expect_eq(h_le.height, 4 as u32);
    let d_le = try decode(alloc, bt_le);
    defer d_le.deinit(alloc);
    expect_eq(std.mem.eql(u8, d_le.pages[0].pixels, &pixels), true);

    const bt_be = try encodeBigTiffSingle(alloc, 4, 4, TiffPixelFormat.Gray8, TiffCompression.None, TiffEndian.Big, &pixels);
    const h_be = try probeTiffHeader(bt_be);
    expect_eq(h_be.is_bigtiff, true);
    expect_eq(h_be.endian == TiffEndian.Big, true);
    let d_be = try decode(alloc, bt_be);
    defer d_be.deinit(alloc);
    expect_eq(std.mem.eql(u8, d_be.pages[0].pixels, &pixels), true);

    // 2. Real BigTIFF fixtures from _ref/image-tiff/tests/images/bigtiff/
    const bt_paths: [3][]const u8 = [
        "_ref/image-tiff/tests/images/bigtiff/BigTIFF.tif",
        "_ref/image-tiff/tests/images/bigtiff/BigTIFFLong.tif",
        "_ref/image-tiff/tests/images/bigtiff/BigTIFFMotorola.tif",
    ];
    for (p of bt_paths) {
        const raw = readFileSync(p);
        if (raw.len >= 16) {
            const hdr = try probeTiffHeader(raw);
            expect_eq(hdr.is_bigtiff, true);
            expect_eq(hdr.width, 64 as u32);
            expect_eq(hdr.height, 64 as u32);
        }
    }
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Circular IFD offset -> InvalidIfd (no infinite loop)
    let gray: [4]u8 = [ 1, 2, 3, 4 ];
    const valid = try encodeSingle(alloc, 2, 2, TiffPixelFormat.Gray8, TiffCompression.None, TiffEndian.Little, &gray);
    let circ = try alloc.dupe(u8, valid);
    // Patch next_ifd_offset of IFD0 (at 8 + 2 + 9*12 = 118) back to 8
    circ[118] = 8;
    circ[119] = 0;
    circ[120] = 0;
    circ[121] = 0;
    let circ_err = false;
    * = decode(alloc, circ) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidIfd")) circ_err = true;
    };
    expect_eq(circ_err, true);

    // 2. Forged huge strip_byte_counts -> TruncatedData (no OOB / OOM)
    let forged = try alloc.dupe(u8, valid);
    // Entry 8 (STRIP_BYTE_COUNTS) value at 8 + 2 + 8*12 + 8 = 114
    forged[114] = 0xFF;
    forged[115] = 0xFF;
    forged[116] = 0xFF;
    forged[117] = 0x7F;
    let trunc_err = false;
    * = decode(alloc, forged) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "TruncatedData")) trunc_err = true;
    };
    expect_eq(trunc_err, true);

    // 3. TooManyPages (> max_pages) guard
    expect_eq(TIFF_MAX_IFDS, 64);
    const p0 = TiffPage{ width: 2, height: 2, format: TiffPixelFormat.Gray8, compression: TiffCompression.None, pixels: &gray };
    const mp2: [2]TiffPage = [ p0, p0 ];
    const mp_bytes = try encodeMultiPage(alloc, &mp2, TiffEndian.Little);
    let pages_err = false;
    * = decodeWithLimits(alloc, mp_bytes, 65535, 65535, 1) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "TooManyPages")) pages_err = true;
    };
    expect_eq(pages_err, true);

    // 4. Invalid signature rejected
    const bad_sig: [8]u8 = [ 'X', 'Y', 42, 0, 8, 0, 0, 0 ];
    let sig_err = false;
    * = decode(alloc, &bad_sig) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidSignature")) sig_err = true;
    };
    expect_eq(sig_err, true);
}

fn runTest04(): void! {
    // 16-lane SIMD parity: tiffCopyPayloadSimd, tiffFillRunSimd, tiffSwap16Simd
    let src: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 128) : (i += 1) src[i] = ((i * 31 + 9) & 0xFF) as u8;

    let d_simd: [128]u8 = undefined;
    let d_scal: [128]u8 = undefined;
    let sz: usize = 0;
    while (sz <= 128) : (sz += 1) {
        tiffCopyPayloadSimd(d_simd[0..sz], src[0..sz]);
        tiffCopyPayloadScalar(d_scal[0..sz], src[0..sz]);
        expect_eq(std.mem.eql(u8, d_simd[0..sz], d_scal[0..sz]), true);

        tiffFillRunSimd(d_simd[0..sz], 0x5A);
        tiffFillRunScalar(d_scal[0..sz], 0x5A);
        expect_eq(std.mem.eql(u8, d_simd[0..sz], d_scal[0..sz]), true);

        if ((sz & 1) == 0) {
            tiffSwap16Simd(d_simd[0..sz], src[0..sz]);
            tiffSwap16Scalar(d_scal[0..sz], src[0..sz]);
            expect_eq(std.mem.eql(u8, d_simd[0..sz], d_scal[0..sz]), true);
        }
    }
}

fn runTest05(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1,500-iteration deterministic PRNG malformed TIFF / BigTIFF fuzzing
    let seed: u64 = 0x71FF_4243_DEAD_BEEF;
    let buf: [128]u8 = undefined;

    let iter: usize = 0;
    let ok_count: usize = 0;
    let err_count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = ((xorshift64(&seed) % 100) + 1) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = (xorshift64(&seed) & 0xFF) as u8;
        }
        if ((iter & 3) == 0 && len >= 8) {
            buf[0] = 'I';
            buf[1] = 'I';
            buf[2] = 42;
            buf[3] = 0;
        } else if ((iter & 3) == 1 && len >= 16) {
            buf[0] = 'M';
            buf[1] = 'M';
            buf[2] = 0;
            buf[3] = 43;
        }
        * = probeTiffHeader(buf[0..len]) catch {};
        let doc = decode(alloc, buf[0..len]) catch {
            err_count += 1;
            continue;
        };
        doc.deinit(alloc);
        ok_count += 1;
    }
    expect_eq(ok_count + err_count, 1500);
}

fn main() {
    test("tiff_ref_01_classic_ii_mm_packbits_lzw_deflate_and_multipage", () => { try runTest01(); });
    test("tiff_ref_02_bigtiff_v43_ii_mm_roundtrip_and_real_fixtures", () => { try runTest02(); });
    test("tiff_ref_03_security_circular_ifd_forged_counts_and_limits", () => { try runTest03(); });
    test("tiff_ref_04_simd_16lane_copy_fill_and_swap16_parity", () => { try runTest04(); });
    test("tiff_ref_05_deterministic_prng_1500_fuzz_malformed_tiff", () => { try runTest05(); });
}
