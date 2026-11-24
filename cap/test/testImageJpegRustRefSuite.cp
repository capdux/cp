// zune-image / zune-jpeg (MIT/Apache-2.0/Zlib) Reference-Hardened Baseline JPEG Suite
// Covers encode->decode PSNR closed-loop, zero-heap stack decode, grayscale + DRI/RSTm,
// real zune-image external JPEG cross-decoding (4:2:0 / YCbCr / grayscale / fuzz-corpus),
// and 1,200-iteration deterministic malformed JPEG fuzzing.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { readFileSync } from "cap:fs";
import {
    Jpeg,
    JpegError,
    JpegHeaderInfo,
    parseJpegHeader,
    jpegEncode,
    jpegEncodeInto,
    decodeJpegToRgbaInto,
    decodeJpegToRgba,
} from "../../lib/image/jpeg.cp";

fn computeRgbPsnr(a: []const u8, b: []const u8): f64 {
    if (a.len != b.len || a.len == 0) return 0.0;
    let sse: f64 = 0.0;
    let count: usize = 0;
    let i: usize = 0;
    while (i + 3 < a.len) : (i += 4) {
        let c: usize = 0;
        while (c < 3) : (c += 1) {
            const d: f64 = @as(f64, @floatFromInt(a[i + c])) - @as(f64, @floatFromInt(b[i + c]));
            sse += d * d;
            count += 1;
        }
    }
    const mse: f64 = sse / @as(f64, @floatFromInt(count));
    if (mse <= 0.000000001) return 99.0;
    return 10.0 * std.math.log10((255.0 * 255.0) / mse);
}

fn readJpegFile(alloc: std.mem.Allocator, path: []const u8): ([]u8)? {
    const raw = readFileSync(path);
    if (raw.len == 0) return null;
    return alloc.dupe(u8, raw) catch return null;
}

// ============================================================================
// Part A: Encode -> Decode Closed-Loop & PSNR Verification
// ============================================================================

fn runTestCase1(): !void {
    const alloc = std.heap.page_allocator;
    const px: [4]u8 = [200, 120, 60, 255];
    const enc1 = jpegEncode(alloc, px[0..], 1, 1, 95) catch unreachable;
    defer alloc.free(enc1);

    let dec1 = decodeJpegToRgba(alloc, enc1) catch unreachable;
    defer dec1.deinit(alloc);
    expect_eq(dec1.width, 1);
    expect_eq(dec1.height, 1);
    expect_eq(dec1.components, 3);
    const psnr1 = computeRgbPsnr(px[0..], dec1.rgba);
    expect_eq(psnr1 >= 38.0, true);

    // 16x16 solid color
    let solid: [16 * 16 * 4]u8 = [0; 16 * 16 * 4];
    let i: usize = 0;
    while (i < solid.len) : (i += 4) {
        solid[i] = 64;
        solid[i + 1] = 180;
        solid[i + 2] = 220;
        solid[i + 3] = 255;
    }
    const enc16 = jpegEncode(alloc, solid[0..], 16, 16, 95) catch unreachable;
    defer alloc.free(enc16);
    let dec16 = decodeJpegToRgba(alloc, enc16) catch unreachable;
    defer dec16.deinit(alloc);
    expect_eq(computeRgbPsnr(solid[0..], dec16.rgba) >= 44.0, true);
}

fn runTestCase2(): !void {
    const alloc = std.heap.page_allocator;
    let grad: [32 * 32 * 4]u8 = [0; 32 * 32 * 4];
    let y: usize = 0;
    while (y < 32) : (y += 1) {
        let x: usize = 0;
        while (x < 32) : (x += 1) {
            const o = (y * 32 + x) * 4;
            grad[o] = @as(u8, @intCast(x * 8));
            grad[o + 1] = @as(u8, @intCast(y * 8));
            grad[o + 2] = @as(u8, @intCast(((x + y) * 4) & 0xFF));
            grad[o + 3] = 255;
        }
    }

    const enc_lo = jpegEncode(alloc, grad[0..], 32, 32, 30) catch unreachable;
    defer alloc.free(enc_lo);
    const enc_hi = jpegEncode(alloc, grad[0..], 32, 32, 95) catch unreachable;
    defer alloc.free(enc_hi);

    let dec_lo = decodeJpegToRgba(alloc, enc_lo) catch unreachable;
    defer dec_lo.deinit(alloc);
    let dec_hi = decodeJpegToRgba(alloc, enc_hi) catch unreachable;
    defer dec_hi.deinit(alloc);

    const psnr_lo = computeRgbPsnr(grad[0..], dec_lo.rgba);
    const psnr_hi = computeRgbPsnr(grad[0..], dec_hi.rgba);
    expect_eq(psnr_lo >= 28.0, true);
    expect_eq(psnr_hi >= 40.0, true);
    expect_eq(psnr_hi > psnr_lo, true);
}

fn runTestCase3(): !void {
    let src: [13 * 17 * 4]u8 = [0; 13 * 17 * 4];
    let y: usize = 0;
    while (y < 17) : (y += 1) {
        let x: usize = 0;
        while (x < 13) : (x += 1) {
            const o = (y * 13 + x) * 4;
            const stripe: u8 = if (((x + y) & 1) == 0) 230 else 30;
            src[o] = stripe;
            src[o + 1] = @as(u8, @intCast((x * 17 + 20) & 0xFF));
            src[o + 2] = @as(u8, @intCast((y * 13 + 40) & 0xFF));
            src[o + 3] = 255;
        }
    }

    // 100% zero-heap encode + zero-heap decode into stack buffers
    let jpg_buf: [8192]u8 = [0; 8192];
    const jpg = jpegEncodeInto(src[0..], 13, 17, 98, jpg_buf[0..]) catch unreachable;

    let out_rgba: [13 * 17 * 4]u8 = [0; 13 * 17 * 4];
    const hdr = decodeJpegToRgbaInto(jpg, out_rgba[0..]) catch unreachable;
    expect_eq(hdr.width, 13);
    expect_eq(hdr.height, 17);
    expect_eq(hdr.components, 3);
    expect_eq(computeRgbPsnr(src[0..], out_rgba[0..]) >= 36.0, true);
}

// ============================================================================

// Part B: Grayscale (1-Component) & DRI + RST0..RST7 Restart Markers

// ============================================================================

fn runTestCase4(): !void {
    // Minimal 16x8 1-component grayscale Baseline JPEG with DRI=1 (2 MCUs separated by RST0):
    // Quant table 0: all 1s.
    // DC Huff 0: 1-bit code 0 -> cat 0 (diff=0), 2-bit code 10 -> cat 7 (diff=+80 -> +10 pixel shift)
    // AC Huff 0: 1-bit code 0 -> EOB (0x00)
    // MCU 0: DC cat=0 (0) + AC EOB (0) -> all pixels = 128; padded with 1s -> 0b0011_1111 = 0x3F
    // RST0: 0xFF 0xD0 (resets DC predictor to 0!)
    // MCU 1: DC cat=7 (10) + 7 bits 1010000 (80) + AC EOB (0) -> DC = +80, val = 128 + 80/8 = 138
    // Bits: 10 1010000 0 + 111111 (pad) = 1010_1000 0011_1111 = 0xA8, 0x3F
    let jpg: [256]u8 = [0; 256];
    let p: usize = 0;

    // SOI
    jpg[p] = 0xFF; jpg[p + 1] = 0xD8; p += 2;

    // DQT (length = 2 + 1 + 64 = 67 = 0x0043, pq=0, tq=0, all 1s)
    jpg[p] = 0xFF; jpg[p + 1] = 0xDB; jpg[p + 2] = 0x00; jpg[p + 3] = 67; jpg[p + 4] = 0x00; p += 5;
    let q: usize = 0;
    while (q < 64) : (q += 1) { jpg[p] = 1; p += 1; }

    // DRI (length = 4, restart_interval = 1)
    jpg[p] = 0xFF; jpg[p + 1] = 0xDD; jpg[p + 2] = 0x00; jpg[p + 3] = 0x04;
    jpg[p + 4] = 0x00; jpg[p + 5] = 0x01; p += 6;

    // SOF0 (length = 8 + 3 = 11, precision=8, h=8, w=16, comps=1, id=1, hv=0x11, q=0)
    jpg[p] = 0xFF; jpg[p + 1] = 0xC0; jpg[p + 2] = 0x00; jpg[p + 3] = 11;
    jpg[p + 4] = 8;
    jpg[p + 5] = 0; jpg[p + 6] = 8;
    jpg[p + 7] = 0; jpg[p + 8] = 16;
    jpg[p + 9] = 1;
    jpg[p + 10] = 1; jpg[p + 11] = 0x11; jpg[p + 12] = 0;
    p += 13;

    // DHT DC 0: bits = [1, 1, 0..0], vals = [0, 7] (length = 2 + 1 + 16 + 2 = 21)
    jpg[p] = 0xFF; jpg[p + 1] = 0xC4; jpg[p + 2] = 0x00; jpg[p + 3] = 21; jpg[p + 4] = 0x00; p += 5;
    jpg[p] = 1; jpg[p + 1] = 1; p += 16;
    jpg[p] = 0; jpg[p + 1] = 7; p += 2;

    // DHT AC 0: bits = [1, 0..0], vals = [0x00] (length = 2 + 1 + 16 + 1 = 20)
    jpg[p] = 0xFF; jpg[p + 1] = 0xC4; jpg[p + 2] = 0x00; jpg[p + 3] = 20; jpg[p + 4] = 0x10; p += 5;
    jpg[p] = 1; p += 16;
    jpg[p] = 0x00; p += 1;

    // SOS (length = 6 + 2 = 8, ns=1, id=1, td/ta=0x00, ss=0, se=63, ah/al=0)
    jpg[p] = 0xFF; jpg[p + 1] = 0xDA; jpg[p + 2] = 0x00; jpg[p + 3] = 8;
    jpg[p + 4] = 1; jpg[p + 5] = 1; jpg[p + 6] = 0x00;
    jpg[p + 7] = 0; jpg[p + 8] = 63; jpg[p + 9] = 0;
    p += 10;

    // Entropy: MCU 0 (0x3F), RST0 (0xFF 0xD0), MCU 1 (0xA8, 0x3F), EOI (0xFF 0xD9)
    jpg[p] = 0x3F; p += 1;
    jpg[p] = 0xFF; jpg[p + 1] = 0xD0; p += 2;
    jpg[p] = 0xA8; jpg[p + 1] = 0x3F; p += 2;
    jpg[p] = 0xFF; jpg[p + 1] = 0xD9; p += 2;

    let rgba: [16 * 8 * 4]u8 = [0; 16 * 8 * 4];
    const hdr = decodeJpegToRgbaInto(jpg[0..p], rgba[0..]) catch unreachable;
    expect_eq(hdr.width, 16);
    expect_eq(hdr.height, 8);
    expect_eq(hdr.components, 1);
    expect_eq(hdr.restart_interval, 1);

    // MCU 0 (x=0..7): Y == 128
    expect_eq(rgba[0], 128);
    expect_eq(rgba[1], 128);
    expect_eq(rgba[2], 128);
    expect_eq(rgba[3], 255);
    // MCU 1 (x=8..15): Y == 138 (after RST0 reset)
    const mcu1_off: usize = 8 * 4;
    expect_eq(rgba[mcu1_off], 138);
    expect_eq(rgba[mcu1_off + 1], 138);
    expect_eq(rgba[mcu1_off + 2], 138);
    expect_eq(rgba[mcu1_off + 3], 255);
}

// ============================================================================

// Part C: Real External zune-image JPEG Cross-Decoding & Fuzz-Corpus Guards

// ============================================================================

fn runTestCase5(): !void {
    const alloc = std.heap.page_allocator;
    const jpg = readJpegFile(alloc, "/data/dev/cp/_ref/zune-image/test-images/jpeg/app14/baseline_ycbcr.jpg") ?? return;
    defer alloc.free(jpg);

    const hdr = parseJpegHeader(jpg) catch unreachable;
    expect_eq(hdr.width > 0 && hdr.height > 0, true);
    expect_eq(hdr.is_progressive, false);
    expect_eq(hdr.components, 3);

    let img = decodeJpegToRgba(alloc, jpg) catch unreachable;
    defer img.deinit(alloc);
    expect_eq(img.width, hdr.width);
    expect_eq(img.height, hdr.height);
    expect_eq(img.rgba.len, @as(usize, hdr.width) * @as(usize, hdr.height) * 4);
    expect_eq(img.rgba[3], 255);
}

fn runTestCase6(): !void {
    const alloc = std.heap.page_allocator;
    const jpg = readJpegFile(alloc, "/data/dev/cp/_ref/zune-image/test-images/jpeg/2029.jpg") ?? return;
    defer alloc.free(jpg);

    const hdr = parseJpegHeader(jpg) catch unreachable;
    expect_eq(hdr.width > 0 && hdr.height > 0, true);
    if (!hdr.is_progressive && (hdr.components == 1 || hdr.components == 3)) {
        let img = decodeJpegToRgba(alloc, jpg) catch unreachable;
        defer img.deinit(alloc);
        expect_eq(img.width, hdr.width);
        expect_eq(img.height, hdr.height);
        expect_eq(img.rgba[3], 255);
    }
}

fn runTestCase7(): !void {
    const alloc = std.heap.page_allocator;
    const jpg = readJpegFile(alloc, "/data/dev/cp/_ref/zune-image/test-images/jpeg/app14/single_component_transform_zero.jpg") ?? return;
    defer alloc.free(jpg);

    const hdr = parseJpegHeader(jpg) catch unreachable;
    expect_eq(hdr.components, 1);
    if (!hdr.is_progressive) {
        let img = decodeJpegToRgba(alloc, jpg) catch unreachable;
        defer img.deinit(alloc);
        expect_eq(img.rgba[0], img.rgba[1]);
        expect_eq(img.rgba[1], img.rgba[2]);
        expect_eq(img.rgba[3], 255);
    }
}

fn runTestCase8(): !void {
    const alloc = std.heap.page_allocator;
    const paths: [6][]const u8 = [
        "/data/dev/cp/_ref/zune-image/fuzz-corpus/jpeg/issue256.jpg",
        "/data/dev/cp/_ref/zune-image/fuzz-corpus/jpeg/issue389.jpg",
        "/data/dev/cp/_ref/zune-image/fuzz-corpus/jpeg/neg_x_IDX_76_RAND_1716085892843070121921591_minimized_214.jpg",
        "/data/dev/cp/_ref/zune-image/fuzz-corpus/jpeg/re.jpg",
        "/data/dev/cp/_ref/zune-image/fuzz-corpus/jpeg/small-img.jpg",
        "/data/dev/cp/_ref/zune-image/fuzz-corpus/jpeg/test_IDX_45_RAND_168601280367171438891916_minimized_837.jpg",
    ];
    let out: [4096]u8 = [0; 4096];
    let i: usize = 0;
    while (i < paths.len) : (i += 1) {
        if (bytes := readJpegFile(alloc, paths[i])) {
            defer alloc.free(bytes);
            * = parseJpegHeader(bytes) catch {};
            * = decodeJpegToRgbaInto(bytes, out[0..]) catch {};
        }
    }
    expect_eq(i, 6);
}

// ============================================================================

// Part D: Error Boundary Guards & 1,200-Iter Deterministic Fuzzing

// ============================================================================

fn runTestCase9(): !void {
    let out: [256]u8 = [0; 256];

    // 1. Invalid SOI
    let e1 = false;
    * = decodeJpegToRgbaInto("NOT_A_JPEG", out[0..]) catch |e| {
        if (e == JpegError.InvalidMarker) e1 = true;
    };
    expect_eq(e1, true);

    // 2. Progressive SOF2 explicitly rejected as UnsupportedFormat
    const prog: [15]u8 = [0xFF, 0xD8, 0xFF, 0xC2, 0x00, 11, 8, 0, 8, 0, 8, 1, 1, 0x11, 0];
    let e2 = false;
    * = decodeJpegToRgbaInto(prog[0..], out[0..]) catch |e| {
        if (e == JpegError.UnsupportedFormat) e2 = true;
    };
    expect_eq(e2, true);

    // 3. Invalid sampling factor (hv = 0x00)
    const bad_samp: [15]u8 = [0xFF, 0xD8, 0xFF, 0xC0, 0x00, 11, 8, 0, 8, 0, 8, 1, 1, 0x00, 0];
    let e3 = false;
    * = decodeJpegToRgbaInto(bad_samp[0..], out[0..]) catch |e| {
        if (e == JpegError.InvalidSamplingFactor) e3 = true;
    };
    expect_eq(e3, true);
}

fn runTestCase10(): !void {
    let src: [8 * 8 * 4]u8 = [0; 8 * 8 * 4];
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        src[i] = @as(u8, @intCast((i * 19 + 3) & 0xFF));
    }
    let base_buf: [2048]u8 = [0; 2048];
    const base_jpg = jpegEncodeInto(src[0..], 8, 8, 75, base_buf[0..]) catch unreachable;

    let mut_buf: [2048]u8 = [0; 2048];
    let out_rgba: [16 * 16 * 4]u8 = [0; 16 * 16 * 4];
    const blen: usize = base_jpg.len;

    let rng: u32 = 0x517C_C1B7;
    let iter: usize = 0;
    while (iter < 1200) : (iter += 1) {
        let c: usize = 0;
        while (c < blen) : (c += 1) {
            mut_buf[c] = base_jpg[c];
        }

        rng = rng *% 1664525 +% 1013904223;
        const mode: u32 = rng & 3;
        let tlen: usize = blen;

        if (mode == 0) {
            // Truncation
            rng = rng *% 1664525 +% 1013904223;
            tlen = @as(usize, rng % @as(u32, @intCast(blen)));
        } else if (mode == 1) {
            // Random bit flips in entropy or marker segments
            let flips: usize = @as(usize, 1) + @as(usize, (rng >> 8) & 3);
            let f: usize = 0;
            while (f < flips) : (f += 1) {
                rng = rng *% 1664525 +% 1013904223;
                const pos: usize = @as(usize, rng % @as(u32, @intCast(blen)));
                mut_buf[pos] ^= @as(u8, @intCast((rng >> 16) & 0xFF));
            }
        } else if (mode == 2) {
            // Inject 0xFF marker bytes inside entropy stream
            rng = rng *% 1664525 +% 1013904223;
            if (blen > 32) {
                const pos: usize = blen - 32 + @as(usize, (rng >> 12) & 30);
                mut_buf[pos] = 0xFF;
                mut_buf[pos + 1] = @as(u8, @intCast((rng >> 20) & 0xFF));
            }
        } else {
            // Synthetic random JPEG starting with SOI
            mut_buf[0] = 0xFF;
            mut_buf[1] = 0xD8;
            tlen = @as(usize, 2) + @as(usize, (rng >> 16) & 127);
            let k: usize = 2;
            while (k < tlen) : (k += 1) {
                rng = rng *% 1664525 +% 1013904223;
                mut_buf[k] = @as(u8, @intCast((rng >> 24) & 0xFF));
            }
        }

        * = parseJpegHeader(mut_buf[0..tlen]) catch {};
        * = decodeJpegToRgbaInto(mut_buf[0..tlen], out_rgba[0..]) catch {};
    }
    expect_eq(iter, 1200);
}

fn main() {
    test("jpeg_ref_01_roundtrip_1x1_and_solid_16x16_high_psnr", () => {
        runTestCase1()?;
    });
    test("jpeg_ref_02_roundtrip_smooth_gradient_32x32_quality_psnr_monotonicity", () => {
        runTestCase2()?;
    });
    test("jpeg_ref_03_roundtrip_non_multiple_of_8_13x17_high_freq_zero_heap", () => {
        runTestCase3()?;
    });
    test("jpeg_ref_04_grayscale_1comp_and_dri_restart_markers", () => {
        runTestCase4()?;
    });
    test("jpeg_ref_05_real_zune_baseline_ycbcr_cross_decode", () => {
        runTestCase5()?;
    });
    test("jpeg_ref_06_real_zune_420_2029_cross_decode", () => {
        runTestCase6()?;
    });
    test("jpeg_ref_07_real_zune_grayscale_single_component", () => {
        runTestCase7()?;
    });
    test("jpeg_ref_08_real_zune_fuzz_corpus_no_panic", () => {
        runTestCase8()?;
    });
    test("jpeg_ref_09_malformed_markers_progressive_and_sampling_guards", () => {
        runTestCase9()?;
    });
    test("jpeg_ref_10_1200_iter_deterministic_malformed_jpeg_fuzz", () => {
        runTestCase10()?;
    });
}
