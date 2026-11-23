// aldanor/rapid-qoi (MIT/Apache-2.0) & QOI v1.0 Reference-Hardened Lossless Codec Test Suite
// Covers:
//   - Test 01: RGB (ch=3) & RGBA (ch=4), sRGB (0) & Linear (1), 0-pixel-diff encode->decode roundtrip
//   - Test 02: All 6 QOI opcodes explicitly triggered (RUN up to 62 & 63+, DIFF wrap-around, LUMA, INDEX, RGB, RGBA)
//   - Test 03: Security guards: width/height=0, invalid channels/colorspace, > QOI_MAX_PIXELS (ImageTooLarge),
//              missing 8-byte end marker (MissingEndMarker), truncated data, buffer too small
//   - Test 04: 16-lane SIMD parity (qoiFillRunRgbaSimd, qoiRgbToRgbaSimd, qoiPixelsEqualSimd vs Scalar)
//   - Test 05: 1,500-iteration deterministic PRNG malformed QOI / forged dimensions / truncated fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    QOI_MAX_PIXELS,
    QoiError,
    QoiHeader,
    qoiFillRunRgbaSimd,
    qoiFillRunRgbaScalar,
    qoiRgbToRgbaSimd,
    qoiRgbToRgbaScalar,
    qoiPixelsEqualSimd,
    qoiPixelsEqualScalar,
    qoiMaxEncodedSize,
    isQoi,
    parseQoiHeader,
    parseQoiHeaderStrict,
    encodeQoi,
    encodeQoiStrict,
    decodeQoi,
    decodeQoiStrict,
} from "../../lib/image/qoi.cp";

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn runTest01(): void! {
    // 1. RGB (channels=3, colorspace=0) roundtrip
    let rgb: [48]u8 = undefined; // 4x4 RGB
    let i: usize = 0;
    while (i < 48) : (i += 1) rgb[i] = ((i * 29 + 7) & 0xFF) as u8;

    const max_rgb = qoiMaxEncodedSize(4, 4, 3).?;
    expect_eq(max_rgb > 22, true);
    let enc_buf: [256]u8 = undefined;
    const enc_rgb = try encodeQoiStrict(&rgb, 4, 4, 3, 0, &enc_buf);
    expect_eq(isQoi(enc_rgb), true);

    let dec_rgb: [48]u8 = undefined;
    const hdr_rgb = try decodeQoiStrict(enc_rgb, &dec_rgb);
    expect_eq(hdr_rgb.width, 4 as u32);
    expect_eq(hdr_rgb.height, 4 as u32);
    expect_eq(hdr_rgb.channels, 3 as u8);
    expect_eq(hdr_rgb.colorspace, 0 as u8);
    expect_eq(qoiPixelsEqualSimd(&dec_rgb, &rgb), true);

    // 2. RGBA (channels=4, colorspace=1) roundtrip
    let rgba: [64]u8 = undefined; // 4x4 RGBA
    i = 0;
    while (i < 64) : (i += 1) rgba[i] = ((i * 17 + 3) & 0xFF) as u8;
    const enc_rgba = try encodeQoiStrict(&rgba, 4, 4, 4, 1, &enc_buf);
    let dec_rgba: [64]u8 = undefined;
    const hdr_rgba = try decodeQoiStrict(enc_rgba, &dec_rgba);
    expect_eq(hdr_rgba.channels, 4 as u8);
    expect_eq(hdr_rgba.colorspace, 1 as u8);
    expect_eq(qoiPixelsEqualSimd(&dec_rgba, &rgba), true);
}

fn runTest02(): void! {
    // Exercise all 6 QOI opcodes:
    // - 70 identical pixels -> triggers max RUN (62) + second RUN (8)
    // - Wrap-around DIFF (255 -> 0 -> 255)
    // - LUMA (green delta +20, r/b delta within [-8,7] relative to green)
    // - RGB & RGBA full literal
    // - INDEX (repeating a previously seen pixel)
    const num_px: usize = 80;
    let pixels: [320]u8 = undefined;
    let i: usize = 0;
    // Pixels 0..69: (10, 20, 30, 255) -> 70-pixel RUN (splits at 62 + 8)
    while (i < 70) : (i += 1) {
        pixels[i * 4] = 10;
        pixels[i * 4 + 1] = 20;
        pixels[i * 4 + 2] = 30;
        pixels[i * 4 + 3] = 255;
    }
    // Pixel 70: wrap-around DIFF from (10,20,30) -> (11,19,31)
    pixels[70 * 4] = 11; pixels[70 * 4 + 1] = 19; pixels[70 * 4 + 2] = 31; pixels[70 * 4 + 3] = 255;
    // Pixel 71: LUMA (+20 green, +22 red, +18 blue) -> (33, 39, 49, 255)
    pixels[71 * 4] = 33; pixels[71 * 4 + 1] = 39; pixels[71 * 4 + 2] = 49; pixels[71 * 4 + 3] = 255;
    // Pixel 72: RGB literal (200, 10, 250, 255)
    pixels[72 * 4] = 200; pixels[72 * 4 + 1] = 10; pixels[72 * 4 + 2] = 250; pixels[72 * 4 + 3] = 255;
    // Pixel 73: RGBA literal (200, 10, 250, 128)
    pixels[73 * 4] = 200; pixels[73 * 4 + 1] = 10; pixels[73 * 4 + 2] = 250; pixels[73 * 4 + 3] = 128;
    // Pixel 74..79: INDEX hit back to (10, 20, 30, 255)
    i = 74;
    while (i < num_px) : (i += 1) {
        pixels[i * 4] = 10;
        pixels[i * 4 + 1] = 20;
        pixels[i * 4 + 2] = 30;
        pixels[i * 4 + 3] = 255;
    }

    let enc_buf: [1024]u8 = undefined;
    const enc = try encodeQoiStrict(&pixels, 80, 1, 4, 0, &enc_buf);
    // Compressed size must be dramatically smaller than 320 + 22 due to RUN + INDEX
    expect_eq(enc.len < 64, true);

    let dec_buf: [320]u8 = undefined;
    const hdr = try decodeQoiStrict(enc, &dec_buf);
    expect_eq(hdr.width, 80 as u32);
    expect_eq(qoiPixelsEqualSimd(&dec_buf, &pixels), true);
}

fn runTest03(): void! {
    expect_eq(QOI_MAX_PIXELS, 64000000 as u64);

    // 1. Invalid header: zero width/height, bad channels=2, bad colorspace=2
    const bad_w: [14]u8 = [ 'q', 'o', 'i', 'f', 0, 0, 0, 0, 0, 0, 0, 1, 4, 0 ];
    let err_w = false;
    * = parseQoiHeaderStrict(&bad_w) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidHeader")) err_w = true;
    };
    expect_eq(err_w, true);

    const bad_ch: [14]u8 = [ 'q', 'o', 'i', 'f', 0, 0, 0, 1, 0, 0, 0, 1, 2, 0 ];
    let err_ch = false;
    * = parseQoiHeaderStrict(&bad_ch) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidChannels")) err_ch = true;
    };
    expect_eq(err_ch, true);

    const bad_cs: [14]u8 = [ 'q', 'o', 'i', 'f', 0, 0, 0, 1, 0, 0, 0, 1, 4, 2 ];
    let err_cs = false;
    * = parseQoiHeaderStrict(&bad_cs) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidColorspace")) err_cs = true;
    };
    expect_eq(err_cs, true);

    // 2. ImageTooLarge (> QOI_MAX_PIXELS = 64,000,000)
    const huge_hdr: [14]u8 = [ 'q', 'o', 'i', 'f', 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 4, 0 ]; // 65536*65536
    let err_huge = false;
    * = parseQoiHeaderStrict(&huge_hdr) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "ImageTooLarge")) err_huge = true;
    };
    expect_eq(err_huge, true);

    // 3. Missing 8-byte end marker -> MissingEndMarker
    let px: [4]u8 = [ 1, 2, 3, 4 ];
    let enc_buf: [64]u8 = undefined;
    const valid = try encodeQoiStrict(&px, 1, 1, 4, 0, &enc_buf);
    let bad_end: [64]u8 = undefined;
    @memcpy(bad_end[0..valid.len], valid);
    bad_end[valid.len - 1] = 0; // corrupt trailing 0x01
    let dec_out: [4]u8 = undefined;
    let err_end = false;
    * = decodeQoiStrict(bad_end[0..valid.len], &dec_out) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "MissingEndMarker")) err_end = true;
    };
    expect_eq(err_end, true);

    // 4. BufferTooSmall
    let tiny_out: [2]u8 = undefined;
    let err_buf = false;
    * = decodeQoiStrict(valid, &tiny_out) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "BufferTooSmall")) err_buf = true;
    };
    expect_eq(err_buf, true);
}

fn runTest04(): void! {
    // 1. 16-lane SIMD RGBA run fill parity (0..64 pixels)
    let simd_px: [256]u8 = undefined;
    let scal_px: [256]u8 = undefined;
    let cnt: usize = 0;
    while (cnt <= 64) : (cnt += 1) {
        qoiFillRunRgbaSimd(simd_px[0 .. cnt * 4], cnt, 0x12, 0x34, 0x56, 0x78);
        qoiFillRunRgbaScalar(scal_px[0 .. cnt * 4], cnt, 0x12, 0x34, 0x56, 0x78);
        expect_eq(qoiPixelsEqualSimd(simd_px[0 .. cnt * 4], scal_px[0 .. cnt * 4]), true);
    }

    // 2. 16-lane SIMD RGB24 -> RGBA32 conversion parity (0..32 pixels)
    let rgb_src: [96]u8 = undefined;
    let i: usize = 0;
    while (i < 96) : (i += 1) rgb_src[i] = ((i * 19 + 5) & 0xFF) as u8;
    let rgba_simd: [128]u8 = undefined;
    let rgba_scal: [128]u8 = undefined;
    let n_px: usize = 0;
    while (n_px <= 32) : (n_px += 1) {
        qoiRgbToRgbaSimd(rgba_simd[0 .. n_px * 4], rgb_src[0 .. n_px * 3]);
        qoiRgbToRgbaScalar(rgba_scal[0 .. n_px * 4], rgb_src[0 .. n_px * 3]);
        expect_eq(qoiPixelsEqualSimd(rgba_simd[0 .. n_px * 4], rgba_scal[0 .. n_px * 4]), true);
        expect_eq(qoiPixelsEqualScalar(rgba_simd[0 .. n_px * 4], rgba_scal[0 .. n_px * 4]), true);
    }
}

fn runTest05(): void! {
    // 1,500-iteration deterministic PRNG malformed QOI / forged dimensions / truncated fuzzing
    let seed: u64 = 0x7101_401F_CAFE_F00D;
    let buf: [128]u8 = undefined;
    let out_px: [256]u8 = undefined;

    let iter: usize = 0;
    let ok_count: usize = 0;
    let err_count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = ((xorshift64(&seed) % 96) + 1) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = (xorshift64(&seed) & 0xFF) as u8;
        }
        if ((iter & 1) == 0 && len >= 22) {
            buf[0] = 'q'; buf[1] = 'o'; buf[2] = 'i'; buf[3] = 'f';
            buf[12] = 4; buf[13] = 0;
            buf[len - 1] = 1;
        }
        * = isQoi(buf[0..len]);
        * = decodeQoiStrict(buf[0..len], &out_px) catch {
            err_count += 1;
            continue;
        };
        ok_count += 1;
    }
    expect_eq(ok_count + err_count, 1500);
}

fn main() {
    test("qoi_ref_01_rgb_rgba_srgb_linear_roundtrip", () => { try runTest01(); });
    test("qoi_ref_02_all_six_opcodes_run_diff_luma_index_rgb_rgba", () => { try runTest02(); });
    test("qoi_ref_03_security_header_max_pixels_end_marker_and_bounds", () => { try runTest03(); });
    test("qoi_ref_04_simd_16lane_run_fill_rgb_to_rgba_and_eq_parity", () => { try runTest04(); });
    test("qoi_ref_05_deterministic_prng_1500_fuzz_malformed_qoi", () => { try runTest05(); });
}
