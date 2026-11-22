// Source: cp/cap/test/test_braille_production_deep.cp (Production deep tests for cap:braille)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BRAILLE_BASE_CP,
    DOT1,
    DOT2,
    DOT3,
    DOT4,
    DOT5,
    DOT6,
    DOT7,
    DOT8,
    BRAILLE_NUMBER_SIGN,
    BRAILLE_CAPITAL_SIGN,
    dotBitForPixel,
    brailleToUtf8,
    utf8ToBraille,
    encodeBrailleMasks,
    decodeBrailleUtf8,
    asciiToBrailleMask,
    textToBrailleUtf8,
    BrailleCanvas,
} from "cap:codec";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. All 256 8-dot Braille masks (0x00..0xFF) brailleToUtf8/utf8ToBraille & batch roundtrip", () => {
    expect_eq(BRAILLE_BASE_CP, 0x2800 as u32);
    expect_eq(BRAILLE_NUMBER_SIGN, 0x3C as u8);
    expect_eq(BRAILLE_CAPITAL_SIGN, 0x20 as u8);

    // Exact UTF-8 byte checks for U+2800 (blank) and U+28FF (all 8 dots)
    const blank = brailleToUtf8(0x00);
    expect_eq(blank[0], 0xE2 as u8);
    expect_eq(blank[1], 0xA0 as u8);
    expect_eq(blank[2], 0x80 as u8);

    const full = brailleToUtf8(0xFF);
    expect_eq(full[0], 0xE2 as u8);
    expect_eq(full[1], 0xA3 as u8);
    expect_eq(full[2], 0xBF as u8);

    // Exhaustive 256-mask roundtrip
    let all_masks: [256]u8 = std.mem.zeroes([256]u8);
    let i: usize = 0;
    while (i < 256) {
        const m = i as u8;
        all_masks[i] = m;
        const u = brailleToUtf8(m);
        const decoded = utf8ToBraille(u[0..]) ?? unreachable;
        expect_eq(decoded, m);
        i += 1;
    }

    // Batch encode & decode roundtrip
    let utf8_buf: [768]u8 = std.mem.zeroes([768]u8);
    let dec_buf: [256]u8 = std.mem.zeroes([256]u8);
    const enc_len = encodeBrailleMasks(all_masks[0..], utf8_buf[0..]) ?? unreachable;
    expect_eq(enc_len, 768 as usize);
    const dec_len = decodeBrailleUtf8(utf8_buf[0..enc_len], dec_buf[0..]) ?? unreachable;
    expect_eq(dec_len, 256 as usize);
    expect_eq(bytesEq(dec_buf[0..], all_masks[0..]), true);
});

test("2. 2x4 sub-pixel coordinate to 8-dot bit mapping & out-of-range rejection", () => {
    expect_eq(dotBitForPixel(0, 0) ?? unreachable, DOT1);
    expect_eq(dotBitForPixel(0, 1) ?? unreachable, DOT2);
    expect_eq(dotBitForPixel(0, 2) ?? unreachable, DOT3);
    expect_eq(dotBitForPixel(0, 3) ?? unreachable, DOT7);
    expect_eq(dotBitForPixel(1, 0) ?? unreachable, DOT4);
    expect_eq(dotBitForPixel(1, 1) ?? unreachable, DOT5);
    expect_eq(dotBitForPixel(1, 2) ?? unreachable, DOT6);
    expect_eq(dotBitForPixel(1, 3) ?? unreachable, DOT8);

    // Union of all 8 sub-pixels must equal 0xFF
    const union_all = DOT1 | DOT2 | DOT3 | DOT4 | DOT5 | DOT6 | DOT7 | DOT8;
    expect_eq(union_all, 0xFF as u8);

    // Out-of-bounds sub-pixel coordinates
    expect_eq(dotBitForPixel(2, 0) == null, true);
    expect_eq(dotBitForPixel(0, 4) == null, true);
    expect_eq(dotBitForPixel(255, 255) == null, true);
});

test("3. BrailleCanvas diagonal line, rectangle, pixel ops & multi-line UTF-8 rendering", () => {
    // 2 cols x 2 rows = 4x8 sub-pixels
    let cells: [4]u8 = std.mem.zeroes([4]u8);
    let canvas = BrailleCanvas.init(2, 2, cells[0..]) ?? unreachable;
    expect_eq(canvas.pixelWidth(), 4 as usize);
    expect_eq(canvas.pixelHeight(), 8 as usize);

    // Draw diagonal (0,0), (1,1), (2,2), (3,3) across top row of cells
    expect_eq(canvas.setPixel(0, 0), true);
    expect_eq(canvas.setPixel(1, 1), true);
    expect_eq(canvas.setPixel(2, 2), true);
    expect_eq(canvas.setPixel(3, 3), true);

    expect_eq(canvas.getPixel(0, 0), true);
    expect_eq(canvas.getPixel(1, 1), true);
    expect_eq(canvas.getPixel(2, 2), true);
    expect_eq(canvas.getPixel(3, 3), true);
    expect_eq(canvas.getPixel(0, 1), false);

    // Draw a 4x4 rectangle border on bottom row of cells (y = 4..7, x = 0..3)
    let x: usize = 0;
    while (x < 4) {
        expect_eq(canvas.setPixel(x, 4), true);
        expect_eq(canvas.setPixel(x, 7), true);
        x += 1;
    }
    let y: usize = 4;
    while (y < 8) {
        expect_eq(canvas.setPixel(0, y), true);
        expect_eq(canvas.setPixel(3, y), true);
        y += 1;
    }

    // Verify exact cell masks:
    // Top-left cell (0,0): (0,0)=DOT1 | (1,1)=DOT5 = 0x11
    // Top-right cell (1,0): (0,2)=DOT3 | (1,3)=DOT8 = 0x84
    // Bottom-left cell (0,1): left border (0,0..3)=DOT1|DOT2|DOT3|DOT7 + top/bot right (1,0)=DOT4, (1,3)=DOT8
    const expect_bl: u8 = DOT1 | DOT2 | DOT3 | DOT7 | DOT4 | DOT8;
    // Bottom-right cell (1,1): right border (1,0..3)=DOT4|DOT5|DOT6|DOT8 + top/bot left (0,0)=DOT1, (0,3)=DOT7
    const expect_br: u8 = DOT4 | DOT5 | DOT6 | DOT8 | DOT1 | DOT7;

    let rendered: [32]u8 = std.mem.zeroes([32]u8);
    const rlen = canvas.renderUtf8(rendered[0..]) ?? unreachable;
    // 2 rows * (2 cells * 3 bytes) + 1 newline = 13 bytes
    expect_eq(rlen, 13 as usize);
    expect_eq(rendered[6], 0x0A as u8); // '\n' between row 0 and row 1

    expect_eq(utf8ToBraille(rendered[0..3]) ?? unreachable, (DOT1 | DOT5));
    expect_eq(utf8ToBraille(rendered[3..6]) ?? unreachable, (DOT3 | DOT8));
    expect_eq(utf8ToBraille(rendered[7..10]) ?? unreachable, expect_bl);
    expect_eq(utf8ToBraille(rendered[10..13]) ?? unreachable, expect_br);

    // Toggle & clearPixel & clear
    expect_eq(canvas.togglePixel(0, 0), true);
    expect_eq(canvas.getPixel(0, 0), false);
    expect_eq(canvas.togglePixel(0, 0), true);
    expect_eq(canvas.getPixel(0, 0), true);
    expect_eq(canvas.clearPixel(0, 0), true);
    expect_eq(canvas.getPixel(0, 0), false);

    canvas.clear();
    expect_eq(canvas.getPixel(1, 1), false);
    expect_eq(canvas.getPixel(3, 7), false);
});

test("4. Grade-1 Braille text conversion: 'hello 123', capitals & punctuation", () => {
    let out: [64]u8 = std.mem.zeroes([64]u8);
    let masks: [16]u8 = std.mem.zeroes([16]u8);

    // "hello 123" -> h, e, l, l, o, ' ', NUMBER_SIGN, 1(a), 2(b), 3(c) = 10 Braille chars = 30 bytes
    const n1 = textToBrailleUtf8("hello 123", out[0..]) ?? unreachable;
    expect_eq(n1, 30 as usize);
    const m1 = decodeBrailleUtf8(out[0..n1], masks[0..]) ?? unreachable;
    expect_eq(m1, 10 as usize);
    expect_eq(masks[0], 0x13 as u8); // h
    expect_eq(masks[1], 0x11 as u8); // e
    expect_eq(masks[2], 0x07 as u8); // l
    expect_eq(masks[3], 0x07 as u8); // l
    expect_eq(masks[4], 0x15 as u8); // o
    expect_eq(masks[5], 0x00 as u8); // space
    expect_eq(masks[6], BRAILLE_NUMBER_SIGN); // ⠼ (0x3C)
    expect_eq(masks[7], 0x01 as u8); // 1 -> a
    expect_eq(masks[8], 0x03 as u8); // 2 -> b
    expect_eq(masks[9], 0x09 as u8); // 3 -> c

    // Capital letters + punctuation: "Hi, 0!"
    // -> CAP, h, i, ',', ' ', NUM, 0(j), '!' = 8 chars = 24 bytes
    const n2 = textToBrailleUtf8("Hi, 0!", out[0..]) ?? unreachable;
    expect_eq(n2, 24 as usize);
    const m2 = decodeBrailleUtf8(out[0..n2], masks[0..]) ?? unreachable;
    expect_eq(m2, 8 as usize);
    expect_eq(masks[0], BRAILLE_CAPITAL_SIGN);
    expect_eq(masks[1], 0x13 as u8); // h
    expect_eq(masks[2], 0x0A as u8); // i
    expect_eq(masks[3], 0x02 as u8); // ','
    expect_eq(masks[4], 0x00 as u8); // ' '
    expect_eq(masks[5], BRAILLE_NUMBER_SIGN);
    expect_eq(masks[6], 0x1A as u8); // 0 -> j
    expect_eq(masks[7], 0x16 as u8); // '!'

    // Single-char punctuation checks
    expect_eq(asciiToBrailleMask(0x2E) ?? unreachable, 0x32 as u8); // '.'
    expect_eq(asciiToBrailleMask(0x3B) ?? unreachable, 0x06 as u8); // ';'
    expect_eq(asciiToBrailleMask(0x3A) ?? unreachable, 0x12 as u8); // ':'
    expect_eq(asciiToBrailleMask(0x3F) ?? unreachable, 0x26 as u8); // '?'
    expect_eq(asciiToBrailleMask(0x27) ?? unreachable, 0x04 as u8); // '\''
    expect_eq(asciiToBrailleMask(0x2D) ?? unreachable, 0x24 as u8); // '-'
});

test("5. Malformed UTF-8, out-of-bounds canvas coords & buffer overflow rejection", () => {
    // Malformed UTF-8 inputs for utf8ToBraille
    const short_bytes: [2]u8 = [0xE2, 0xA0];
    const long_bytes: [4]u8 = [0xE2, 0xA0, 0x80, 0x00];
    const wrong_lead: [3]u8 = [0xE3, 0xA0, 0x80];
    const below_range: [3]u8 = [0xE2, 0x9F, 0xBF]; // U+27FF
    const above_range: [3]u8 = [0xE2, 0xA4, 0x80]; // U+2900
    const bad_cont_lo: [3]u8 = [0xE2, 0xA0, 0x7F];
    const bad_cont_hi: [3]u8 = [0xE2, 0xA0, 0xC0];

    expect_eq(utf8ToBraille(short_bytes[0..]) == null, true);
    expect_eq(utf8ToBraille(long_bytes[0..]) == null, true);
    expect_eq(utf8ToBraille(wrong_lead[0..]) == null, true);
    expect_eq(utf8ToBraille(below_range[0..]) == null, true);
    expect_eq(utf8ToBraille(above_range[0..]) == null, true);
    expect_eq(utf8ToBraille(bad_cont_lo[0..]) == null, true);
    expect_eq(utf8ToBraille(bad_cont_hi[0..]) == null, true);

    // Batch encode/decode overflow & unaligned UTF-8 rejection
    const two_masks: [2]u8 = [0x01, 0xFF];
    let tiny_utf8: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(encodeBrailleMasks(two_masks[0..], tiny_utf8[0..]) == null, true);

    let tiny_masks: [1]u8 = std.mem.zeroes([1]u8);
    let valid_6: [6]u8 = std.mem.zeroes([6]u8);
    const enc6 = encodeBrailleMasks(two_masks[0..], valid_6[0..]) ?? unreachable;
    expect_eq(enc6, 6 as usize);
    expect_eq(decodeBrailleUtf8(valid_6[0..5], tiny_masks[0..]) == null, true); // not multiple of 3
    expect_eq(decodeBrailleUtf8(valid_6[0..6], tiny_masks[0..]) == null, true); // dst overflow
    expect_eq(decodeBrailleUtf8(above_range[0..], tiny_masks[0..]) == null, true); // non-Braille

    // Unsupported ASCII & buffer overflow in textToBrailleUtf8
    expect_eq(asciiToBrailleMask(0x7E) == null, true); // '~' unsupported
    expect_eq(asciiToBrailleMask(0x80) == null, true); // non-ASCII
    let small_out: [8]u8 = std.mem.zeroes([8]u8);
    expect_eq(textToBrailleUtf8("abc", small_out[0..]) == null, true); // needs 9 bytes
    expect_eq(textToBrailleUtf8("a~b", small_out[0..]) == null, true);

    // BrailleCanvas invalid init, out-of-bounds pixel ops & render buffer overflow
    let buf3: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(BrailleCanvas.init(0, 2, buf3[0..]) == null, true);
    expect_eq(BrailleCanvas.init(2, 0, buf3[0..]) == null, true);
    expect_eq(BrailleCanvas.init(2, 2, buf3[0..]) == null, true); // needs 4 cells

    let buf4: [4]u8 = std.mem.zeroes([4]u8);
    let c = BrailleCanvas.init(2, 2, buf4[0..]) ?? unreachable;
    expect_eq(c.setPixel(4, 0), false);
    expect_eq(c.setPixel(0, 8), false);
    expect_eq(c.clearPixel(4, 0), false);
    expect_eq(c.togglePixel(0, 8), false);
    expect_eq(c.getPixel(4, 0), false);
    expect_eq(c.getPixel(0, 8), false);

    let short_render: [12]u8 = std.mem.zeroes([12]u8); // needs 13 bytes
    expect_eq(c.renderUtf8(short_render[0..]) == null, true);
});
