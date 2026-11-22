// Source: cp/cap/test/test_tga_production_deep.cp (Production deep tests for cap:tga)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TgaImageType,
    TgaHeader,
    parseTgaHeader,
    encodeTgaTrueColor,
    decodeTgaToRgba,
} from "cap:image";

test("1. parseTgaHeader decodes fields, helper methods, id_length, and color_map offset", () => {
    // 18 header + 4 id + 2*3 colormap (6 bytes) = 28 bytes
    let buf: [28]u8 = std.mem.zeroes([28]u8);
    buf[0] = 4;    // id_length = 4
    buf[1] = 1;    // color_map_type = 1
    buf[2] = 10;   // rle_true_color
    buf[5] = 2;    // cm_length = 2
    buf[7] = 24;   // cm_entry_size = 24 bits -> 3 bytes/entry
    buf[12] = 16;  // width = 16
    buf[14] = 8;   // height = 8
    buf[16] = 32;  // pixel_depth = 32
    buf[17] = 0x28; // top-origin + 8 alpha bits

    const h = parseTgaHeader(buf[0..]) ?? unreachable;
    expect_eq(h.id_length, 4);
    expect_eq(h.color_map_type, 1);
    expect_eq(h.image_type == TgaImageType.rle_true_color, true);
    expect_eq(h.width, 16);
    expect_eq(h.height, 8);
    expect_eq(h.pixel_depth, 32);
    expect_eq(h.isTopOrigin(), true);
    expect_eq(h.isRle(), true);
    expect_eq(h.bytesPerPixel(), 4);
    expect_eq(h.pixelDataOffset(), 28);

    // Reject truncated header, unknown type, zero width, invalid depth=16
    expect_eq(parseTgaHeader(buf[0..17]) == null, true);
    expect_eq(parseTgaHeader(buf[0..27]) == null, true); // pixelDataOffset > len
    buf[2] = 99;
    expect_eq(parseTgaHeader(buf[0..]) == null, true);
});

test("2. 32-bit RGBA encodeTgaTrueColor -> decodeTgaToRgba roundtrip verifies BGRA wire order and buffer bounds", () => {
    const rgba: [16]u8 = [
        10, 20, 30, 255,
        40, 50, 60, 128,
        200, 150, 100, 64,
        1, 2, 3, 0,
    ];
    let tga_buf: [34]u8 = std.mem.zeroes([34]u8);
    const encoded = encodeTgaTrueColor(rgba[0..], 2, 2, 4, tga_buf[0..]) ?? unreachable;
    expect_eq(encoded.len, 34);

    // Wire pixel 0 must be BGRA: (30, 20, 10, 255)
    expect_eq(encoded[18], 30);
    expect_eq(encoded[19], 20);
    expect_eq(encoded[20], 10);
    expect_eq(encoded[21], 255);

    // 1 byte short fails cleanly
    expect_eq(encodeTgaTrueColor(rgba[0..], 2, 2, 4, tga_buf[0..33]) == null, true);

    let decoded: [16]u8 = std.mem.zeroes([16]u8);
    const hdr = decodeTgaToRgba(encoded, decoded[0..]) ?? unreachable;
    expect_eq(hdr.width, 2);
    expect_eq(hdr.height, 2);
    expect_eq(hdr.isTopOrigin(), true);
    expect_eq(std.mem.eql(u8, decoded[0..], rgba[0..]), true);
});

test("3. 24-bit RGB roundtrip and bottom-origin vertical flip to standard top-origin RGBA", () => {
    // 24-bit RGB 2x1
    const rgb: [6]u8 = [255, 0, 128, 10, 20, 30];
    let tga_buf: [24]u8 = std.mem.zeroes([24]u8);
    const enc = encodeTgaTrueColor(rgb[0..], 2, 1, 3, tga_buf[0..]) ?? unreachable;
    let decoded: [8]u8 = std.mem.zeroes([8]u8);
    const h = decodeTgaToRgba(enc, decoded[0..]) ?? unreachable;
    expect_eq(h.bytesPerPixel(), 3);
    const expected_rgb_rgba: [8]u8 = [255, 0, 128, 255, 10, 20, 30, 255];
    expect_eq(std.mem.eql(u8, decoded[0..], expected_rgb_rgba[0..]), true);

    // Bottom-origin 1x2 TGA (row 0 in file is bottom row, row 1 is top row)
    let bot_tga: [24]u8 = std.mem.zeroes([24]u8);
    bot_tga[2] = 2;   // true_color
    bot_tga[12] = 1;  // width = 1
    bot_tga[14] = 2;  // height = 2
    bot_tga[16] = 24; // 24-bit
    bot_tga[17] = 0;  // bottom-origin
    // File row 0 (bottom): BGR = (0, 0, 255) -> Red
    bot_tga[18] = 0; bot_tga[19] = 0; bot_tga[20] = 255;
    // File row 1 (top): BGR = (255, 0, 0) -> Blue
    bot_tga[21] = 255; bot_tga[22] = 0; bot_tga[23] = 0;

    let flipped: [8]u8 = std.mem.zeroes([8]u8);
    const fh = decodeTgaToRgba(bot_tga[0..], flipped[0..]) ?? unreachable;
    expect_eq(fh.isTopOrigin(), false);
    // Top row should now be Blue, bottom row Red
    const expected_flip: [8]u8 = [0, 0, 255, 255, 255, 0, 0, 255];
    expect_eq(std.mem.eql(u8, flipped[0..], expected_flip[0..]), true);
});

test("4. RLE decoding for 32-bit TrueColor (RLE + Raw packets) and 8-bit Grayscale", () => {
    // 2x2 RLE 32-bit TrueColor (top-origin):
    // Packet 1: RLE repeat 3 pixels of BGRA(30, 20, 10, 255) -> header 0x82 (128 | 2) + 4 bytes
    // Packet 2: Raw 1 pixel of BGRA(90, 80, 70, 200) -> header 0x00 + 4 bytes
    let rle_tc: [28]u8 = std.mem.zeroes([28]u8);
    rle_tc[2] = 10;    // rle_true_color
    rle_tc[12] = 2;    // width = 2
    rle_tc[14] = 2;    // height = 2
    rle_tc[16] = 32;   // 32-bit
    rle_tc[17] = 0x28; // top-origin
    rle_tc[18] = 0x82; rle_tc[19] = 30; rle_tc[20] = 20; rle_tc[21] = 10; rle_tc[22] = 255;
    rle_tc[23] = 0x00; rle_tc[24] = 90; rle_tc[25] = 80; rle_tc[26] = 70; rle_tc[27] = 200;

    let out16: [16]u8 = std.mem.zeroes([16]u8);
    const h1 = decodeTgaToRgba(rle_tc[0..], out16[0..]) ?? unreachable;
    expect_eq(h1.isRle(), true);
    const exp_tc: [16]u8 = [
        10, 20, 30, 255,
        10, 20, 30, 255,
        10, 20, 30, 255,
        70, 80, 90, 200,
    ];
    expect_eq(std.mem.eql(u8, out16[0..], exp_tc[0..]), true);

    // 2x1 RLE 8-bit grayscale: RLE repeat 2 of 0x77 -> header 0x81 + 0x77
    let rle_gray: [20]u8 = std.mem.zeroes([20]u8);
    rle_gray[2] = 11;   // rle_grayscale
    rle_gray[12] = 2;
    rle_gray[14] = 1;
    rle_gray[16] = 8;
    rle_gray[17] = 0x20;
    rle_gray[18] = 0x81;
    rle_gray[19] = 0x77;

    let out8: [8]u8 = std.mem.zeroes([8]u8);
    const h2 = decodeTgaToRgba(rle_gray[0..], out8[0..]) ?? unreachable;
    expect_eq(h2.image_type == TgaImageType.rle_grayscale, true);
    const exp_gray: [8]u8 = [0x77, 0x77, 0x77, 255, 0x77, 0x77, 0x77, 255];
    expect_eq(std.mem.eql(u8, out8[0..], exp_gray[0..]), true);
});

test("5. Malformed and out-of-bounds protection: truncated payload, RLE count overflow, undersized output", () => {
    let out16: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(decodeTgaToRgba("", out16[0..]) == null, true);

    // Truncated uncompressed 2x2 24-bit TGA (needs 12 bytes, only 6 provided)
    let trunc_raw: [24]u8 = std.mem.zeroes([24]u8);
    trunc_raw[2] = 2;
    trunc_raw[12] = 2;
    trunc_raw[14] = 2;
    trunc_raw[16] = 24;
    trunc_raw[17] = 0x20;
    expect_eq(decodeTgaToRgba(trunc_raw[0..], out16[0..]) == null, true);

    // RLE packet count exceeds total_pixels (1x1 image, RLE packet claims 5 pixels)
    let ovf_rle: [22]u8 = std.mem.zeroes([22]u8);
    ovf_rle[2] = 10;
    ovf_rle[12] = 1;
    ovf_rle[14] = 1;
    ovf_rle[16] = 24;
    ovf_rle[17] = 0x20;
    ovf_rle[18] = 0x84; // count = 5 > 1
    expect_eq(decodeTgaToRgba(ovf_rle[0..], out16[0..]) == null, true);

    // Output buffer too small for 2x2 RGBA (needs 16 bytes, only 12 given)
    let ok_tga: [30]u8 = std.mem.zeroes([30]u8);
    ok_tga[2] = 2;
    ok_tga[12] = 2;
    ok_tga[14] = 2;
    ok_tga[16] = 24;
    ok_tga[17] = 0x20;
    expect_eq(decodeTgaToRgba(ok_tga[0..], out16[0..12]) == null, true);
});
