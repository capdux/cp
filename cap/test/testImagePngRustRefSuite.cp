// image-rs/image-png (MIT/Apache-2.0) Reference-Hardened PNG Codec Test Suite
// Covers:
//   - Encode -> Decode lossless closed-loop (Gray/RGB/Indexed+PLTE+tRNS/RGBA, all 5 filters, multi-sizes)
//   - Multi-IDAT chunk concatenation & SIMD unfilterRow 0-error verification
//   - Real PngSuite cross-decoding from _ref/image-png/tests/pngsuite/ (all 5 color types x 1/2/4/8/16-bit,
//     Non-interlaced vs Adam7 pixel-exact equivalence, odd sizes 1x1..9x9, all 5 filters, tRNS alpha,
//     and all 14 official corrupt x*.png files strictly rejected)
//   - 1,200-iteration deterministic PRNG malformed PNG fuzzing (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { readFileSync } from "cap:fs";
import {
    Png,
    PngImage,
    PngHeader,
    PngColorType,
    PngFilterType,
    PngError,
    RgbaColor,
} from "../../lib/image/png.cp";
import { pngCrc32 } from "../../lib/image/_pngZlib.cp";

fn slicesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn countTransparentPixels(rgba: []const u8): usize {
    let cnt: usize = 0;
    let i: usize = 3;
    while (i < rgba.len) : (i += 4) {
        if (rgba[i] < 255) cnt += 1;
    }
    return cnt;
}

fn writeU32Be(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

fn main() {
    // ========================================================================
    // Part A: Encode -> Decode Closed-Loop (All 5 Filters, Color Types, Sizes)
    // ========================================================================
    test("png_ref_01_roundtrip_rgba_all_5_filters_and_simd_sizes", () => {
        const widths: [5]u32 = [1, 3, 17, 33, 64];
        const heights: [5]u32 = [1, 5, 13, 19, 24];
        const filters: [5]PngFilterType = [
            PngFilterType.None,
            PngFilterType.Sub,
            PngFilterType.Up,
            PngFilterType.Average,
            PngFilterType.Paeth,
        ];

        let si: usize = 0;
        while (si < 5) : (si += 1) {
            const w = widths[si];
            const h = heights[si];
            const len = (w as usize) * (h as usize) * 4;
            let rgba: []u8 = std.heap.page_allocator.alloc(u8, len) catch unreachable;
            defer std.heap.page_allocator.free(rgba);

            let y: u32 = 0;
            while (y < h) : (y += 1) {
                let x: u32 = 0;
                while (x < w) : (x += 1) {
                    const o = ((y * w + x) * 4) as usize;
                    rgba[o] = ((x * 37 + y * 13) & 0xFF) as u8;
                    rgba[o + 1] = ((x * 11 + y * 53 + 7) & 0xFF) as u8;
                    rgba[o + 2] = (((x ^ y) * 29 + 19) & 0xFF) as u8;
                    rgba[o + 3] = if (((x + y) & 3) == 0) 128 else 255;
                }
            }

            let fi: usize = 0;
            while (fi < 5) : (fi += 1) {
                const encoded = Png.encodeRgbaWithFilter(rgba, w, h, filters[fi]);
                expect_eq(encoded.len > 33, true);
                expect_eq(Png.hasSignature(encoded), true);

                const hdr = Png.parseHeader(encoded) catch unreachable;
                expect_eq(hdr.width, w);
                expect_eq(hdr.height, h);
                expect_eq(hdr.bit_depth, 8);
                expect_eq(hdr.color_type == PngColorType.Rgba, true);
                expect_eq(hdr.interlaced, false);

                const decoded = Png.decodeStrict(encoded) catch unreachable;
                expect_eq(decoded.width, w);
                expect_eq(decoded.height, h);
                expect_eq(slicesEqual(rgba, decoded.rgba), true);
            }
        }
    });

    test("png_ref_02_roundtrip_gray_rgb_indexed_plte_trns", () => {
        // 1. Grayscale 8-bit with Paeth filter
        const gw: u32 = 19;
        const gh: u32 = 11;
        let gray: [19 * 11]u8 = [0; 19 * 11];
        let gi: usize = 0;
        while (gi < gray.len) : (gi += 1) {
            gray[gi] = ((gi * 17 + 5) & 0xFF) as u8;
        }
        const png_gray = Png.encodeGray(gray[0..], gw, gh, PngFilterType.Paeth);
        const dec_gray = Png.decodeStrict(png_gray) catch unreachable;
        expect_eq(dec_gray.width, gw);
        expect_eq(dec_gray.height, gh);
        expect_eq(dec_gray.color_type == PngColorType.Grayscale, true);
        let check_g: usize = 0;
        while (check_g < gray.len) : (check_g += 1) {
            expect_eq(dec_gray.rgba[check_g * 4], gray[check_g]);
            expect_eq(dec_gray.rgba[check_g * 4 + 1], gray[check_g]);
            expect_eq(dec_gray.rgba[check_g * 4 + 2], gray[check_g]);
            expect_eq(dec_gray.rgba[check_g * 4 + 3], 255);
        }

        // 2. RGB 8-bit with Up filter (16-lane SIMD path)
        const rw: u32 = 23;
        const rh: u32 = 9;
        let rgb: [23 * 9 * 3]u8 = [0; 23 * 9 * 3];
        let ri: usize = 0;
        while (ri < rgb.len) : (ri += 1) {
            rgb[ri] = ((ri * 31 + 3) & 0xFF) as u8;
        }
        const png_rgb = Png.encodeRgb(rgb[0..], rw, rh, PngFilterType.Up);
        const dec_rgb = Png.decodeStrict(png_rgb) catch unreachable;
        expect_eq(dec_rgb.width, rw);
        expect_eq(dec_rgb.height, rh);
        expect_eq(dec_rgb.color_type == PngColorType.Rgb, true);
        let px: usize = 0;
        while (px < (rw * rh) as usize) : (px += 1) {
            expect_eq(dec_rgb.rgba[px * 4], rgb[px * 3]);
            expect_eq(dec_rgb.rgba[px * 4 + 1], rgb[px * 3 + 1]);
            expect_eq(dec_rgb.rgba[px * 4 + 2], rgb[px * 3 + 2]);
            expect_eq(dec_rgb.rgba[px * 4 + 3], 255);
        }

        // 3. Indexed (PLTE + tRNS)
        const palette: [12]u8 = [
            255, 0, 0,
            0, 255, 0,
            0, 0, 255,
            255, 255, 0,
        ];
        const trns: [3]u8 = [0, 128, 200];
        const indices: [4]u8 = [0, 1, 2, 3];
        const png_idx = Png.encodeIndexed(indices[0..], 2, 2, palette[0..], trns[0..]);
        const hdr_idx = Png.parseHeader(png_idx) catch unreachable;
        expect_eq(hdr_idx.has_plte, true);
        expect_eq(hdr_idx.has_trns, true);
        expect_eq(hdr_idx.color_type == PngColorType.Indexed, true);

        let dec_idx = Png.decodeStrict(png_idx) catch unreachable;
        expect_eq(dec_idx.getPixel(0, 0).eql(RgbaColor{ r: 255, g: 0, b: 0, a: 0 }), true);
        expect_eq(dec_idx.getPixel(1, 0).eql(RgbaColor{ r: 0, g: 255, b: 0, a: 128 }), true);
        expect_eq(dec_idx.getPixel(0, 1).eql(RgbaColor{ r: 0, g: 0, b: 255, a: 200 }), true);
        expect_eq(dec_idx.getPixel(1, 1).eql(RgbaColor{ r: 255, g: 255, b: 0, a: 255 }), true);
    });

    test("png_ref_03_multi_idat_concatenation_and_simd_unfilter_exact", () => {
        // 1. Verify SIMD unfilterRow(Up) across 49 bytes (3x16 + 1 tail)
        let raw_row: [49]u8 = [0; 49];
        let prev_row: [49]u8 = [0; 49];
        let out_row: [49]u8 = [0; 49];
        let i: usize = 0;
        while (i < 49) : (i += 1) {
            raw_row[i] = ((i * 7 + 200) & 0xFF) as u8;
            prev_row[i] = ((i * 13 + 90) & 0xFF) as u8;
        }
        expect_eq(Png.unfilterRow(2, 4, raw_row[0..], prev_row[0..], out_row[0..]), true);
        i = 0;
        while (i < 49) : (i += 1) {
            expect_eq(out_row[i], raw_row[i] +% prev_row[i]);
        }
        expect_eq(Png.unfilterRow(5, 4, raw_row[0..], prev_row[0..], out_row[0..]), false);

        // 2. Multi-IDAT split test: split single IDAT from valid PNG into 3 IDAT chunks
        let rgba: [8 * 8 * 4]u8 = [0; 8 * 8 * 4];
        i = 0;
        while (i < rgba.len) : (i += 1) {
            rgba[i] = ((i * 23 + 11) & 0xFF) as u8;
        }
        const single = Png.encodeRgbaWithFilter(rgba[0..], 8, 8, PngFilterType.Sub);
        const idat_off: usize = 33;
        const comp_len = ((single[idat_off] as usize) << 24) |
            ((single[idat_off + 1] as usize) << 16) |
            ((single[idat_off + 2] as usize) << 8) |
            (single[idat_off + 3] as usize);
        expect_eq(comp_len >= 6, true);
        const idat_data = single[idat_off + 8 .. idat_off + 8 + comp_len];
        const p1 = comp_len / 3;
        const p2 = (comp_len * 2) / 3;

        const multi_len: usize = (33 as usize) + (12 + p1) + (12 + (p2 - p1)) + (12 + (comp_len - p2)) + 12;
        let multi: []u8 = std.heap.page_allocator.alloc(u8, multi_len) catch unreachable;
        defer std.heap.page_allocator.free(multi);

        let k: usize = 0;
        while (k < 33) : (k += 1) multi[k] = single[k];
        let pos: usize = 33;

        const starts: [3]usize = [0, p1, p2];
        const ends: [3]usize = [p1, p2, comp_len];
        let ci: usize = 0;
        while (ci < 3) : (ci += 1) {
            const seg = idat_data[starts[ci]..ends[ci]];
            writeU32Be(multi, pos, seg.len as u32);
            multi[pos + 4] = 0x49;
            multi[pos + 5] = 0x44;
            multi[pos + 6] = 0x41;
            multi[pos + 7] = 0x54;
            let si: usize = 0;
            while (si < seg.len) : (si += 1) multi[pos + 8 + si] = seg[si];
            const crc = pngCrc32(multi[pos + 4 .. pos + 8 + seg.len]);
            writeU32Be(multi, pos + 8 + seg.len, crc);
            pos += 12 + seg.len;
        }
        const iend_src = single[single.len - 12 .. single.len];
        k = 0;
        while (k < 12) : (k += 1) multi[pos + k] = iend_src[k];

        const dec_multi = Png.decodeStrict(multi) catch unreachable;
        expect_eq(dec_multi.width, 8);
        expect_eq(dec_multi.height, 8);
        expect_eq(slicesEqual(rgba[0..], dec_multi.rgba), true);
    });

    // ========================================================================
    // Part B: Real image-rs/image-png PngSuite Cross-Decoding Verification
    // ========================================================================
    test("png_ref_04_pngsuite_all_color_types_bit_depths_and_adam7_pixel_exact", () => {
        const non_interlaced: [15]string = [
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn0g01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn0g02.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn0g04.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn0g16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn2c16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn3p01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn3p02.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn3p04.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn3p08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn4a08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn4a16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn6a08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basn6a16.png",
        ];
        const adam7: [15]string = [
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi0g01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi0g02.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi0g04.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi0g16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi2c16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi3p01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi3p02.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi3p04.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi3p08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi4a08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi4a16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi6a08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/basi6a16.png",
        ];

        let idx: usize = 0;
        while (idx < 15) : (idx += 1) {
            const b_non = readFileSync(non_interlaced[idx]);
            const b_a7 = readFileSync(adam7[idx]);
            expect_eq(b_non.len > 0, true);
            expect_eq(b_a7.len > 0, true);

            const h_non = Png.parseHeader(b_non) catch unreachable;
            const h_a7 = Png.parseHeader(b_a7) catch unreachable;
            expect_eq(h_non.width, 32);
            expect_eq(h_non.height, 32);
            expect_eq(h_non.interlaced, false);
            expect_eq(h_a7.interlaced, true);
            expect_eq(h_non.bit_depth, h_a7.bit_depth);

            const d_non = Png.decodeStrict(b_non) catch unreachable;
            const d_a7 = Png.decodeStrict(b_a7) catch unreachable;
            expect_eq(d_non.width, 32);
            expect_eq(d_non.height, 32);
            expect_eq(slicesEqual(d_non.rgba, d_a7.rgba), true);
        }
    });

    test("png_ref_05_pngsuite_odd_sizes_filters_and_trns", () => {
        const odd_non: [4]string = [
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s01n3p01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s03n3p01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s07n3p02.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s09n3p02.png",
        ];
        const odd_a7: [4]string = [
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s01i3p01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s03i3p01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s07i3p02.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/s09i3p02.png",
        ];
        const odd_dims: [4]u32 = [1, 3, 7, 9];

        let i: usize = 0;
        while (i < 4) : (i += 1) {
            const bn = readFileSync(odd_non[i]);
            const bi = readFileSync(odd_a7[i]);
            expect_eq(bn.len > 0, true);
            expect_eq(bi.len > 0, true);
            const dn = Png.decodeStrict(bn) catch unreachable;
            const di = Png.decodeStrict(bi) catch unreachable;
            expect_eq(dn.width, odd_dims[i]);
            expect_eq(dn.height, odd_dims[i]);
            expect_eq(slicesEqual(dn.rgba, di.rgba), true);
        }

        const filter_files: [10]string = [
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f00n0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f01n0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f02n0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f03n0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f04n0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f00n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f01n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f02n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f03n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/f04n2c08.png",
        ];
        i = 0;
        while (i < 10) : (i += 1) {
            const bf = readFileSync(filter_files[i]);
            expect_eq(bf.len > 0, true);
            const df = Png.decodeStrict(bf) catch unreachable;
            expect_eq(df.width, 32);
            expect_eq(df.height, 32);
        }

        const trns_files: [6]string = [
            "/data/dev/cp/_ref/image-png/tests/pngsuite/tbbn0g04.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/tbwn0g16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/tbrn2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/tbbn2c16.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/tm3n3p02.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/tbbn3p08.png",
        ];
        i = 0;
        while (i < 6) : (i += 1) {
            const bt = readFileSync(trns_files[i]);
            expect_eq(bt.len > 0, true);
            const ht = Png.parseHeader(bt) catch unreachable;
            expect_eq(ht.has_trns, true);
            const dt = Png.decodeStrict(bt) catch unreachable;
            expect_eq(dt.width, 32);
            expect_eq(dt.height, 32);
            expect_eq(countTransparentPixels(dt.rgba) > 0, true);
        }
    });

    test("png_ref_06_pngsuite_all_14_corrupt_files_strictly_rejected", () => {
        const corrupt_files: [14]string = [
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xc1n0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xc9n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xcrn0g04.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xcsn0g01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xd0n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xd3n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xd9n2c08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xdtn0g01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xhdn0g08.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xlfn0g04.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xs1n0g01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xs2n0g01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xs4n0g01.png",
            "/data/dev/cp/_ref/image-png/tests/pngsuite/xs7n0g01.png",
        ];

        let i: usize = 0;
        while (i < 14) : (i += 1) {
            const bc = readFileSync(corrupt_files[i]);
            expect_eq(bc.len > 0, true);
            const res = Png.decode(bc);
            expect_eq(res == null, true);
        }
    });

    // ========================================================================
    // Part C: 1,200-Iteration Deterministic PRNG Malformed PNG Fuzzing
    // ========================================================================
    test("png_ref_07_deterministic_1200_malformed_fuzz_zero_panic", () => {
        let seed_rgba: [4 * 4 * 4]u8 = [0; 4 * 4 * 4];
        let si: usize = 0;
        while (si < seed_rgba.len) : (si += 1) {
            seed_rgba[si] = ((si * 19 + 7) & 0xFF) as u8;
        }
        const seed_png = Png.encodeRgbaWithFilter(seed_rgba[0..], 4, 4, PngFilterType.Paeth);

        let state: u64 = 0x1E3779B97F4A7C15;
        let buf: [256]u8 = [0; 256];
        let iter: usize = 0;
        let rejected: usize = 0;
        let flip_i: usize = 0;
        let flips: usize = 0;
        let fpos: usize = 0;

        while (iter < 1200) : (iter += 1) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            const r = state *% 0x2545F4914F6CDD1D;

            const mode = iter % 4;
            let len: usize = 0;

            if (mode == 0) {
                len = (r % 129) as usize;
                let j: usize = 0;
                while (j < len) : (j += 1) {
                    state ^= state >> 12;
                    state ^= state << 25;
                    state ^= state >> 27;
                    buf[j] = (state & 0xFF) as u8;
                }
            } else if (mode == 1) {
                len = (8 as usize) + ((r % 120) as usize);
                buf[0] = 0x89; buf[1] = 0x50; buf[2] = 0x4E; buf[3] = 0x47;
                buf[4] = 0x0D; buf[5] = 0x0A; buf[6] = 0x1A; buf[7] = 0x0A;
                let j: usize = 8;
                while (j < len) : (j += 1) {
                    state ^= state >> 12;
                    state ^= state << 25;
                    state ^= state >> 27;
                    buf[j] = (state & 0xFF) as u8;
                }
            } else if (mode == 2) {
                len = (r % seed_png.len) as usize;
                let j: usize = 0;
                while (j < len) : (j += 1) buf[j] = seed_png[j];
            } else {
                len = if (seed_png.len < 256) seed_png.len else 256;
                let j: usize = 0;
                while (j < len) : (j += 1) buf[j] = seed_png[j];
                flips = (1 as usize) + ((r % 3) as usize);
                flip_i = 0;
                while (flip_i < flips) {
                    state ^= state >> 12;
                    state ^= state << 25;
                    state ^= state >> 27;
                    fpos = (state % (len as u64)) as usize;
                    buf[fpos] ^= (1 + ((state >> 8) % 255)) as u8;
                    flip_i += 1;
                }
            }

            const slice = buf[0..len];
            * = Png.probe(slice);
            const decoded_opt = Png.decode(slice);
            if (decoded_opt == null) {
                rejected += 1;
            }
        }

        expect_eq(rejected >= 1150, true);
    });
}
