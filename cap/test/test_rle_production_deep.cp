// Source: cp/cap/test/test_rle_production_deep.cp (Production deep tests for cap:rle)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PACKBITS_NOP,
    PCX_RUN_MASK,
    PCX_MAX_RUN,
    TGA_RLE_FLAG,
    TGA_MAX_PACKET,
    ZERO_RLE_MAX_RUN,
    packbitsMaxEncodedSize,
    packbitsEncode,
    packbitsDecode,
    pcxEncode,
    pcxDecode,
    tgaRleEncode,
    tgaRleDecode,
    zeroRleEncode,
    zeroRleDecode,
} from "cap:rle";

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

test("1. Apple PackBits standard test vector, 0x80 NOP skipping & long-run/random roundtrip", () => {
    expect_eq(PACKBITS_NOP, 0x80 as u8);
    expect_eq(packbitsMaxEncodedSize(0), 0 as usize);
    expect_eq(packbitsMaxEncodedSize(128), 129 as usize);
    expect_eq(packbitsMaxEncodedSize(129), 131 as usize);

    // Official Apple Technical Note TN1023 PackBits vector + embedded 0x80 NOP
    const encoded_vec: [17]u8 = [
        0xFE, 0xAA,             // 3x 0xAA
        0x80,                   // NOP (skipped)
        0x02, 0x80, 0x00, 0x2A, // 3 literals: 80 00 2A
        0xFD, 0xAA,             // 4x 0xAA
        0x03, 0x80, 0x00, 0x2A, 0x22, // 4 literals: 80 00 2A 22
        0xF7, 0xAA,             // 10x 0xAA
        0x80,                   // trailing NOP
    ];
    const expected_dec: [24]u8 = [
        0xAA, 0xAA, 0xAA,
        0x80, 0x00, 0x2A,
        0xAA, 0xAA, 0xAA, 0xAA,
        0x80, 0x00, 0x2A, 0x22,
        0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    ];

    let dec_buf: [64]u8 = std.mem.zeroes([64]u8);
    const dec_len = packbitsDecode(encoded_vec[0..], dec_buf[0..]) ?? unreachable;
    expect_eq(dec_len, 24 as usize);
    expect_eq(bytesEq(dec_buf[0..dec_len], expected_dec[0..]), true);

    // Long run (300 identical bytes -> splits across 128 + 128 + 44) + pseudo-random non-repeating tail
    let raw: [400]u8 = std.mem.zeroes([400]u8);
    let i: usize = 0;
    while (i < 300) {
        raw[i] = 0x5A;
        i += 1;
    }
    while (i < 400) {
        raw[i] = ((i * 37 + 13) & 0xFF) as u8;
        i += 1;
    }

    let enc: [512]u8 = std.mem.zeroes([512]u8);
    let rt: [400]u8 = std.mem.zeroes([400]u8);
    const elen = packbitsEncode(raw[0..], enc[0..]) ?? unreachable;
    expect_eq(elen < 130, true); // high compression on 300-byte run
    const dlen = packbitsDecode(enc[0..elen], rt[0..]) ?? unreachable;
    expect_eq(dlen, 400 as usize);
    expect_eq(bytesEq(rt[0..dlen], raw[0..]), true);
});

test("2. ZSoft PCX RLE roundtrip with >= 0xC0 high-bit literals & > 63 runs", () => {
    expect_eq(PCX_RUN_MASK, 0xC0 as u8);
    expect_eq(PCX_MAX_RUN, 63 as usize);

    // Mix of low literal (0x41), high literal >= 0xC0 (0xC0, 0xFF), and 150-byte run (splits 63+63+24)
    let raw: [160]u8 = std.mem.zeroes([160]u8);
    raw[0] = 0x41;
    raw[1] = 0xC0;
    raw[2] = 0xFF;
    raw[3] = 0x00;
    let i: usize = 4;
    while (i < 154) {
        raw[i] = 0xEE;
        i += 1;
    }
    raw[154] = 0x01;
    raw[155] = 0xC1;
    raw[156] = 0xC2;
    raw[157] = 0x7F;
    raw[158] = 0xBF;
    raw[159] = 0xD0;

    let enc: [256]u8 = std.mem.zeroes([256]u8);
    let dec_buf: [160]u8 = std.mem.zeroes([160]u8);
    const elen = pcxEncode(raw[0..], enc[0..]) ?? unreachable;
    expect_eq(elen < 40, true);
    const dlen = pcxDecode(enc[0..elen], dec_buf[0..]) ?? unreachable;
    expect_eq(dlen, 160 as usize);
    expect_eq(bytesEq(dec_buf[0..dlen], raw[0..]), true);
});

test("3. Truevision TGA RLE 1B indexed, 3B RGB & 4B RGBA pixel packet roundtrip", () => {
    expect_eq(TGA_RLE_FLAG, 0x80 as u8);
    expect_eq(TGA_MAX_PACKET, 128 as usize);

    // 1B indexed pixels
    const px1: [10]u8 = [1, 2, 3, 7, 7, 7, 7, 7, 9, 10];
    let enc1: [32]u8 = std.mem.zeroes([32]u8);
    let dec1: [10]u8 = std.mem.zeroes([10]u8);
    const e1 = tgaRleEncode(px1[0..], 1, enc1[0..]) ?? unreachable;
    const d1 = tgaRleDecode(enc1[0..e1], 1, dec1[0..]) ?? unreachable;
    expect_eq(d1, 10 as usize);
    expect_eq(bytesEq(dec1[0..], px1[0..]), true);

    // 3B RGB pixels: 4x Red + 2 raw pixels + 3x Blue = 9 pixels (27 bytes)
    const rgb: [27]u8 = [
        0xFF, 0x00, 0x00,
        0xFF, 0x00, 0x00,
        0xFF, 0x00, 0x00,
        0xFF, 0x00, 0x00,
        0x10, 0x20, 0x30,
        0x40, 0x50, 0x60,
        0x00, 0x00, 0xFF,
        0x00, 0x00, 0xFF,
        0x00, 0x00, 0xFF,
    ];
    let enc3: [64]u8 = std.mem.zeroes([64]u8);
    let dec3: [27]u8 = std.mem.zeroes([27]u8);
    const e3 = tgaRleEncode(rgb[0..], 3, enc3[0..]) ?? unreachable;
    expect_eq(e3 < 27, true);
    const d3 = tgaRleDecode(enc3[0..e3], 3, dec3[0..]) ?? unreachable;
    expect_eq(d3, 27 as usize);
    expect_eq(bytesEq(dec3[0..], rgb[0..]), true);

    // 4B RGBA pixels: 5x identical RGBA + 1 raw RGBA = 24 bytes
    const rgba: [24]u8 = [
        0x11, 0x22, 0x33, 0xFF,
        0x11, 0x22, 0x33, 0xFF,
        0x11, 0x22, 0x33, 0xFF,
        0x11, 0x22, 0x33, 0xFF,
        0x11, 0x22, 0x33, 0xFF,
        0xAA, 0xBB, 0xCC, 0x80,
    ];
    let enc4: [64]u8 = std.mem.zeroes([64]u8);
    let dec4: [24]u8 = std.mem.zeroes([24]u8);
    const e4 = tgaRleEncode(rgba[0..], 4, enc4[0..]) ?? unreachable;
    expect_eq(e4, 10 as usize); // (1+4) RLE + (1+4) Raw = 10 bytes
    const d4 = tgaRleDecode(enc4[0..e4], 4, dec4[0..]) ?? unreachable;
    expect_eq(d4, 24 as usize);
    expect_eq(bytesEq(dec4[0..], rgba[0..]), true);
});

test("4. Zero-RLE (RLE0) sparse all-zero (> 255 run) & all-non-zero roundtrip", () => {
    expect_eq(ZERO_RLE_MAX_RUN, 255 as usize);

    // Sparse buffer: 300 zeros + non-zero burst + 5 zeros + non-zero tail
    let sparse: [315]u8 = std.mem.zeroes([315]u8);
    sparse[300] = 0x11;
    sparse[301] = 0x22;
    sparse[302] = 0x33;
    // 303..308 are 5 zeros
    sparse[308] = 0x99;
    sparse[309] = 0xAA;
    sparse[314] = 0xFF;

    let enc: [64]u8 = std.mem.zeroes([64]u8);
    let dec_buf: [315]u8 = std.mem.zeroes([315]u8);
    const elen = zeroRleEncode(sparse[0..], enc[0..]) ?? unreachable;
    expect_eq(elen < 20, true);
    const dlen = zeroRleDecode(enc[0..elen], dec_buf[0..]) ?? unreachable;
    expect_eq(dlen, 315 as usize);
    expect_eq(bytesEq(dec_buf[0..], sparse[0..]), true);

    // All non-zero buffer passes through with identical length
    const nz: [5]u8 = [1, 2, 3, 4, 5];
    let enc_nz: [8]u8 = std.mem.zeroes([8]u8);
    let dec_nz: [5]u8 = std.mem.zeroes([5]u8);
    const enz = zeroRleEncode(nz[0..], enc_nz[0..]) ?? unreachable;
    expect_eq(enz, 5 as usize);
    const dnz = zeroRleDecode(enc_nz[0..enz], dec_nz[0..]) ?? unreachable;
    expect_eq(dnz, 5 as usize);
    expect_eq(bytesEq(dec_nz[0..], nz[0..]), true);
});

test("5. Boundary guards: truncated control bytes, invalid bpp_bytes & output buffer overflow rejection", () => {
    let out: [4]u8 = std.mem.zeroes([4]u8);

    // PackBits: truncated literal / truncated repeat / dst overflow
    const pb_trunc_lit: [2]u8 = [0x05, 0xAA]; // claims 6 literals, only 1 left
    expect_eq(packbitsDecode(pb_trunc_lit[0..], out[0..]) == null, true);
    const pb_trunc_run: [1]u8 = [0xFE]; // missing repeat byte
    expect_eq(packbitsDecode(pb_trunc_run[0..], out[0..]) == null, true);
    const pb_overflow: [2]u8 = [0xF7, 0xAA]; // 10 bytes into 4B out
    expect_eq(packbitsDecode(pb_overflow[0..], out[0..]) == null, true);
    expect_eq(packbitsEncode("ABCDEFGH", out[0..]) == null, true);

    // PCX: dangling 0xC0..0xFF without data byte / dst overflow
    const pcx_dangling: [1]u8 = [0xC5];
    expect_eq(pcxDecode(pcx_dangling[0..], out[0..]) == null, true);
    const pcx_overflow: [2]u8 = [0xCA, 0x55]; // 10 bytes into 4B out
    expect_eq(pcxDecode(pcx_overflow[0..], out[0..]) == null, true);

    // TGA: invalid bpp_bytes (0 and 5), unaligned src, truncated RLE packet, dst overflow
    expect_eq(tgaRleEncode("AAAA", 0, out[0..]) == null, true);
    expect_eq(tgaRleEncode("AAAA", 5, out[0..]) == null, true);
    expect_eq(tgaRleEncode("AAAAA", 3, out[0..]) == null, true); // 5 % 3 != 0
    expect_eq(tgaRleDecode("\x81\x01", 3, out[0..]) == null, true); // RLE 3B pixel truncated to 1B
    expect_eq(tgaRleDecode("\x85\xAA", 1, out[0..]) == null, true); // 6 bytes into 4B out

    // Zero-RLE: dangling 0x00, zero count [0x00, 0x00], dst overflow
    expect_eq(zeroRleDecode("\x00", out[0..]) == null, true);
    expect_eq(zeroRleDecode("\x00\x00", out[0..]) == null, true);
    expect_eq(zeroRleDecode("\x00\x0A", out[0..]) == null, true); // 10 zeros into 4B out
});
