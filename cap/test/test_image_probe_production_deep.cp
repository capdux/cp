// Source: cp/cap/test/test_image_probe_production_deep.cp (Production deep tests for cap:image_probe)

import { test, expect_eq } from "cap:test";
import {
    ImageFormat,
    WebpKind,
    ImageInfo,
    detectFormat,
    probe,
    probePng,
    probeJpeg,
    probeWebp,
    probeBmp,
    probeGif,
} from "cap:image_probe";

fn makePngHeader(w: u32, h: u32, bit_depth: u8, color_type: u8, interlace: u8): [33]u8 {
    let b: [33]u8 = [0; 33];
    b[0] = 0x89; b[1] = 'P'; b[2] = 'N'; b[3] = 'G';
    b[4] = 0x0D; b[5] = 0x0A; b[6] = 0x1A; b[7] = 0x0A;
    // IHDR length = 13 (big-endian)
    b[11] = 13;
    b[12] = 'I'; b[13] = 'H'; b[14] = 'D'; b[15] = 'R';
    // width BE
    b[16] = ((w >> 24) & 0xFF) as u8;
    b[17] = ((w >> 16) & 0xFF) as u8;
    b[18] = ((w >> 8) & 0xFF) as u8;
    b[19] = (w & 0xFF) as u8;
    // height BE
    b[20] = ((h >> 24) & 0xFF) as u8;
    b[21] = ((h >> 16) & 0xFF) as u8;
    b[22] = ((h >> 8) & 0xFF) as u8;
    b[23] = (h & 0xFF) as u8;
    b[24] = bit_depth;
    b[25] = color_type;
    b[28] = interlace;
    return b;
}

test("PNG IHDR width, height, bit depth, color type, alpha and interlace probing") {
    const png_rgba = makePngHeader(1920, 1080, 8, 6, 0);
    const info = probe(png_rgba[0..33]) catch unreachable;
    expect_eq(info.format == .png, true);
    expect_eq(info.mimeType(), "image/png");
    expect_eq(info.format.extension(), "png");
    expect_eq(info.width, @as(u32, 1920));
    expect_eq(info.height, @as(u32, 1080));
    expect_eq(info.pixelCount(), @as(u64, 1920 * 1080));
    expect_eq(info.bit_depth, @as(u8, 8));
    expect_eq(info.color_type, @as(u8, 6));
    expect_eq(info.has_alpha, true);
    expect_eq(info.is_progressive, false);

    // 16-bit RGB + Adam7 interlace
    const png_rgb16 = makePngHeader(640, 480, 16, 2, 1);
    const info2 = probePng(png_rgb16[0..33]) catch unreachable;
    expect_eq(info2.bit_depth, @as(u8, 16));
    expect_eq(info2.color_type, @as(u8, 2));
    expect_eq(info2.has_alpha, false);
    expect_eq(info2.is_progressive, true);
}

test("JPEG SOF0 baseline and SOF2 progressive probing past APP0 markers") {
    // SOI + APP0 (length=6, 4 payload bytes) + SOF0 (8-bit, h=720, w=1280)
    let jpg_sof0: [24]u8 = [0; 24];
    jpg_sof0[0] = 0xFF; jpg_sof0[1] = 0xD8; // SOI
    jpg_sof0[2] = 0xFF; jpg_sof0[3] = 0xE0; // APP0
    jpg_sof0[4] = 0x00; jpg_sof0[5] = 0x06; // len=6
    jpg_sof0[10] = 0xFF; jpg_sof0[11] = 0xC0; // SOF0
    jpg_sof0[12] = 0x00; jpg_sof0[13] = 0x0B; // len=11
    jpg_sof0[14] = 8; // precision
    jpg_sof0[15] = 0x02; jpg_sof0[16] = 0xD0; // height = 720
    jpg_sof0[17] = 0x05; jpg_sof0[18] = 0x00; // width = 1280

    const info0 = probe(jpg_sof0[0..24]) catch unreachable;
    expect_eq(info0.format == .jpeg, true);
    expect_eq(info0.mimeType(), "image/jpeg");
    expect_eq(info0.width, @as(u32, 1280));
    expect_eq(info0.height, @as(u32, 720));
    expect_eq(info0.bit_depth, @as(u8, 8));
    expect_eq(info0.is_progressive, false);

    // 切换为 SOF2 (0xC2 Progressive), 宽高 800x600
    jpg_sof0[11] = 0xC2;
    jpg_sof0[15] = 0x02; jpg_sof0[16] = 0x58; // 600
    jpg_sof0[17] = 0x03; jpg_sof0[18] = 0x20; // 800
    const info2 = probeJpeg(jpg_sof0[0..24]) catch unreachable;
    expect_eq(info2.width, @as(u32, 800));
    expect_eq(info2.height, @as(u32, 600));
    expect_eq(info2.is_progressive, true);
}

test("WebP VP8, VP8L and VP8X dimensions, alpha and animation probing") {
    // 1. VP8 (lossy) 320x240
    let vp8: [30]u8 = [0; 30];
    vp8[0] = 'R'; vp8[1] = 'I'; vp8[2] = 'F'; vp8[3] = 'F';
    vp8[8] = 'W'; vp8[9] = 'E'; vp8[10] = 'B'; vp8[11] = 'P';
    vp8[12] = 'V'; vp8[13] = 'P'; vp8[14] = '8'; vp8[15] = ' ';
    vp8[23] = 0x9D; vp8[24] = 0x01; vp8[25] = 0x2A;
    vp8[26] = 0x40; vp8[27] = 0x01; // 320 LE
    vp8[28] = 0xF0; vp8[29] = 0x00; // 240 LE

    const i_vp8 = probe(vp8[0..30]) catch unreachable;
    expect_eq(i_vp8.format == .webp, true);
    expect_eq(i_vp8.webp_kind == .vp8, true);
    expect_eq(i_vp8.width, @as(u32, 320));
    expect_eq(i_vp8.height, @as(u32, 240));
    expect_eq(i_vp8.has_alpha, false);
    expect_eq(i_vp8.is_animated, false);

    // 2. VP8L (lossless) 512x256 with alpha=1
    let vp8l: [25]u8 = [0; 25];
    vp8l[0] = 'R'; vp8l[1] = 'I'; vp8l[2] = 'F'; vp8l[3] = 'F';
    vp8l[8] = 'W'; vp8l[9] = 'E'; vp8l[10] = 'B'; vp8l[11] = 'P';
    vp8l[12] = 'V'; vp8l[13] = 'P'; vp8l[14] = '8'; vp8l[15] = 'L';
    vp8l[20] = 0x2F;
    const bits: u32 = @as(u32, 511) | (@as(u32, 255) << 14) | (@as(u32, 1) << 28);
    vp8l[21] = (bits & 0xFF) as u8;
    vp8l[22] = ((bits >> 8) & 0xFF) as u8;
    vp8l[23] = ((bits >> 16) & 0xFF) as u8;
    vp8l[24] = ((bits >> 24) & 0xFF) as u8;

    const i_vp8l = probeWebp(vp8l[0..25]) catch unreachable;
    expect_eq(i_vp8l.webp_kind == .vp8l, true);
    expect_eq(i_vp8l.width, @as(u32, 512));
    expect_eq(i_vp8l.height, @as(u32, 256));
    expect_eq(i_vp8l.has_alpha, true);

    // 3. VP8X (extended) 1024x768 with alpha(0x10) + animation(0x02)
    let vp8x: [30]u8 = [0; 30];
    vp8x[0] = 'R'; vp8x[1] = 'I'; vp8x[2] = 'F'; vp8x[3] = 'F';
    vp8x[8] = 'W'; vp8x[9] = 'E'; vp8x[10] = 'B'; vp8x[11] = 'P';
    vp8x[12] = 'V'; vp8x[13] = 'P'; vp8x[14] = '8'; vp8x[15] = 'X';
    vp8x[20] = 0x12; // alpha + anim
    const w_m1: u32 = 1023;
    const h_m1: u32 = 767;
    vp8x[24] = (w_m1 & 0xFF) as u8;
    vp8x[25] = ((w_m1 >> 8) & 0xFF) as u8;
    vp8x[26] = ((w_m1 >> 16) & 0xFF) as u8;
    vp8x[27] = (h_m1 & 0xFF) as u8;
    vp8x[28] = ((h_m1 >> 8) & 0xFF) as u8;
    vp8x[29] = ((h_m1 >> 16) & 0xFF) as u8;

    const i_vp8x = probe(vp8x[0..30]) catch unreachable;
    expect_eq(i_vp8x.webp_kind == .vp8x, true);
    expect_eq(i_vp8x.width, @as(u32, 1024));
    expect_eq(i_vp8x.height, @as(u32, 768));
    expect_eq(i_vp8x.has_alpha, true);
    expect_eq(i_vp8x.is_animated, true);
}

test("BMP positive/negative height (24bpp/32bpp) and GIF89a probing") {
    // 1. BMP 24bpp 正高度 (bottom-up): 400x300
    let bmp24: [54]u8 = [0; 54];
    bmp24[0] = 'B'; bmp24[1] = 'M';
    std.mem.writeInt(u32, bmp24[14..18], 40, .little);
    std.mem.writeInt(i32, bmp24[18..22], 400, .little);
    std.mem.writeInt(i32, bmp24[22..26], 300, .little);
    std.mem.writeInt(u16, bmp24[28..30], 24, .little);

    const b1 = probe(bmp24[0..54]) catch unreachable;
    expect_eq(b1.format == .bmp, true);
    expect_eq(b1.width, @as(u32, 400));
    expect_eq(b1.height, @as(u32, 300));
    expect_eq(b1.bit_depth, @as(u8, 24));
    expect_eq(b1.top_down, false);
    expect_eq(b1.has_alpha, false);

    // 2. BMP V4 32bpp 负高度 (top-down) + BI_BITFIELDS 显式 Alpha 掩码
    let bmp32: [122]u8 = [0; 122];
    bmp32[0] = 'B'; bmp32[1] = 'M';
    std.mem.writeInt(u32, bmp32[14..18], 108, .little);
    std.mem.writeInt(i32, bmp32[18..22], 64, .little);
    std.mem.writeInt(i32, bmp32[22..26], -128, .little);
    std.mem.writeInt(u16, bmp32[28..30], 32, .little);
    std.mem.writeInt(u32, bmp32[30..34], 3, .little); // BI_BITFIELDS
    std.mem.writeInt(u32, bmp32[66..70], 0xFF000000, .little); // alpha mask

    const b2 = probeBmp(bmp32[0..122]) catch unreachable;
    expect_eq(b2.width, @as(u32, 64));
    expect_eq(b2.height, @as(u32, 128));
    expect_eq(b2.bit_depth, @as(u8, 32));
    expect_eq(b2.top_down, true);
    expect_eq(b2.has_alpha, true);

    // 3. GIF89a 200x150
    let gif: [13]u8 = [0; 13];
    gif[0] = 'G'; gif[1] = 'I'; gif[2] = 'F'; gif[3] = '8'; gif[4] = '9'; gif[5] = 'a';
    std.mem.writeInt(u16, gif[6..8], 200, .little);
    std.mem.writeInt(u16, gif[8..10], 150, .little);
    gif[10] = 0xF7; // 8-bit color resolution

    const g1 = probe(gif[0..13]) catch unreachable;
    expect_eq(g1.format == .gif, true);
    expect_eq(g1.width, @as(u32, 200));
    expect_eq(g1.height, @as(u32, 150));
    expect_eq(g1.bit_depth, @as(u8, 8));
    expect_eq(g1.has_alpha, true);
}

test("Truncated, malformed, zero-dimension and unknown header error guards") {
    // 1. 未知/空格式
    expect_eq(detectFormat("") == null, true);
    expect_eq(detectFormat("not an image") == null, true);
    const e_unknown = switch (probe("not an image")) {
        err.UnknownFormat => true,
        else => false,
    };
    expect_eq(e_unknown, true);

    // 2. 截断 PNG 头
    const png_ok = makePngHeader(100, 100, 8, 6, 0);
    const e_png_trunc = switch (probe(png_ok[0..20])) {
        err.TruncatedHeader => true,
        else => false,
    };
    expect_eq(e_png_trunc, true);

    // 3. 零宽高 PNG -> InvalidHeader
    const png_zero = makePngHeader(0, 100, 8, 6, 0);
    const e_png_zero = switch (probe(png_zero[0..33])) {
        err.InvalidHeader => true,
        else => false,
    };
    expect_eq(e_png_zero, true);

    // 4. BMP i32::MIN 负高度溢出防护 -> InvalidHeader
    let bmp_min: [54]u8 = [0; 54];
    bmp_min[0] = 'B'; bmp_min[1] = 'M';
    std.mem.writeInt(u32, bmp_min[14..18], 40, .little);
    std.mem.writeInt(i32, bmp_min[18..22], 16, .little);
    std.mem.writeInt(i32, bmp_min[22..26], std.math.minInt(i32), .little);
    std.mem.writeInt(u16, bmp_min[28..30], 24, .little);
    const e_bmp_min = switch (probe(bmp_min[0..54])) {
        err.InvalidHeader => true,
        else => false,
    };
    expect_eq(e_bmp_min, true);

    // 5. 不支持的 WebP FourCC -> UnsupportedVariant
    let bad_webp: [20]u8 = [0; 20];
    bad_webp[0] = 'R'; bad_webp[1] = 'I'; bad_webp[2] = 'F'; bad_webp[3] = 'F';
    bad_webp[8] = 'W'; bad_webp[9] = 'E'; bad_webp[10] = 'B'; bad_webp[11] = 'P';
    bad_webp[12] = 'V'; bad_webp[13] = 'P'; bad_webp[14] = '9'; bad_webp[15] = ' ';
    const e_webp_bad = switch (probe(bad_webp[0..20])) {
        err.UnsupportedVariant => true,
        else => false,
    };
    expect_eq(e_webp_bad, true);
}

test("detectFormat magic dispatch consistency across all 5 image formats") {
    const png_b = makePngHeader(1, 1, 8, 2, 0);
    expect_eq(detectFormat(png_b[0..33]).? == .png, true);

    const jpg_b: [2]u8 = [0xFF, 0xD8];
    expect_eq(detectFormat(jpg_b[0..2]).? == .jpeg, true);

    const bmp_b: [2]u8 = ['B', 'M'];
    expect_eq(detectFormat(bmp_b[0..2]).? == .bmp, true);

    const gif_b = "GIF87a";
    expect_eq(detectFormat(gif_b).? == .gif, true);

    const webp_b = "RIFF0000WEBP";
    expect_eq(detectFormat(webp_b).? == .webp, true);
}
