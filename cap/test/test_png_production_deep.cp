// Source: cp/cap/test/test_png_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:png & cap.png

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { compressZlib, crc32 } from "cap:deflate";
import {
    PngError,
    ColorType,
    PngImage,
    encode,
    decode,
    decodeWithMaxDimensions,
} from "cap:png";

fn writeU32BE(buf: []u8, pos: usize, v: u32) {
    buf[pos] = ((v >> 24) & 0xff) as u8;
    buf[pos + 1] = ((v >> 16) & 0xff) as u8;
    buf[pos + 2] = ((v >> 8) & 0xff) as u8;
    buf[pos + 3] = (v & 0xff) as u8;
}

fn appendTestChunk(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    chunk_type: [4]u8,
    payload: []const u8,
): !void {
    let len_be: [4]u8 = [0, 0, 0, 0];
    writeU32BE(&len_be, 0, payload.len as u32);
    out.appendSlice(alloc, &len_be)?;
    const crc_start = out.items.len;
    out.appendSlice(alloc, chunk_type[0..4])?;
    if (payload.len > 0) {
        out.appendSlice(alloc, payload)?;
    }
    const c = crc32(out.items[crc_start..out.items.len]);
    let crc_be: [4]u8 = [0, 0, 0, 0];
    writeU32BE(&crc_be, 0, c);
    out.appendSlice(alloc, &crc_be)?;
}

fn buildIhdrPayload(width: u32, height: u32, color_type: u8): [13]u8 {
    let p: [13]u8 = [0; 13];
    writeU32BE(&p, 0, width);
    writeU32BE(&p, 4, height);
    p[8] = 8;
    p[9] = color_type;
    p[10] = 0;
    p[11] = 0;
    p[12] = 0;
    return p;
}

fn paethRef(a: u8, b: u8, c: u8): u8 {
    const ia = a as i32;
    const ib = b as i32;
    const ic = c as i32;
    const p = ia + ib - ic;
    const pa = if (p >= ia) (p - ia) else (ia - p);
    const pb = if (p >= ib) (p - ib) else (ib - p);
    const pc = if (p >= ic) (p - ic) else (ic - p);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

fn testLosslessRoundtripAllColorTypes(): !void {
    const alloc = std.testing.allocator;

    // 1. RGBA 8x8 gradient with alpha channel
    const w: u32 = 8;
    const h: u32 = 8;
    let rgba: [256]u8 = [0; 256];
    let y: usize = 0;
    while (y < 8) : (y += 1) {
        let x: usize = 0;
        while (x < 8) : (x += 1) {
            const idx = (y * 8 + x) * 4;
            rgba[idx] = (x * 32) as u8;
            rgba[idx + 1] = (y * 32) as u8;
            rgba[idx + 2] = ((x + y) * 16) as u8;
            rgba[idx + 3] = (255 - y * 28) as u8;
        }
    }

    const png_rgba = encode(alloc, w, h, ColorType.Rgba, &rgba)?;
    defer alloc.free(png_rgba);

    // Verify 8-byte PNG signature
    const expected_sig: [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
    expect_eq(std.mem.eql(u8, png_rgba[0..8], &expected_sig), true);

    let dec_rgba = decode(alloc, png_rgba)?;
    defer dec_rgba.deinit(alloc);
    expect_eq(dec_rgba.width, w);
    expect_eq(dec_rgba.height, h);
    expect_eq(dec_rgba.bit_depth, 8 as u8);
    expect_eq(dec_rgba.color_type == ColorType.Rgba, true);
    expect_eq(dec_rgba.bytesPerPixel(), 4 as usize);
    expect_eq(std.mem.eql(u8, dec_rgba.pixels, &rgba), true);

    // 2. RGB 4x3 lossless roundtrip
    let rgb: [36]u8 = [0; 36];
    let i: usize = 0;
    while (i < 36) : (i += 1) {
        rgb[i] = ((i * 19 + 7) & 0xff) as u8;
    }
    const png_rgb = encode(alloc, 4, 3, ColorType.Rgb, &rgb)?;
    defer alloc.free(png_rgb);
    let dec_rgb = decode(alloc, png_rgb)?;
    defer dec_rgb.deinit(alloc);
    expect_eq(dec_rgb.color_type == ColorType.Rgb, true);
    expect_eq(dec_rgb.bytesPerPixel(), 3 as usize);
    expect_eq(std.mem.eql(u8, dec_rgb.pixels, &rgb), true);

    // 3. Grayscale 5x4 lossless roundtrip
    let gray: [20]u8 = [0; 20];
    i = 0;
    while (i < 20) : (i += 1) {
        gray[i] = (i * 12) as u8;
    }
    const png_gray = encode(alloc, 5, 4, ColorType.Grayscale, &gray)?;
    defer alloc.free(png_gray);
    let dec_gray = decode(alloc, png_gray)?;
    defer dec_gray.deinit(alloc);
    expect_eq(dec_gray.color_type == ColorType.Grayscale, true);
    expect_eq(dec_gray.bytesPerPixel(), 1 as usize);
    expect_eq(std.mem.eql(u8, dec_gray.pixels, &gray), true);
}

fn testAllFiveScanlineFilters(): !void {
    const alloc = std.testing.allocator;

    // 4x5 RGB image (stride = 12, bpp = 3), one row per filter type: 0=None, 1=Sub, 2=Up, 3=Avg, 4=Paeth
    const w: usize = 4;
    const h: usize = 5;
    const bpp: usize = 3;
    const stride: usize = w * bpp;
    let pixels: [60]u8 = [0; 60];
    let idx: usize = 0;
    while (idx < 60) : (idx += 1) {
        pixels[idx] = ((idx * 37 + 13) & 0xff) as u8;
    }

    // Build raw scanlines with explicit filter types 0, 1, 2, 3, 4 on rows 0..4
    let raw: [65]u8 = [0; 65];
    let y: usize = 0;
    while (y < h) : (y += 1) {
        const f: u8 = y as u8;
        const dst_off = y * (1 + stride);
        const src_off = y * stride;
        raw[dst_off] = f;

        let x: usize = 0;
        while (x < stride) : (x += 1) {
            const cur = pixels[src_off + x];
            const a: u8 = if (x >= bpp) pixels[src_off + x - bpp] else 0;
            const b: u8 = if (y > 0) pixels[src_off - stride + x] else 0;
            const c: u8 = if (y > 0 && x >= bpp) pixels[src_off - stride + x - bpp] else 0;
            const fb: u8 = switch (f) {
                0 => cur,
                1 => cur -% a,
                2 => cur -% b,
                3 => cur -% ((((a as u16) + (b as u16)) >> 1) as u8),
                4 => cur -% paethRef(a, b, c),
                else => unreachable,
            };
            raw[dst_off + 1 + x] = fb;
        }
    }

    const z = compressZlib(alloc, &raw)?;
    defer alloc.free(z);

    let png_buf: std.ArrayList(u8) = .empty;
    defer png_buf.deinit(alloc);
    const sig: [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
    png_buf.appendSlice(alloc, &sig)?;
    const ihdr = buildIhdrPayload(4, 5, 2);
    appendTestChunk(alloc, &png_buf, ['I', 'H', 'D', 'R'], &ihdr)?;
    appendTestChunk(alloc, &png_buf, ['I', 'D', 'A', 'T'], z)?;
    const empty: [0]u8 = [];
    appendTestChunk(alloc, &png_buf, ['I', 'E', 'N', 'D'], &empty)?;

    let decoded = decode(alloc, png_buf.items)?;
    defer decoded.deinit(alloc);
    expect_eq(decoded.width, 4 as u32);
    expect_eq(decoded.height, 5 as u32);
    expect_eq(std.mem.eql(u8, decoded.pixels, &pixels), true);
}

fn testMultiIdatAndPltePalette(): !void {
    const alloc = std.testing.allocator;

    // 4x2 Indexed PNG with 4-color PLTE, split across 3 IDAT chunks
    // Raw scanlines: 2 rows * (1 filter + 4 indices) = 10 bytes
    const raw: [10]u8 = [
        0, 0, 1, 2, 3,
        0, 3, 2, 1, 0,
    ];
    const z = compressZlib(alloc, &raw)?;
    defer alloc.free(z);

    // Ensure z is split into 3 non-empty IDAT chunks
    expect_eq(z.len >= 6, true);
    const s1 = z.len / 3;
    const s2 = (z.len * 2) / 3;

    const plte: [12]u8 = [
        255, 0, 0,     // 0: Red
        0, 255, 0,     // 1: Green
        0, 0, 255,     // 2: Blue
        255, 255, 0,   // 3: Yellow
    ];

    let png_buf: std.ArrayList(u8) = .empty;
    defer png_buf.deinit(alloc);
    const sig: [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
    png_buf.appendSlice(alloc, &sig)?;
    const ihdr = buildIhdrPayload(4, 2, 3);
    appendTestChunk(alloc, &png_buf, ['I', 'H', 'D', 'R'], &ihdr)?;
    appendTestChunk(alloc, &png_buf, ['P', 'L', 'T', 'E'], &plte)?;
    appendTestChunk(alloc, &png_buf, ['I', 'D', 'A', 'T'], z[0..s1])?;
    appendTestChunk(alloc, &png_buf, ['I', 'D', 'A', 'T'], z[s1..s2])?;
    appendTestChunk(alloc, &png_buf, ['I', 'D', 'A', 'T'], z[s2..z.len])?;
    const empty: [0]u8 = [];
    appendTestChunk(alloc, &png_buf, ['I', 'E', 'N', 'D'], &empty)?;

    let decoded = decode(alloc, png_buf.items)?;
    defer decoded.deinit(alloc);
    expect_eq(decoded.width, 4 as u32);
    expect_eq(decoded.height, 2 as u32);
    expect_eq(decoded.color_type == ColorType.Rgb, true);
    expect_eq(decoded.pixels.len, 24 as usize);

    const expected_rgb: [24]u8 = [
        255, 0, 0,   0, 255, 0,   0, 0, 255,   255, 255, 0,
        255, 255, 0, 0, 0, 255,   0, 255, 0,   255, 0, 0,
    ];
    expect_eq(std.mem.eql(u8, decoded.pixels, &expected_rgb), true);
}

fn testErrorHandlingAndDimensionGuard(): !void {
    const alloc = std.testing.allocator;

    const px: [16]u8 = [100; 16];
    const valid_png = encode(alloc, 2, 2, ColorType.Rgba, &px)?;
    defer alloc.free(valid_png);

    // 1. Invalid PNG signature
    let bad_sig = alloc.alloc(u8, valid_png.len) catch unreachable;
    defer alloc.free(bad_sig);
    @memcpy(bad_sig, valid_png);
    bad_sig[0] = 0x00;
    let got_bad_sig = false;
    * = decode(alloc, bad_sig) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidSignature")) got_bad_sig = true;
    };
    expect_eq(got_bad_sig, true);

    // 2. Chunk CRC-32 tampering -> ChecksumMismatch
    let bad_crc = alloc.alloc(u8, valid_png.len) catch unreachable;
    defer alloc.free(bad_crc);
    @memcpy(bad_crc, valid_png);
    bad_crc[bad_crc.len - 1] ^= 0x01; // Flip 1 bit in IEND CRC
    let got_crc_err = false;
    * = decode(alloc, bad_crc) catch e => {
        if (std.mem.eql(u8, @errorName(e), "ChecksumMismatch")) got_crc_err = true;
    };
    expect_eq(got_crc_err, true);

    // 3. Invalid filter type (filter byte = 5) -> InvalidFilterType
    const bad_filter_raw: [5]u8 = [5, 10, 20, 30, 40];
    const bad_filter_z = compressZlib(alloc, &bad_filter_raw)?;
    defer alloc.free(bad_filter_z);

    let bad_f_png: std.ArrayList(u8) = .empty;
    defer bad_f_png.deinit(alloc);
    const sig: [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
    bad_f_png.appendSlice(alloc, &sig)?;
    const ihdr_1x1 = buildIhdrPayload(1, 1, 6);
    appendTestChunk(alloc, &bad_f_png, ['I', 'H', 'D', 'R'], &ihdr_1x1)?;
    appendTestChunk(alloc, &bad_f_png, ['I', 'D', 'A', 'T'], bad_filter_z)?;
    const empty: [0]u8 = [];
    appendTestChunk(alloc, &bad_f_png, ['I', 'E', 'N', 'D'], &empty)?;

    let got_bad_filter = false;
    * = decode(alloc, bad_f_png.items) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidFilterType")) got_bad_filter = true;
    };
    expect_eq(got_bad_filter, true);

    // 4. Missing IEND chunk -> MissingIend (slice off the 12-byte IEND chunk)
    let got_missing_iend = false;
    * = decode(alloc, valid_png[0..(valid_png.len - 12)]) catch e => {
        if (std.mem.eql(u8, @errorName(e), "MissingIend")) got_missing_iend = true;
    };
    expect_eq(got_missing_iend, true);

    // 5. Dimension bomb guard -> ImageTooLarge
    let got_too_large = false;
    * = decodeWithMaxDimensions(alloc, valid_png, 1, 1) catch e => {
        if (std.mem.eql(u8, @errorName(e), "ImageTooLarge")) got_too_large = true;
    };
    expect_eq(got_too_large, true);
}

fn testCapNamespaceDynamicMountingAndZeroLeak(): !void {
    const alloc = std.testing.allocator;

    const pixels: [12]u8 = [
        255, 0, 0,   0, 255, 0,
        0, 0, 255,   128, 128, 128,
    ];
    const encoded = cap.png.encode(alloc, 2, 2, cap.png.ColorType.Rgb, &pixels)?;
    defer alloc.free(encoded);

    let img = cap.png.decodeWithMaxDimensions(alloc, encoded, 16, 16)?;
    defer img.deinit(alloc);

    expect_eq(img.width, 2 as u32);
    expect_eq(img.height, 2 as u32);
    expect_eq(img.bytesPerPixel(), 3 as usize);
    expect_eq(std.mem.eql(u8, img.pixels, &pixels), true);
}

test("1. RGBA/RGB/Grayscale Lossless Encode-Decode Roundtrip", () => {
    testLosslessRoundtripAllColorTypes() catch unreachable;
});

test("2. All 5 Scanline Filter Types (None/Sub/Up/Average/Paeth) Decode", () => {
    testAllFiveScanlineFilters() catch unreachable;
});

test("3. Multi-IDAT Chunk Concatenation & PLTE Palette Expansion", () => {
    testMultiIdatAndPltePalette() catch unreachable;
});

test("4. Invalid Signature, CRC Tamper, Invalid Filter, Missing IEND & Dimension Guard", () => {
    testErrorHandlingAndDimensionGuard() catch unreachable;
});

test("5. cap.png Dynamic Mounting & std.testing.allocator Zero-Leak", () => {
    testCapNamespaceDynamicMountingAndZeroLeak() catch unreachable;
});
