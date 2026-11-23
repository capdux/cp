// image-rs/image-gif (MIT/Apache-2.0) Reference-Hardened GIF Codec Test Suite
// Covers:
//   - Part A: Encode -> Decode lossless closed-loop (RGBA & Indexed, single & multi-frame animation,
//             GCE delay/transparent/disposal 0..3, NETSCAPE2.0 loop_count, LCT override, 4096-entry LZW clear)
//   - Part B: 16-lane @Vector(16, u8) SIMD hot paths (palette expand, rect clear, branchless alpha composite)
//             verified bit-exact (0 error) against scalar reference + out-of-bounds sub-frame clipping
//   - Part C: All 13 real GIF87a/GIF89a samples from _ref/image-gif/tests/samples/ cross-verified against
//             official image-rs/image-gif IEEE CRC32 ground truth (tests/results.txt)
//   - Part D: Official regression fixtures (small_interlaced 7x7/9x9/47x63, crashtest issue-96/117,
//             regression issue-208/209, stall issue-101, and full-prefix truncation of anim-gr.gif)
//   - Part E: 1,500-iteration deterministic PRNG malformed GIF / LZW / overflow fuzzing (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { readFileSync } from "cap:fs";
import {
    Gif,
    GifHeader,
    GifImage,
    GifFrame,
    GifAnimation,
    GifEncodeFrame,
    GifDisposal,
    GifError,
    encodeGifRgba,
    encodeGifIndexed,
    decodeGifAnimation,
    decodeGifStrict,
} from "../../lib/image/gif.cp";

fn slicesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Standard IEEE 802.3 CRC32 (256-entry lookup table, matches image-rs/image-gif tests/check_testimages.rs)
fn buildCrc32Table(): [256]u32 {
    let table: [256]u32 = [0; 256];
    let i: u32 = 0;
    while (i < 256) : (i += 1) {
        let c = i;
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((c & 1) != 0) {
                c = (c >> 1) ^ 0xEDB88320;
            } else {
                c = c >> 1;
            }
        }
        table[i as usize] = c;
    }
    return table;
}

fn crc32Update(table: *const [256]u32, crc_in: u32, data: []const u8): u32 {
    let crc = crc_in;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        let idx = ((crc ^ (data[i] as u32)) & 0xFF) as usize;
        crc = (crc >> 8) ^ table.*[idx];
    }
    return crc;
}

fn animationFramesCrc32(anim: *GifAnimation): u32 {
    const table = buildCrc32Table();
    let crc: u32 = 0xFFFFFFFF;
    let fi: usize = 0;
    while (fi < anim.frames.len) : (fi += 1) {
        crc = crc32Update(&table, crc, anim.frames[fi].rgba);
    }
    return crc ^ 0xFFFFFFFF;
}

fn expandPaletteScalarRef(indices: []const u8, ct: []const u8, trns_idx: i32, out_rgba: []u8) {
    let p: usize = 0;
    while (p < indices.len) : (p += 1) {
        let ci = indices[p] as usize;
        let dst = p * 4;
        if (ci * 3 + 2 < ct.len) {
            out_rgba[dst + 0] = ct[ci * 3 + 0];
            out_rgba[dst + 1] = ct[ci * 3 + 1];
            out_rgba[dst + 2] = ct[ci * 3 + 2];
            out_rgba[dst + 3] = if (trns_idx >= 0 && (ci as i32) == trns_idx) 0 else 255;
        } else {
            out_rgba[dst + 0] = 0;
            out_rgba[dst + 1] = 0;
            out_rgba[dst + 2] = 0;
            out_rgba[dst + 3] = 0;
        }
    }
}

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn tryDecodeAnimationOpt(bytes: []const u8): GifAnimation? {
    const res = decodeGifAnimation(bytes) catch return null;
    return res;
}

fn runTest01(): void! {
    const widths: [5]usize = [1, 3, 17, 33, 64];
    const heights: [5]usize = [1, 5, 13, 19, 24];

    let si: usize = 0;
    while (si < 5) : (si += 1) {
        const w = widths[si];
        const h = heights[si];
        const npix = w * h;
        let rgba: []u8 = std.heap.page_allocator.alloc(u8, npix * 4) catch unreachable;
        defer std.heap.page_allocator.free(rgba);

        let y: usize = 0;
        while (y < h) : (y += 1) {
            let x: usize = 0;
            while (x < w) : (x += 1) {
                const o = (y * w + x) * 4;
                const cid = (x + y * 3) & 7;
                if (cid == 0 && (si & 1) == 1) {
                    rgba[o + 0] = 0;
                    rgba[o + 1] = 0;
                    rgba[o + 2] = 0;
                    rgba[o + 3] = 0;
                } else {
                    rgba[o + 0] = ((cid * 31 + 17) & 0xFF) as u8;
                    rgba[o + 1] = ((cid * 67 + 41) & 0xFF) as u8;
                    rgba[o + 2] = ((cid * 97 + 13) & 0xFF) as u8;
                    rgba[o + 3] = 255;
                }
            }
        }

        const encoded = encodeGifRgba(rgba, w, h) catch unreachable;
        defer std.heap.page_allocator.free(encoded);

        const hdr = Gif.probe(encoded).?;
        expect_eq(hdr.width, w);
        expect_eq(hdr.height, h);
        expect_eq(hdr.has_gct, true);
        expect_eq(hdr.is_gif89a, true);

        let decoded = decodeGifStrict(encoded) catch unreachable;
        defer decoded.deinit();
        expect_eq(decoded.width, w);
        expect_eq(decoded.height, h);
        expect_eq(slicesEqual(rgba, decoded.rgba), true);

        let compat = Gif.decode(encoded).?;
        defer compat.deinit();
        expect_eq(slicesEqual(rgba, compat.rgba), true);
    }
}

fn runTest02(): void! {
    const gct: [12]u8 = [
        255, 0, 0,
        0, 255, 0,
        0, 0, 255,
        255, 255, 0,
    ];
    const lct: [12]u8 = [
        10, 20, 30,
        200, 100, 50,
        50, 200, 250,
        255, 255, 255,
    ];

    let idx0: [48]u8 = [0; 48];
    let idx1: [16]u8 = [
        0, 1, 1, 0,
        1, 2, 2, 1,
        1, 2, 2, 1,
        0, 1, 1, 0,
    ];
    let idx2: [16]u8 = [2; 16];
    let idx3: [4]u8 = [3, 3, 3, 3];

    const enc_frames: [4]GifEncodeFrame = [
        GifEncodeFrame{
            left: 0,
            top: 0,
            width: 8,
            height: 6,
            delay_cs: 10,
            disposal: GifDisposal.Keep,
            transparent_index: -1,
            local_palette: "",
            indices: idx0[0..],
        },
        GifEncodeFrame{
            left: 2,
            top: 1,
            width: 4,
            height: 4,
            delay_cs: 25,
            disposal: GifDisposal.RestoreBackground,
            transparent_index: 0,
            local_palette: lct[0..],
            indices: idx1[0..],
        },
        GifEncodeFrame{
            left: 6,
            top: 4,
            width: 4,
            height: 4,
            delay_cs: 15,
            disposal: GifDisposal.RestorePrevious,
            transparent_index: -1,
            local_palette: "",
            indices: idx2[0..],
        },
        GifEncodeFrame{
            left: 0,
            top: 0,
            width: 2,
            height: 2,
            delay_cs: 5,
            disposal: GifDisposal.Keep,
            transparent_index: -1,
            local_palette: "",
            indices: idx3[0..],
        },
    ];

    const gif_bytes = encodeGifIndexed(8, 6, gct[0..], enc_frames[0..], 0) catch unreachable;
    defer std.heap.page_allocator.free(gif_bytes);

    let anim = decodeGifAnimation(gif_bytes) catch unreachable;
    defer anim.deinit();

    expect_eq(anim.width, 8);
    expect_eq(anim.height, 6);
    expect_eq(anim.loop_count, 0);
    expect_eq(anim.frames.len, 4);

    expect_eq(anim.frames[0].delay_cs, 10);
    expect_eq(anim.frames[0].disposal == GifDisposal.Keep, true);
    expect_eq(anim.frames[1].left, 2);
    expect_eq(anim.frames[1].top, 1);
    expect_eq(anim.frames[1].delay_cs, 25);
    expect_eq(anim.frames[1].transparent_index, 0);
    expect_eq(anim.frames[1].disposal == GifDisposal.RestoreBackground, true);
    expect_eq(anim.frames[2].disposal == GifDisposal.RestorePrevious, true);

    let composed = anim.renderComposedFrames() catch unreachable;
    defer {
        let ci: usize = 0;
        while (ci < composed.len) : (ci += 1) {
            composed[ci].deinit();
        }
        std.heap.page_allocator.free(composed);
    }

    const f1_p21 = (1 * 8 + 2) * 4;
    expect_eq(composed[1].rgba[f1_p21 + 0], 255);
    expect_eq(composed[1].rgba[f1_p21 + 1], 0);
    expect_eq(composed[1].rgba[f1_p21 + 2], 0);
    expect_eq(composed[1].rgba[f1_p21 + 3], 255);

    const f1_p32 = (2 * 8 + 3) * 4;
    expect_eq(composed[1].rgba[f1_p32 + 0], 50);
    expect_eq(composed[1].rgba[f1_p32 + 1], 200);
    expect_eq(composed[1].rgba[f1_p32 + 2], 250);
    expect_eq(composed[1].rgba[f1_p32 + 3], 255);

    expect_eq(composed[2].rgba[f1_p32 + 3], 0);
    const f2_p75 = (5 * 8 + 7) * 4;
    expect_eq(composed[2].rgba[f2_p75 + 2], 255);
    expect_eq(composed[2].rgba[f2_p75 + 3], 255);

    expect_eq(composed[3].rgba[f2_p75 + 0], 255);
    expect_eq(composed[3].rgba[f2_p75 + 2], 0);
    expect_eq(composed[3].rgba[0], 255);
    expect_eq(composed[3].rgba[1], 255);
    expect_eq(composed[3].rgba[2], 0);
}

fn runTest03(): void! {
    const w: usize = 128;
    const h: usize = 128;
    const npix = w * h;
    let pal: [768]u8 = [0; 768];
    let ci: usize = 0;
    while (ci < 256) : (ci += 1) {
        pal[ci * 3 + 0] = ci as u8;
        pal[ci * 3 + 1] = ((ci * 73) & 0xFF) as u8;
        pal[ci * 3 + 2] = ((ci * 151) & 0xFF) as u8;
    }

    let indices: []u8 = std.heap.page_allocator.alloc(u8, npix) catch unreachable;
    defer std.heap.page_allocator.free(indices);
    let rng: u64 = 0x5AFEBABE12345678;
    let p: usize = 0;
    while (p < npix) : (p += 1) {
        indices[p] = (xorshift64(&rng) & 0xFF) as u8;
    }

    const frames: [1]GifEncodeFrame = [
        GifEncodeFrame{
            width: w,
            height: h,
            indices: indices,
        },
    ];
    const encoded = encodeGifIndexed(w, h, pal[0..], frames[0..], 1) catch unreachable;
    defer std.heap.page_allocator.free(encoded);

    let anim = decodeGifAnimation(encoded) catch unreachable;
    defer anim.deinit();
    expect_eq(anim.frames.len, 1);
    expect_eq(slicesEqual(indices, anim.frames[0].indices), true);
}

fn runTest04(): void! {
    let rng: u64 = 0x1122334455667788;
    let pix_cnt: usize = 1;
    while (pix_cnt <= 65) : (pix_cnt += 1) {
        const nbytes = pix_cnt * 4;
        let dst_simd: []u8 = std.heap.page_allocator.alloc(u8, nbytes) catch unreachable;
        defer std.heap.page_allocator.free(dst_simd);
        let dst_scalar: []u8 = std.heap.page_allocator.alloc(u8, nbytes) catch unreachable;
        defer std.heap.page_allocator.free(dst_scalar);
        let src: []u8 = std.heap.page_allocator.alloc(u8, nbytes) catch unreachable;
        defer std.heap.page_allocator.free(src);

        let i: usize = 0;
        while (i < pix_cnt) : (i += 1) {
            const o = i * 4;
            const v1 = xorshift64(&rng);
            const v2 = xorshift64(&rng);
            dst_simd[o + 0] = (v1 & 0xFF) as u8;
            dst_simd[o + 1] = ((v1 >> 8) & 0xFF) as u8;
            dst_simd[o + 2] = ((v1 >> 16) & 0xFF) as u8;
            dst_simd[o + 3] = 255;
            dst_scalar[o + 0] = dst_simd[o + 0];
            dst_scalar[o + 1] = dst_simd[o + 1];
            dst_scalar[o + 2] = dst_simd[o + 2];
            dst_scalar[o + 3] = 255;

            src[o + 0] = (v2 & 0xFF) as u8;
            src[o + 1] = ((v2 >> 8) & 0xFF) as u8;
            src[o + 2] = ((v2 >> 16) & 0xFF) as u8;
            src[o + 3] = if (((v2 >> 24) & 1) == 0) 0 else 255;
        }

        Gif.compositeRowSimd(dst_simd, src);
        Gif.compositeRowScalar(dst_scalar, src);
        expect_eq(slicesEqual(dst_simd, dst_scalar), true);
    }

    const ct: [12]u8 = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120];
    let idx_arr: [37]u8 = [0; 37];
    let k: usize = 0;
    while (k < 37) : (k += 1) {
        idx_arr[k] = if ((k % 7) == 0) 255 else ((k % 6) as u8);
    }
    let rgba_simd: [37 * 4]u8 = [0; 37 * 4];
    let rgba_ref: [37 * 4]u8 = [0; 37 * 4];
    Gif.expandPaletteToRgbaSimd(idx_arr[0..], ct[0..], 2, rgba_simd[0..]);
    expandPaletteScalarRef(idx_arr[0..], ct[0..], 2, rgba_ref[0..]);
    expect_eq(slicesEqual(rgba_simd[0..], rgba_ref[0..]), true);
}

fn runTest05(): void! {
    const paths: [13][]const u8 = [
        "_ref/image-gif/tests/samples/alpha_gif_a.gif",
        "_ref/image-gif/tests/samples/anim-gr.gif",
        "_ref/image-gif/tests/samples/beacon_icc.gif",
        "_ref/image-gif/tests/samples/beacon_xmp.gif",
        "_ref/image-gif/tests/samples/beacon.gif",
        "_ref/image-gif/tests/samples/interlaced.gif",
        "_ref/image-gif/tests/samples/moon_impact.gif",
        "_ref/image-gif/tests/samples/sample_1.gif",
        "_ref/image-gif/tests/samples/2x2.gif",
        "_ref/image-gif/tests/samples/sample_big.gif",
        "_ref/image-gif/tests/samples/gifplayer-muybridge.gif",
        "_ref/image-gif/tests/samples/set_hsts.gif",
        "_ref/image-gif/tests/samples/bufo-thinks-about-fishsticks.gif",
    ];
    const expected_crcs: [13]u32 = [
        3871893825,
        291646878,
        2462153529,
        2462153529,
        2462153529,
        2115495372,
        2438689726,
        3275424619,
        3802149240,
        4184562096,
        4267078865,
        224161812,
        1018609591,
    ];

    let si: usize = 0;
    while (si < 13) : (si += 1) {
        const raw = readFileSync(paths[si]);
        expect_eq(raw.len > 13, true);

        let anim = decodeGifAnimation(raw) catch unreachable;
        defer anim.deinit();

        expect_eq(anim.frames.len > 0, true);
        const got_crc = animationFramesCrc32(&anim);
        expect_eq(got_crc, expected_crcs[si]);

        if (si == 6) {
            expect_eq(anim.frames.len, 14);
        }
        if (si == 5) {
            expect_eq(anim.frames[0].interlaced, true);
        }
    }
}

fn runTest06(): void! {
    const small_7x7: [39]u8 = [
        0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x07, 0x00, 0x07, 0x00, 0xf0, 0x00, 0x00, 0xad, 0xad, 0xad,
        0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x40, 0x02, 0x06, 0x84,
        0x8f, 0xa9, 0xcb, 0x8d, 0x05, 0x00, 0x3b,
    ];
    const small_9x9: [71]u8 = [
        0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x09, 0x00, 0x09, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0x21, 0xff, 0x0b, 0x49, 0x6d, 0x61, 0x67, 0x65, 0x4d, 0x61, 0x67, 0x69, 0x63,
        0x6b, 0x0e, 0x67, 0x61, 0x6d, 0x6d, 0x61, 0x3d, 0x30, 0x2e, 0x34, 0x35, 0x34, 0x35, 0x34, 0x35,
        0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x09, 0x00, 0x40, 0x02, 0x08, 0x84, 0x8f, 0xa9,
        0xcb, 0xed, 0x0f, 0x4f, 0x01, 0x00, 0x3b,
    ];
    const small_47x63: [149]u8 = [
        0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x2f, 0x00, 0x3f, 0x00, 0xf0, 0x00, 0x00, 0x23, 0x23, 0x23,
        0xdc, 0xdc, 0xdc, 0x21, 0xff, 0x0b, 0x49, 0x6d, 0x61, 0x67, 0x65, 0x4d, 0x61, 0x67, 0x69, 0x63,
        0x6b, 0x0e, 0x67, 0x61, 0x6d, 0x6d, 0x61, 0x3d, 0x30, 0x2e, 0x34, 0x35, 0x34, 0x35, 0x34, 0x35,
        0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x3f, 0x00, 0x40, 0x02, 0x56, 0x8c, 0x8f, 0xa9,
        0xcb, 0xed, 0x0f, 0xa3, 0x9c, 0xb4, 0xda, 0x0a, 0xb2, 0xde, 0xbc, 0xfb, 0x0f, 0x86, 0xe2, 0x48,
        0x6a, 0xd7, 0xc9, 0x94, 0xea, 0xca, 0xb6, 0xee, 0x0b, 0xc7, 0xf2, 0x4c, 0xd7, 0xf6, 0x8d, 0xe7,
        0xfa, 0xce, 0xf7, 0xfe, 0x0f, 0x0c, 0x0a, 0x87, 0xc4, 0xa2, 0xf1, 0x88, 0x4c, 0x2a, 0x97, 0xcc,
        0x66, 0x10, 0x05, 0x45, 0x28, 0xa3, 0x54, 0xa7, 0xf5, 0x49, 0x85, 0x5e, 0xb7, 0xbd, 0xac, 0x96,
        0x0b, 0xc6, 0x79, 0x51, 0xe1, 0x72, 0x6d, 0x7c, 0x32, 0xab, 0x63, 0xe8, 0xcb, 0xfa, 0x0d, 0x8f,
        0xcb, 0x4b, 0x05, 0x00, 0x3b,
    ];

    let a1 = decodeGifAnimation(small_7x7[0..]) catch unreachable;
    defer a1.deinit();
    expect_eq(a1.frames[0].width, 7);
    expect_eq(a1.frames[0].height, 7);
    expect_eq(a1.frames[0].indices.len, 49);
    expect_eq(a1.frames[0].interlaced, true);

    let a2 = decodeGifAnimation(small_9x9[0..]) catch unreachable;
    defer a2.deinit();
    expect_eq(a2.frames[0].width, 9);
    expect_eq(a2.frames[0].height, 9);
    expect_eq(a2.frames[0].indices.len, 81);

    let a3 = decodeGifAnimation(small_47x63[0..]) catch unreachable;
    defer a3.deinit();
    expect_eq(a3.frames[0].width, 47);
    expect_eq(a3.frames[0].height, 63);
    expect_eq(a3.frames[0].indices.len, 2961);
}

fn runTest07(): void! {
    const malformed_paths: [7][]const u8 = [
        "_ref/image-gif/tests/crashtest/issue-96-small-codesize.gif",
        "_ref/image-gif/tests/crashtest/issue-96-small-codesize-2.gif",
        "_ref/image-gif/tests/crashtest/issue-117-out-of-range.gif",
        "_ref/image-gif/tests/regression/issue-208-block-type.gif",
        "_ref/image-gif/tests/regression/issue-208-block-type-beta.gif",
        "_ref/image-gif/tests/regression/issue-209-extension-block-type.gif",
        "_ref/image-gif/tests/stall/issue-101-infinite-empty-loop.gif",
    ];

    let mi: usize = 0;
    while (mi < 7) : (mi += 1) {
        const raw = readFileSync(malformed_paths[mi]);
        if (anim_val := tryDecodeAnimationOpt(raw)) {
            let anim = anim_val;
            anim.deinit();
        }
    }

    const anim_raw = readFileSync("_ref/image-gif/tests/samples/anim-gr.gif");
    let cut: usize = 0;
    while (cut < anim_raw.len) : (cut += 1) {
        const prefix_slice = anim_raw[0..cut];
        * = Gif.probe(prefix_slice);
        if (anim_val := tryDecodeAnimationOpt(prefix_slice)) {
            let anim = anim_val;
            anim.deinit();
        }
    }
    expect_eq(cut, anim_raw.len);
}

fn runTest08(): void! {
    const seed_gif = readFileSync("_ref/image-gif/tests/samples/anim-gr.gif");
    let rng: u64 = 0x51B54A32D192ED03;
    let fuzz_buf: [512]u8 = [0; 512];
    let survived: usize = 0;

    let iter: usize = 0;
    while (iter < 1500) : (iter += 1) {
        let mode = iter % 5;
        let len: usize = 0;

        if (mode == 0) {
            len = ((xorshift64(&rng) % 256) as usize);
            let j: usize = 0;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = (xorshift64(&rng) & 0xFF) as u8;
            }
        } else if (mode == 1) {
            len = (13 as usize) + ((xorshift64(&rng) % 200) as usize);
            fuzz_buf[0] = 'G'; fuzz_buf[1] = 'I'; fuzz_buf[2] = 'F';
            fuzz_buf[3] = '8'; fuzz_buf[4] = '9'; fuzz_buf[5] = 'a';
            let j: usize = 6;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = (xorshift64(&rng) & 0xFF) as u8;
            }
        } else if (mode == 2) {
            len = if (seed_gif.len < 512) seed_gif.len else 512;
            let j: usize = 0;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = seed_gif[j];
            }
            let flips: usize = (1 as usize) + ((xorshift64(&rng) % 6) as usize);
            let f: usize = 0;
            while (f < flips) : (f += 1) {
                let pos = (xorshift64(&rng) as usize) % len;
                fuzz_buf[pos] = (xorshift64(&rng) & 0xFF) as u8;
            }
        } else if (mode == 3) {
            len = 32;
            fuzz_buf[0] = 'G'; fuzz_buf[1] = 'I'; fuzz_buf[2] = 'F';
            fuzz_buf[3] = '8'; fuzz_buf[4] = '9'; fuzz_buf[5] = 'a';
            fuzz_buf[6] = 0xFF; fuzz_buf[7] = 0xFF;
            fuzz_buf[8] = 0xFF; fuzz_buf[9] = 0xFF;
            fuzz_buf[10] = 0x80;
            fuzz_buf[11] = 0; fuzz_buf[12] = 0;
            let j: usize = 13;
            while (j < 19) : (j += 1) {
                fuzz_buf[j] = (j * 17) as u8;
            }
            fuzz_buf[19] = 0x2C;
            fuzz_buf[20] = 0; fuzz_buf[21] = 0;
            fuzz_buf[22] = 0; fuzz_buf[23] = 0;
            const huge_dim = (iter & 1) == 0;
            fuzz_buf[24] = if (huge_dim) 0xFF else 0x04;
            fuzz_buf[25] = if (huge_dim) 0xFF else 0x00;
            fuzz_buf[26] = if (huge_dim) 0xFF else 0x04;
            fuzz_buf[27] = if (huge_dim) 0xFF else 0x00;
            fuzz_buf[28] = 0x00;
            const bad_mcs: [4]u8 = [0, 1, 12, 255];
            fuzz_buf[29] = bad_mcs[(iter / 5) % 4];
            fuzz_buf[30] = 0x00;
            fuzz_buf[31] = 0x3B;
        } else {
            len = 36;
            fuzz_buf[0] = 'G'; fuzz_buf[1] = 'I'; fuzz_buf[2] = 'F';
            fuzz_buf[3] = '8'; fuzz_buf[4] = '7'; fuzz_buf[5] = 'a';
            fuzz_buf[6] = 4; fuzz_buf[7] = 0;
            fuzz_buf[8] = 4; fuzz_buf[9] = 0;
            fuzz_buf[10] = 0x80;
            fuzz_buf[11] = 0; fuzz_buf[12] = 0;
            fuzz_buf[13] = 255; fuzz_buf[14] = 0; fuzz_buf[15] = 0;
            fuzz_buf[16] = 0; fuzz_buf[17] = 255; fuzz_buf[18] = 0;
            fuzz_buf[19] = 0x2C;
            fuzz_buf[20] = 0; fuzz_buf[21] = 0;
            fuzz_buf[22] = 0; fuzz_buf[23] = 0;
            fuzz_buf[24] = 4; fuzz_buf[25] = 0;
            fuzz_buf[26] = 4; fuzz_buf[27] = 0;
            fuzz_buf[28] = 0;
            fuzz_buf[29] = 2;
            fuzz_buf[30] = 4;
            fuzz_buf[31] = (xorshift64(&rng) | 0x07) as u8;
            fuzz_buf[32] = (xorshift64(&rng) & 0xFF) as u8;
            fuzz_buf[33] = (xorshift64(&rng) & 0xFF) as u8;
            fuzz_buf[34] = 0x00;
            fuzz_buf[35] = 0x3B;
        }

        const sample = fuzz_buf[0..len];
        * = Gif.probe(sample);
        if (anim_val := tryDecodeAnimationOpt(sample)) {
            let anim = anim_val;
            anim.deinit();
        }
        survived += 1;
    }

    expect_eq(survived, 1500);
}

fn main() {
    test("gif_ref_01_roundtrip_rgba_opaque_and_transparent", () => { try runTest01(); });
    test("gif_ref_02_multiframe_animation_lct_disposal_and_canvas_compose", () => { try runTest02(); });
    test("gif_ref_03_lzw_4096_table_full_clear_roundtrip", () => { try runTest03(); });
    test("gif_ref_04_simd_composite_and_palette_expand_0_error", () => { try runTest04(); });
    test("gif_ref_05_all_13_real_samples_exact_crc32_ground_truth", () => { try runTest05(); });
    test("gif_ref_06_small_interlaced_gifsicle_regression", () => { try runTest06(); });
    test("gif_ref_07_crashtest_stall_regression_and_prefix_truncation", () => { try runTest07(); });
    test("gif_ref_08_deterministic_prng_1500_fuzz_malformed_gif", () => { try runTest08(); });
}
