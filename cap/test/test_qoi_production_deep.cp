// Source: cp/cap/test/test_qoi_production_deep.cp (Production deep tests for cap:qoi)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    QoiHeader,
    isQoi,
    parseQoiHeader,
    encodeQoi,
    decodeQoi,
} from "cap:qoi";

test("1. parseQoiHeader and isQoi validate magic, dimensions, channels, and colorspace", () => {
    const valid_hdr: [14]u8 = [
        'q', 'o', 'i', 'f',
        0, 0, 1, 0, // width = 256
        0, 0, 0, 64, // height = 64
        4, // RGBA
        0, // sRGB
    ];
    expect_eq(isQoi(valid_hdr[0..]), true);
    const h = parseQoiHeader(valid_hdr[0..]) ?? unreachable;
    expect_eq(h.width, 256);
    expect_eq(h.height, 64);
    expect_eq(h.channels, 4);
    expect_eq(h.colorspace, 0);

    // Invalid magic, zero width, invalid channels=2, invalid colorspace=2, short slice
    const bad_magic: [14]u8 = ['p', 'n', 'g', '!', 0, 0, 0, 1, 0, 0, 0, 1, 4, 0];
    expect_eq(isQoi(bad_magic[0..]), false);

    const zero_w: [14]u8 = ['q', 'o', 'i', 'f', 0, 0, 0, 0, 0, 0, 0, 1, 4, 0];
    expect_eq(parseQoiHeader(zero_w[0..]) == null, true);

    const bad_ch: [14]u8 = ['q', 'o', 'i', 'f', 0, 0, 0, 1, 0, 0, 0, 1, 2, 0];
    expect_eq(parseQoiHeader(bad_ch[0..]) == null, true);

    const bad_cs: [14]u8 = ['q', 'o', 'i', 'f', 0, 0, 0, 1, 0, 0, 0, 1, 3, 2];
    expect_eq(parseQoiHeader(bad_cs[0..]) == null, true);

    expect_eq(parseQoiHeader(valid_hdr[0..10]) == null, true);
});

test("2. RGBA lossless encode -> decode roundtrip covers RUN (>62), INDEX, DIFF (wrap), LUMA, RGB, RGBA", () => {
    // 80 pixels RGBA = 320 bytes
    let src: [320]u8 = std.mem.zeroes([320]u8);

    // Pixels 0..64 (65 identical pixels -> triggers 62-max RUN + 3-RUN)
    let i: usize = 0;
    while (i < 65) {
        src[i * 4] = 10;
        src[i * 4 + 1] = 20;
        src[i * 4 + 2] = 30;
        src[i * 4 + 3] = 255;
        i += 1;
    }
    // Pixel 65: DIFF (+1, -1, +0)
    src[65 * 4] = 11;
    src[65 * 4 + 1] = 19;
    src[65 * 4 + 2] = 30;
    src[65 * 4 + 3] = 255;
    // Pixel 66: LUMA (dg = +10, dr = +12, db = +8)
    src[66 * 4] = 23;
    src[66 * 4 + 1] = 29;
    src[66 * 4 + 2] = 38;
    src[66 * 4 + 3] = 255;
    // Pixel 67: full RGB jump (r=255, g=0, b=200, a=255)
    src[67 * 4] = 255;
    src[67 * 4 + 1] = 0;
    src[67 * 4 + 2] = 200;
    src[67 * 4 + 3] = 255;
    // Pixel 68: DIFF wrap-around across 255->0 and 0->255
    src[68 * 4] = 0;
    src[68 * 4 + 1] = 255;
    src[68 * 4 + 2] = 201;
    src[68 * 4 + 3] = 255;
    // Pixel 69: RGBA alpha change (a = 128)
    src[69 * 4] = 0;
    src[69 * 4 + 1] = 255;
    src[69 * 4 + 2] = 201;
    src[69 * 4 + 3] = 128;
    // Pixel 70: INDEX hit back to pixel 0 (10, 20, 30, 255)
    src[70 * 4] = 10;
    src[70 * 4 + 1] = 20;
    src[70 * 4 + 2] = 30;
    src[70 * 4 + 3] = 255;
    // Pixels 71..79: gradient + trailing RUN
    let j: usize = 71;
    while (j < 80) {
        src[j * 4] = 100;
        src[j * 4 + 1] = 150;
        src[j * 4 + 2] = 200;
        src[j * 4 + 3] = 255;
        j += 1;
    }

    let enc_buf: [512]u8 = std.mem.zeroes([512]u8);
    const encoded = encodeQoi(src[0..], 10, 8, 4, 0, enc_buf[0..]) ?? unreachable;
    expect_eq(isQoi(encoded), true);
    // Significant lossless compression due to RUN/INDEX/DIFF/LUMA
    expect_eq(encoded.len < 80, true);

    let dec_buf: [320]u8 = std.mem.zeroes([320]u8);
    const hdr = decodeQoi(encoded, dec_buf[0..]) ?? unreachable;
    expect_eq(hdr.width, 10);
    expect_eq(hdr.height, 8);
    expect_eq(hdr.channels, 4);
    expect_eq(hdr.colorspace, 0);
    expect_eq(std.mem.eql(u8, src[0..], dec_buf[0..]), true);
});

test("3. RGB 3-channel linear colorspace gradient & checkerboard roundtrip", () => {
    // 8x8 RGB = 192 bytes
    let src: [192]u8 = std.mem.zeroes([192]u8);
    let y: usize = 0;
    while (y < 8) {
        let x: usize = 0;
        while (x < 8) {
            const p = (y * 8 + x) * 3;
            src[p] = u8((x * 31 + y * 7) & 0xFF);
            src[p + 1] = u8((x * 13 + y * 29) & 0xFF);
            src[p + 2] = u8(((x + y) % 2) * 255);
            x += 1;
        }
        y += 1;
    }

    let enc_buf: [512]u8 = std.mem.zeroes([512]u8);
    const encoded = encodeQoi(src[0..], 8, 8, 3, 1, enc_buf[0..]) ?? unreachable;

    let dec_buf: [192]u8 = std.mem.zeroes([192]u8);
    const hdr = decodeQoi(encoded, dec_buf[0..]) ?? unreachable;
    expect_eq(hdr.width, 8);
    expect_eq(hdr.height, 8);
    expect_eq(hdr.channels, 3);
    expect_eq(hdr.colorspace, 1);
    expect_eq(std.mem.eql(u8, src[0..], dec_buf[0..]), true);
});

test("4. Exact output buffer sizing for encodeQoi and decodeQoi", () => {
    const px: [12]u8 = [1, 2, 3, 4, 5, 6, 200, 100, 50, 200, 100, 50]; // 2x2 RGB
    let enc_buf: [64]u8 = std.mem.zeroes([64]u8);
    const full = encodeQoi(px[0..], 2, 2, 3, 0, enc_buf[0..]) ?? unreachable;
    const exact_len = full.len;

    // Exact length succeeds
    let exact_buf: [64]u8 = std.mem.zeroes([64]u8);
    const exact = encodeQoi(px[0..], 2, 2, 3, 0, exact_buf[0..exact_len]) ?? unreachable;
    expect_eq(std.mem.eql(u8, full, exact), true);

    // 1 byte too small fails cleanly
    expect_eq(encodeQoi(px[0..], 2, 2, 3, 0, exact_buf[0 .. exact_len - 1]) == null, true);

    // Decode into buffer 1 byte too small fails cleanly
    let dec_small: [11]u8 = std.mem.zeroes([11]u8);
    expect_eq(decodeQoi(full, dec_small[0..]) == null, true);
});

test("5. Malformed stream protection: corrupted end marker, truncated chunk, mismatched pixel length", () => {
    const px: [16]u8 = [
        10, 20, 30, 255,
        40, 50, 60, 255,
        70, 80, 90, 200,
        100, 110, 120, 255,
    ];
    let enc_buf: [64]u8 = std.mem.zeroes([64]u8);
    const encoded = encodeQoi(px[0..], 2, 2, 4, 0, enc_buf[0..]) ?? unreachable;

    // Copy and corrupt end marker
    let bad_end: [64]u8 = std.mem.zeroes([64]u8);
    let i: usize = 0;
    while (i < encoded.len) {
        bad_end[i] = encoded[i];
        i += 1;
    }
    bad_end[encoded.len - 1] = 0x02;
    let dec_buf: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(decodeQoi(bad_end[0..encoded.len], dec_buf[0..]) == null, true);

    // Truncated stream with valid-looking end marker but missing chunk bytes
    const trunc_stream: [23]u8 = [
        'q', 'o', 'i', 'f',
        0, 0, 0, 2,
        0, 0, 0, 2,
        4, 0,
        0xFF, // RGBA tag but 0 bytes follow before end marker!
        0, 0, 0, 0, 0, 0, 0, 1,
    ];
    expect_eq(decodeQoi(trunc_stream[0..], dec_buf[0..]) == null, true);

    // Mismatched pixels slice length on encode
    expect_eq(encodeQoi(px[0..12], 2, 2, 4, 0, enc_buf[0..]) == null, true);
});
