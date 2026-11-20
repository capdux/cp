// Source: cp/cap/test/test_bmp_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:bmp & cap.bmp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BmpImage,
    BmpPixelFormat,
    BmpError,
    encode24,
    encode32,
    decode,
    decodeWithMaxDimensions,
} from "cap:bmp";

fn writeU16LE(buf: []u8, pos: usize, val: u16) {
    buf[pos] = (val & 0xFF) as u8;
    buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, pos: usize, val: u32) {
    buf[pos] = (val & 0xFF) as u8;
    buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
    buf[pos + 2] = ((val >> 16) & 0xFF) as u8;
    buf[pos + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeI32LE(buf: []u8, pos: usize, val: i32) {
    const u: u32 = @bitCast(val);
    writeU32LE(buf, pos, u);
}

test("bmp_24bit_non4_multiple_row_padding_roundtrip") {
    const alloc = std.testing.allocator;

    // 3x2 RGB (row = 9 bytes -> padded to 12 bytes, 3 padding zero bytes per row)
    const rgb_3x2: [18]u8 = [
        255, 0, 0,   0, 255, 0,   0, 0, 255,
        12, 34, 56,  78, 90, 123, 200, 210, 220,
    ];
    const enc_3x2 = try encode24(alloc, 3, 2, &rgb_3x2);
    defer alloc.free(enc_3x2);

    // 54 header + 2 * 12 = 78 bytes
    expect_eq(enc_3x2.len, 78);
    expect_eq(enc_3x2[0], 'B');
    expect_eq(enc_3x2[1], 'M');
    // Verify padding bytes at end of first stored row (bytes 63..65) are 0
    expect_eq(enc_3x2[63], 0);
    expect_eq(enc_3x2[64], 0);
    expect_eq(enc_3x2[65], 0);

    let dec_3x2 = try decode(alloc, enc_3x2);
    defer dec_3x2.deinit(alloc);
    expect_eq(dec_3x2.width, 3);
    expect_eq(dec_3x2.height, 2);
    expect_eq(dec_3x2.format == BmpPixelFormat.Rgb24, true);
    expect_eq(dec_3x2.bytesPerPixel(), 3);
    expect_eq(std.mem.eql(u8, dec_3x2.pixels, &rgb_3x2), true);

    // 5x3 RGB (row = 15 bytes -> padded to 16 bytes, 1 padding byte per row)
    let rgb_5x3: [45]u8 = undefined;
    for (i of 0..45) {
        rgb_5x3[i] = ((i * 7 + 13) & 0xFF) as u8;
    }
    const enc_5x3 = try encode24(alloc, 5, 3, &rgb_5x3);
    defer alloc.free(enc_5x3);
    expect_eq(enc_5x3.len, 54 + 3 * 16);

    let dec_5x3 = try decode(alloc, enc_5x3);
    defer dec_5x3.deinit(alloc);
    expect_eq(dec_5x3.width, 5);
    expect_eq(dec_5x3.height, 3);
    expect_eq(std.mem.eql(u8, dec_5x3.pixels, &rgb_5x3), true);
}

test("bmp_32bit_rgba_alpha_roundtrip") {
    const alloc = std.testing.allocator;

    const rgba: [16]u8 = [
        255, 10, 20, 0,
        30, 255, 40, 128,
        50, 60, 255, 200,
        100, 150, 200, 255,
    ];
    const encoded = try encode32(alloc, 2, 2, &rgba);
    defer alloc.free(encoded);
    expect_eq(encoded.len, 54 + 16);

    let decoded = try decode(alloc, encoded);
    defer decoded.deinit(alloc);
    expect_eq(decoded.width, 2);
    expect_eq(decoded.height, 2);
    expect_eq(decoded.format == BmpPixelFormat.Rgba32, true);
    expect_eq(decoded.bytesPerPixel(), 4);
    expect_eq(std.mem.eql(u8, decoded.pixels, &rgba), true);
}

test("bmp_top_down_and_8bit_palette_decode") {
    const alloc = std.testing.allocator;

    // 1. Hand-crafted 2x2 top-down (height = -2) 24-bit BMP
    // Row stride = 2*3 = 6 -> padded to 8 bytes
    let td_bmp: [70]u8 = [0; 70];
    td_bmp[0] = 'B';
    td_bmp[1] = 'M';
    writeU32LE(&td_bmp, 2, 70);
    writeU32LE(&td_bmp, 10, 54);
    writeU32LE(&td_bmp, 14, 40);
    writeI32LE(&td_bmp, 18, 2);
    writeI32LE(&td_bmp, 22, -2); // top-down
    writeU16LE(&td_bmp, 26, 1);
    writeU16LE(&td_bmp, 28, 24);
    // Row 0 (top): BGR (10,20,30), (40,50,60) -> RGB (30,20,10), (60,50,40)
    td_bmp[54] = 10; td_bmp[55] = 20; td_bmp[56] = 30;
    td_bmp[57] = 40; td_bmp[58] = 50; td_bmp[59] = 60;
    // Row 1 (bottom): BGR (70,80,90), (100,110,120) -> RGB (90,80,70), (120,110,100)
    td_bmp[62] = 70; td_bmp[63] = 80; td_bmp[64] = 90;
    td_bmp[65] = 100; td_bmp[66] = 110; td_bmp[67] = 120;

    let td_img = try decode(alloc, &td_bmp);
    defer td_img.deinit(alloc);
    expect_eq(td_img.width, 2);
    expect_eq(td_img.height, 2);
    expect_eq(td_img.format == BmpPixelFormat.Rgb24, true);
    const expected_td: [12]u8 = [30, 20, 10, 60, 50, 40, 90, 80, 70, 120, 110, 100];
    expect_eq(std.mem.eql(u8, td_img.pixels, &expected_td), true);

    // 2. Hand-crafted 3x2 bottom-up 8-bit palette BMP (4 palette colors)
    // Header: 54 + 4*4 (16) = 70 offset; row stride = 4 bytes -> 70 + 2*4 = 78 bytes
    let pal_bmp: [78]u8 = [0; 78];
    pal_bmp[0] = 'B';
    pal_bmp[1] = 'M';
    writeU32LE(&pal_bmp, 2, 78);
    writeU32LE(&pal_bmp, 10, 70);
    writeU32LE(&pal_bmp, 14, 40);
    writeI32LE(&pal_bmp, 18, 3);
    writeI32LE(&pal_bmp, 22, 2); // bottom-up
    writeU16LE(&pal_bmp, 26, 1);
    writeU16LE(&pal_bmp, 28, 8);
    writeU32LE(&pal_bmp, 46, 4); // colors_used = 4

    // Palette (BGRA):
    // 0: Red   -> (B=0, G=0, R=255)
    pal_bmp[54] = 0; pal_bmp[55] = 0; pal_bmp[56] = 255;
    // 1: Green -> (B=0, G=255, R=0)
    pal_bmp[58] = 0; pal_bmp[59] = 255; pal_bmp[60] = 0;
    // 2: Blue  -> (B=255, G=0, R=0)
    pal_bmp[62] = 255; pal_bmp[63] = 0; pal_bmp[64] = 0;
    // 3: White -> (B=255, G=255, R=255)
    pal_bmp[66] = 255; pal_bmp[67] = 255; pal_bmp[68] = 255;

    // Bottom row (stored first at 70): [2, 3, 0]
    pal_bmp[70] = 2; pal_bmp[71] = 3; pal_bmp[72] = 0;
    // Top row (stored second at 74): [0, 1, 2]
    pal_bmp[74] = 0; pal_bmp[75] = 1; pal_bmp[76] = 2;

    let pal_img = try decode(alloc, &pal_bmp);
    defer pal_img.deinit(alloc);
    expect_eq(pal_img.width, 3);
    expect_eq(pal_img.height, 2);
    expect_eq(pal_img.format == BmpPixelFormat.Rgb24, true);
    const expected_pal: [18]u8 = [
        255, 0, 0,     0, 255, 0,     0, 0, 255,
        0, 0, 255,     255, 255, 255, 255, 0, 0,
    ];
    expect_eq(std.mem.eql(u8, pal_img.pixels, &expected_pal), true);
}

test("bmp_security_and_error_guards") {
    const alloc = std.testing.allocator;

    const rgb: [12]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
    const valid = try encode24(alloc, 2, 2, &rgb);
    defer alloc.free(valid);

    // 1. Invalid signature
    let bad_sig: [70]u8 = [0; 70];
    @memcpy(bad_sig[0..valid.len], valid);
    bad_sig[0] = 'P';
    const r_sig = decode(alloc, bad_sig[0..valid.len]);
    expect_eq(r_sig == BmpError.InvalidSignature, true);

    // 2. Unsupported compression (BI_RLE8 = 1)
    let bad_comp: [70]u8 = [0; 70];
    @memcpy(bad_comp[0..valid.len], valid);
    writeU32LE(&bad_comp, 30, 1);
    const r_comp = decode(alloc, bad_comp[0..valid.len]);
    expect_eq(r_comp == BmpError.UnsupportedCompression, true);

    // 3. Unsupported bit depth (16-bit)
    let bad_bpp: [70]u8 = [0; 70];
    @memcpy(bad_bpp[0..valid.len], valid);
    writeU16LE(&bad_bpp, 28, 16);
    const r_bpp = decode(alloc, bad_bpp[0..valid.len]);
    expect_eq(r_bpp == BmpError.UnsupportedBitDepth, true);

    // 4. Truncated data
    const r_trunc = decode(alloc, valid[0 .. valid.len - 1]);
    expect_eq(r_trunc == BmpError.TruncatedData, true);

    // 5. ImageTooLarge via decodeWithMaxDimensions
    const r_large = decodeWithMaxDimensions(alloc, valid, 1, 2);
    expect_eq(r_large == BmpError.ImageTooLarge, true);

    // 6. InvalidDimensions (minInt(i32) height)
    let bad_dim: [70]u8 = [0; 70];
    @memcpy(bad_dim[0..valid.len], valid);
    writeU32LE(&bad_dim, 22, 0x8000_0000);
    const r_dim = decode(alloc, bad_dim[0..valid.len]);
    expect_eq(r_dim == BmpError.InvalidDimensions, true);
}

test("bmp_cap_namespace_mount_and_zero_leak") {
    const alloc = std.testing.allocator;

    const rgba: [4]u8 = [42, 84, 126, 255];
    const encoded = try cap.bmp.encode32(alloc, 1, 1, &rgba);
    defer alloc.free(encoded);

    let img = try cap.bmp.decode(alloc, encoded);
    defer img.deinit(alloc);

    expect_eq(img.width, 1);
    expect_eq(img.height, 1);
    expect_eq(img.format == cap.bmp.BmpPixelFormat.Rgba32, true);
    expect_eq(std.mem.eql(u8, img.pixels, &rgba), true);
}
