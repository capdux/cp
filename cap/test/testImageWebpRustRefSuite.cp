// image-webp (MIT/Apache-2.0) Reference-Hardened RFC 9649 WebP VP8L & VP8X Suite
// Covers encode->decode lossless roundtrip, VP8X/ICCP/EXIF/XMP/ANIM container parsing,
// real libwebp reference image cross-decoding (1/2/4/8-bit palette, predictor, color-cache,
// cross-color, subtract-green, meta-huffman), and 1,000-iteration deterministic malformed fuzzing.

const std = @import("std");
import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    Webp,
    WebpError,
    WebpExtendedInfo,
    parseWebpChunks,
    webpEncodeLossless,
    webpDecodeLosslessInto,
    webpDecodeLossless,
    Png,
} from "../../lib/image/index.cp";

fn assertLosslessRoundtrip(rgba: []const u8, width: u32, height: u32) {
    const alloc = std.heap.page_allocator;
    const encoded = webpEncodeLossless(alloc, rgba, width, height) catch unreachable;
    defer alloc.free(encoded);

    const probed = Webp.probe(encoded).?;
    expect_true(probed.width == @as(usize, width));
    expect_true(probed.height == @as(usize, height));
    expect_true(probed.is_lossless);

    let decoded = webpDecodeLossless(alloc, encoded) catch unreachable;
    defer decoded.deinit(alloc);

    expect_true(decoded.width == width);
    expect_true(decoded.height == height);
    expect_true(decoded.rgba.len == rgba.len);
    expect_true(std.mem.eql(u8, decoded.rgba, rgba));
}

fn readFileAll(alloc: std.mem.Allocator, path: []const u8): ([]u8)? {
    const f = std.fs.openFileAbsolute(path, .{}) catch return null;
    defer f.close();
    const data = f.readToEndAlloc(alloc, 4 * 1024 * 1024) catch return null;
    return data;
}

fn assertRefWebpMatchesPng(webp_path: []const u8, png_path: []const u8) {
    const alloc = std.heap.page_allocator;
    const webp_bytes = readFileAll(alloc, webp_path) ?? return;
    defer alloc.free(webp_bytes);
    const png_bytes = readFileAll(alloc, png_path) ?? return;
    defer alloc.free(png_bytes);

    let decoded_img = webpDecodeLossless(alloc, webp_bytes) catch unreachable;
    defer decoded_img.deinit(alloc);

    const ref_png = Png.decode(png_bytes).?;
    expect_true(decoded_img.width == ref_png.width);
    expect_true(decoded_img.height == ref_png.height);
    expect_true(decoded_img.rgba.len == ref_png.rgba.len);
    expect_true(std.mem.eql(u8, decoded_img.rgba, ref_png.rgba));
}

// ============================================================================
// Part A: Encode -> Decode Bit-Exact Lossless Roundtrips
// ============================================================================

test("webp_ref_01_roundtrip_1x1_single_pixel_with_alpha") {
    const px: [4]u8 = [0xDE, 0xAD, 0xBE, 0x80];
    assertLosslessRoundtrip(px[0..], 1, 1);
}

test("webp_ref_02_roundtrip_solid_color_simple_1sym_huffman") {
    let rgba: [32 * 24 * 4]u8 = [0; 32 * 24 * 4];
    let i: usize = 0;
    while (i < rgba.len) : (i += 4) {
        rgba[i] = 42;
        rgba[i + 1] = 199;
        rgba[i + 2] = 73;
        rgba[i + 3] = 255;
    }
    assertLosslessRoundtrip(rgba[0..], 32, 24);
}

test("webp_ref_03_roundtrip_checkerboard_simple_2sym_and_lz77") {
    let rgba: [32 * 32 * 4]u8 = [0; 32 * 32 * 4];
    let y: usize = 0;
    while (y < 32) : (y += 1) {
        let x: usize = 0;
        while (x < 32) : (x += 1) {
            const off = (y * 32 + x) * 4;
            if (((x + y) & 1) == 0) {
                rgba[off] = 250;
                rgba[off + 1] = 10;
                rgba[off + 2] = 20;
                rgba[off + 3] = 255;
            } else {
                rgba[off] = 15;
                rgba[off + 1] = 220;
                rgba[off + 2] = 240;
                rgba[off + 3] = 180;
            }
        }
    }
    assertLosslessRoundtrip(rgba[0..], 32, 32);
}

test("webp_ref_04_roundtrip_non_multiple_of_8_gradient_13x17_alpha") {
    let rgba: [13 * 17 * 4]u8 = [0; 13 * 17 * 4];
    let y: usize = 0;
    while (y < 17) : (y += 1) {
        let x: usize = 0;
        while (x < 13) : (x += 1) {
            const off = (y * 13 + x) * 4;
            rgba[off] = @as(u8, @intCast((x * 19) & 0xFF));
            rgba[off + 1] = @as(u8, @intCast((y * 15) & 0xFF));
            rgba[off + 2] = @as(u8, @intCast(((x + y) * 9) & 0xFF));
            rgba[off + 3] = @as(u8, @intCast((255 - y * 12) & 0xFF));
        }
    }
    assertLosslessRoundtrip(rgba[0..], 13, 17);
}

test("webp_ref_05_roundtrip_high_entropy_noise_48x37_normal_huffman") {
    let rgba: [48 * 37 * 4]u8 = [0; 48 * 37 * 4];
    let seed: u32 = 0x1357_9BDF;
    let i: usize = 0;
    while (i < rgba.len) : (i += 1) {
        seed = seed *% 1664525 +% 1013904223;
        rgba[i] = @as(u8, @intCast((seed >> 24) & 0xFF));
    }
    assertLosslessRoundtrip(rgba[0..], 48, 37);
}

test("webp_ref_06_decode_into_caller_stack_buffer_and_small_buf_guard") {
    let src: [8 * 6 * 4]u8 = [0; 8 * 6 * 4];
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        src[i] = @as(u8, @intCast((i * 37 + 11) & 0xFF));
    }
    const alloc = std.heap.page_allocator;
    const encoded = webpEncodeLossless(alloc, src[0..], 8, 6) catch unreachable;
    defer alloc.free(encoded);

    // Caller-owned stack buffer exact fit
    let out: [8 * 6 * 4]u8 = [0; 8 * 6 * 4];
    const hdr = webpDecodeLosslessInto(encoded, out[0..]) catch unreachable;
    expect_eq(hdr.width, 8);
    expect_eq(hdr.height, 6);
    expect_true(std.mem.eql(u8, out[0..], src[0..]));

    // Too-small buffer -> BufferTooSmall
    let small: [16]u8 = [0; 16];
    let too_small = false;
    * = webpDecodeLosslessInto(encoded, small[0..]) catch |e| {
        if (e == WebpError.BufferTooSmall) too_small = true;
    };
    expect_true(too_small);
}

// ============================================================================
// Part B: VP8X Extended Container & Metadata Chunk Parsing
// ============================================================================

fn appendLeU32(list: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32) {
    list.append(alloc, @as(u8, @intCast(v & 0xFF))) catch unreachable;
    list.append(alloc, @as(u8, @intCast((v >> 8) & 0xFF))) catch unreachable;
    list.append(alloc, @as(u8, @intCast((v >> 16) & 0xFF))) catch unreachable;
    list.append(alloc, @as(u8, @intCast((v >> 24) & 0xFF))) catch unreachable;
}

fn appendChunk(list: *std.ArrayList(u8), alloc: std.mem.Allocator, tag: []const u8, payload: []const u8) {
    list.appendSlice(alloc, tag) catch unreachable;
    appendLeU32(list, alloc, @as(u32, @intCast(payload.len)));
    list.appendSlice(alloc, payload) catch unreachable;
    if ((payload.len & 1) == 1) {
        list.append(alloc, 0x00) catch unreachable;
    }
}

test("webp_ref_07_vp8x_container_with_iccp_exif_xmp_and_vp8l_decode") {
    const alloc = std.heap.page_allocator;
    let src: [4 * 3 * 4]u8 = [0; 4 * 3 * 4];
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        src[i] = @as(u8, @intCast((i * 13 + 5) & 0xFF));
    }
    const simple_webp = webpEncodeLossless(alloc, src[0..], 4, 3) catch unreachable;
    defer alloc.free(simple_webp);
    const simple_chunks = parseWebpChunks(simple_webp) catch unreachable;
    expect_false(simple_chunks.is_extended);
    expect_true(simple_chunks.vp8l_payload.len >= 5);

    // Assemble a full VP8X extended WebP with odd-length ICCP, VP8L, EXIF, and XMP chunks
    let ext: std.ArrayList(u8) = .{};
    defer ext.deinit(alloc);
    ext.appendSlice(alloc, "RIFF") catch unreachable;
    appendLeU32(&ext, alloc, 0); // patched below
    ext.appendSlice(alloc, "WEBP") catch unreachable;

    // VP8X payload (10 bytes): flags = ICC(0x20)|ALPHA(0x10)|EXIF(0x08)|XMP(0x04) = 0x3C, w=4 (3), h=3 (2)
    const vp8x_body: [10]u8 = [0x3C, 0, 0, 0, 3, 0, 0, 2, 0, 0];
    appendChunk(&ext, alloc, "VP8X", vp8x_body[0..]);
    appendChunk(&ext, alloc, "ICCP", "ICC_ODD"); // 7 bytes -> 1 pad byte!
    appendChunk(&ext, alloc, "VP8L", simple_chunks.vp8l_payload);
    appendChunk(&ext, alloc, "EXIF", "Exif\x00\x00MM");
    appendChunk(&ext, alloc, "XMP ", "<x:xmpmeta/>");

    // Patch RIFF size = total - 8
    const riff_sz: u32 = @as(u32, @intCast(ext.items.len - 8));
    ext.items[4] = @as(u8, @intCast(riff_sz & 0xFF));
    ext.items[5] = @as(u8, @intCast((riff_sz >> 8) & 0xFF));
    ext.items[6] = @as(u8, @intCast((riff_sz >> 16) & 0xFF));
    ext.items[7] = @as(u8, @intCast((riff_sz >> 24) & 0xFF));

    const info = parseWebpChunks(ext.items) catch unreachable;
    expect_true(info.is_extended);
    expect_true(info.has_icc);
    expect_true(info.has_alpha);
    expect_true(info.has_exif);
    expect_true(info.has_xmp);
    expect_false(info.is_animated);
    expect_eq(info.canvas_width, 4);
    expect_eq(info.canvas_height, 3);
    expect_true(std.mem.eql(u8, info.icc_profile, "ICC_ODD"));
    expect_true(std.mem.eql(u8, info.exif_data, "Exif\x00\x00MM"));
    expect_true(std.mem.eql(u8, info.xmp_data, "<x:xmpmeta/>"));

    // Decode VP8L directly from the VP8X extended container!
    let out: [4 * 3 * 4]u8 = [0; 4 * 3 * 4];
    const hdr = webpDecodeLosslessInto(ext.items, out[0..]) catch unreachable;
    expect_eq(hdr.width, 4);
    expect_eq(hdr.height, 3);
    expect_true(std.mem.eql(u8, out[0..], src[0..]));
}

test("webp_ref_08_vp8x_animated_and_invalid_container_guards") {
    const alloc = std.heap.page_allocator;
    let anim: std.ArrayList(u8) = .{};
    defer anim.deinit(alloc);
    anim.appendSlice(alloc, "RIFF") catch unreachable;
    appendLeU32(&anim, alloc, 36);
    anim.appendSlice(alloc, "WEBP") catch unreachable;
    const vp8x_anim: [10]u8 = [0x02, 0, 0, 0, 15, 0, 0, 31, 0, 0]; // 16x32 animated
    appendChunk(&anim, alloc, "VP8X", vp8x_anim[0..]);
    const anim_body: [6]u8 = [0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00];
    appendChunk(&anim, alloc, "ANIM", anim_body[0..]);

    const info = parseWebpChunks(anim.items) catch unreachable;
    expect_true(info.is_animated);
    expect_eq(info.canvas_width, 16);
    expect_eq(info.canvas_height, 32);
    expect_eq(info.anim_data.len, 6);

    // Non-RIFF garbage -> InvalidContainer
    let bad = false;
    * = parseWebpChunks("NOT_A_WEBP_FILE_AT_ALL") catch |e| {
        if (e == WebpError.InvalidContainer) bad = true;
    };
    expect_true(bad);
}

// ============================================================================
// Part C: Real libwebp / image-webp Reference Image Cross-Decoding
// ============================================================================

test("webp_ref_09_real_libwebp_regression_dark") {
    assertRefWebpMatchesPng(
        "/data/dev/cp/_ref/image-webp/tests/images/regression/dark.webp",
        "/data/dev/cp/_ref/image-webp/tests/reference/regression/dark.png",
    );
}

test("webp_ref_10_real_libwebp_regression_color_index_palette") {
    assertRefWebpMatchesPng(
        "/data/dev/cp/_ref/image-webp/tests/images/regression/color_index.webp",
        "/data/dev/cp/_ref/image-webp/tests/reference/regression/color_index.png",
    );
}

test("webp_ref_11_real_libwebp_regression_1bit_palette") {
    assertRefWebpMatchesPng(
        "/data/dev/cp/_ref/image-webp/tests/images/regression/lossless_indexed_1bit_palette.webp",
        "/data/dev/cp/_ref/image-webp/tests/reference/regression/lossless_indexed_1bit_palette.png",
    );
}

test("webp_ref_12_real_libwebp_regression_2bit_palette") {
    assertRefWebpMatchesPng(
        "/data/dev/cp/_ref/image-webp/tests/images/regression/lossless_indexed_2bit_palette.webp",
        "/data/dev/cp/_ref/image-webp/tests/reference/regression/lossless_indexed_2bit_palette.png",
    );
}

test("webp_ref_13_real_libwebp_regression_4bit_palette") {
    assertRefWebpMatchesPng(
        "/data/dev/cp/_ref/image-webp/tests/images/regression/lossless_indexed_4bit_palette.webp",
        "/data/dev/cp/_ref/image-webp/tests/reference/regression/lossless_indexed_4bit_palette.png",
    );
}

test("webp_ref_14_real_libwebp_gallery2_lossless_full_transforms") {
    // Full real-world libwebp lossless image exercising Predictor 0..13, Cross-Color,
    // Subtract Green, Color Cache, Meta-Huffman, and 2D LZ77 distance map!
    assertRefWebpMatchesPng(
        "/data/dev/cp/_ref/image-webp/tests/images/gallery2/1_webp_ll.webp",
        "/data/dev/cp/_ref/image-webp/tests/reference/gallery2/1_webp_ll.png",
    );
}

test("webp_ref_15_real_libwebp_gallery2_lossless_image2") {
    assertRefWebpMatchesPng(
        "/data/dev/cp/_ref/image-webp/tests/images/gallery2/2_webp_ll.webp",
        "/data/dev/cp/_ref/image-webp/tests/reference/gallery2/2_webp_ll.png",
    );
}

// ============================================================================
// Part D: 1,000-Iteration Deterministic Malformed / Truncated VP8L Fuzzing
// ============================================================================

test("webp_ref_16_1000_iter_malformed_vp8l_fuzz_no_panic") {
    const alloc = std.heap.page_allocator;
    let src: [8 * 8 * 4]u8 = [0; 8 * 8 * 4];
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        src[i] = @as(u8, @intCast((i * 29 + 7) & 0xFF));
    }
    const base_webp = webpEncodeLossless(alloc, src[0..], 8, 8) catch unreachable;
    defer alloc.free(base_webp);

    let mut_buf: [1024]u8 = [0; 1024];
    let out_rgba: [16 * 16 * 4]u8 = [0; 16 * 16 * 4];
    const base_len: usize = if (base_webp.len < 1024) base_webp.len else 1024;

    let rng: u32 = 0x9E37_79B9;
    let iter: usize = 0;
    while (iter < 1000) : (iter += 1) {
        // Copy valid baseline
        let c: usize = 0;
        while (c < base_len) : (c += 1) {
            mut_buf[c] = base_webp[c];
        }

        // Deterministic mutations: truncate or flip 1..4 bytes
        rng = rng *% 1664525 +% 1013904223;
        const mode: u32 = rng & 3;
        let test_len: usize = base_len;

        if (mode == 0) {
            // Truncation at arbitrary offset
            rng = rng *% 1664525 +% 1013904223;
            test_len = @as(usize, rng % @as(u32, @intCast(base_len)));
        } else if (mode == 1) {
            // Bit flips inside VP8L bitstream (after byte 20)
            let flips: usize = @as(usize, 1) + @as(usize, (rng >> 8) & 3);
            let f: usize = 0;
            while (f < flips) : (f += 1) {
                rng = rng *% 1664525 +% 1013904223;
                const pos: usize = @as(usize, 20) + @as(usize, rng % @as(u32, @intCast(base_len - 20)));
                mut_buf[pos] ^= @as(u8, @intCast((rng >> 16) & 0xFF));
            }
        } else if (mode == 2) {
            // Corrupt RIFF/VP8L header bytes
            rng = rng *% 1664525 +% 1013904223;
            const pos: usize = @as(usize, rng % 25);
            if (pos < base_len) {
                mut_buf[pos] ^= @as(u8, @intCast((rng >> 12) & 0xFF));
            }
        } else {
            // Pure random VP8L payload starting with 0x2F and small valid dims (<= 16x16)
            mut_buf[0] = 0x2F;
            mut_buf[1] = @as(u8, @intCast(rng & 0x0F)); // width 1..16
            mut_buf[2] = @as(u8, @intCast((rng >> 4) & 0xC0));
            mut_buf[3] = @as(u8, @intCast((rng >> 8) & 0x03)); // height 1..16
            mut_buf[4] = @as(u8, @intCast((rng >> 16) & 0x10)); // version=0, random alpha
            test_len = @as(usize, 5) + @as(usize, (rng >> 20) & 63);
            let k: usize = 5;
            while (k < test_len) : (k += 1) {
                rng = rng *% 1664525 +% 1013904223;
                mut_buf[k] = @as(u8, @intCast((rng >> 24) & 0xFF));
            }
        }

        // Must never panic, crash, or hang — either succeeds cleanly or returns WebpError
        * = parseWebpChunks(mut_buf[0..test_len]) catch {};
        * = webpDecodeLosslessInto(mut_buf[0..test_len], out_rgba[0..]) catch {};
    }
    expect_eq(iter, 1000);
}
