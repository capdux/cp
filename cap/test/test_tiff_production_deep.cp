// Source: cp/cap/test/test_tiff_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:tiff & cap.tiff

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TiffError,
    TiffEndian,
    TiffCompression,
    TiffPixelFormat,
    TiffPage,
    TiffDocument,
    encodeSingle,
    encodeMultiPage,
    decode,
    decodeWithLimits,
} from "cap:tiff";

fn buildPatternPixels(alloc: std.mem.Allocator, width: u32, height: u32, spp: usize): []u8 {
    const total = (width as usize) * (height as usize) * spp;
    let buf = alloc.alloc(u8, total) catch unreachable;
    let y: usize = 0;
    while (y < (height as usize)) : (y += 1) {
        let x: usize = 0;
        while (x < (width as usize)) : (x += 1) {
            let c: usize = 0;
            while (c < spp) : (c += 1) {
                const idx = (y * (width as usize) + x) * spp + c;
                buf[idx] = (((x * 17 + y * 31 + c * 67) / 2) & 0xff) as u8;
            }
        }
    }
    return buf;
}

fn verifySingleRoundtrip(
    alloc: std.mem.Allocator,
    w: u32,
    h: u32,
    fmt: TiffPixelFormat,
    spp: usize,
    comp: TiffCompression,
    endian: TiffEndian,
): !void {
    const pixels = buildPatternPixels(alloc, w, h, spp);
    defer alloc.free(pixels);

    const encoded = encodeSingle(alloc, w, h, fmt, comp, endian, pixels)?;
    defer alloc.free(encoded);

    if (endian == TiffEndian.Little) {
        expect_eq(encoded[0], 'I');
        expect_eq(encoded[1], 'I');
    } else {
        expect_eq(encoded[0], 'M');
        expect_eq(encoded[1], 'M');
    }

    let doc = decode(alloc, encoded)?;
    defer doc.deinit(alloc);

    expect_eq(doc.pages.len, 1);
    expect_eq(doc.pages[0].width, w);
    expect_eq(doc.pages[0].height, h);
    expect_eq(doc.pages[0].format == fmt, true);
    expect_eq(doc.pages[0].compression == comp, true);
    expect_eq(std.mem.eql(u8, doc.pages[0].pixels, pixels), true);
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    const endians: [2]TiffEndian = [TiffEndian.Little, TiffEndian.Big];
    const comps: [4]TiffCompression = [
        TiffCompression.None,
        TiffCompression.PackBits,
        TiffCompression.Lzw,
        TiffCompression.Deflate,
    ];

    let ei: usize = 0;
    while (ei < endians.len) : (ei += 1) {
        let ci: usize = 0;
        while (ci < comps.len) : (ci += 1) {
            verifySingleRoundtrip(alloc, 17, 13, TiffPixelFormat.Gray8, 1, comps[ci], endians[ei])?;
            verifySingleRoundtrip(alloc, 11, 9, TiffPixelFormat.Rgb24, 3, comps[ci], endians[ei])?;
            verifySingleRoundtrip(alloc, 8, 7, TiffPixelFormat.Rgba32, 4, comps[ci], endians[ei])?;
        }
    }
}

fn pushU16LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u16) {
    out.append(alloc, (v & 0xff) as u8) catch unreachable;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch unreachable;
}

fn pushU32LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32) {
    out.append(alloc, (v & 0xff) as u8) catch unreachable;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch unreachable;
    out.append(alloc, ((v >> 16) & 0xff) as u8) catch unreachable;
    out.append(alloc, ((v >> 24) & 0xff) as u8) catch unreachable;
}

fn pushIfdEntryLE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), tag: u16, ftype: u16, count: u32, val: u32) {
    pushU16LE(alloc, out, tag);
    pushU16LE(alloc, out, ftype);
    pushU32LE(alloc, out, count);
    pushU32LE(alloc, out, val);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 1. Multi-page encode & decode roundtrip
    const p0_pix = buildPatternPixels(alloc, 10, 6, 1);
    defer alloc.free(p0_pix);
    const p1_pix = buildPatternPixels(alloc, 8, 5, 3);
    defer alloc.free(p1_pix);
    const p2_pix = buildPatternPixels(alloc, 4, 4, 4);
    defer alloc.free(p2_pix);

    const in_pages: [3]TiffPage = [
        TiffPage{ width: 10, height: 6, format: TiffPixelFormat.Gray8, compression: TiffCompression.PackBits, pixels: p0_pix },
        TiffPage{ width: 8, height: 5, format: TiffPixelFormat.Rgb24, compression: TiffCompression.Lzw, pixels: p1_pix },
        TiffPage{ width: 4, height: 4, format: TiffPixelFormat.Rgba32, compression: TiffCompression.Deflate, pixels: p2_pix },
    ];

    const multi_bytes = encodeMultiPage(alloc, in_pages[0..3], TiffEndian.Big)?;
    defer alloc.free(multi_bytes);

    let multi_doc = decode(alloc, multi_bytes)?;
    defer multi_doc.deinit(alloc);

    expect_eq(multi_doc.pages.len, 3);
    expect_eq(std.mem.eql(u8, multi_doc.pages[0].pixels, p0_pix), true);
    expect_eq(std.mem.eql(u8, multi_doc.pages[1].pixels, p1_pix), true);
    expect_eq(std.mem.eql(u8, multi_doc.pages[2].pixels, p2_pix), true);

    // 2. Multi-strip synthetic TIFF (2 strips of 4x2 Gray8, SHORT StripOffsets & StripByteCounts)
    let raw: std.ArrayList(u8) = .empty;
    defer raw.deinit(alloc);

    raw.append(alloc, 'I') catch unreachable;
    raw.append(alloc, 'I') catch unreachable;
    pushU16LE(alloc, &raw, 42);
    pushU32LE(alloc, &raw, 8); // IFD at offset 8

    // 9 IFD entries -> 2 + 9*12 + 4 = 114 bytes -> ends at 122
    pushU16LE(alloc, &raw, 9);
    // Strip 0 at 122..130, Strip 1 at 130..138 (inline SHORT pairs!)
    const strip0_inline: u32 = (122 as u32) | ((130 as u32) << 16);
    const cnt_inline: u32 = (8 as u32) | ((8 as u32) << 16);

    pushIfdEntryLE(alloc, &raw, 256, 4, 1, 4); // ImageWidth = 4
    pushIfdEntryLE(alloc, &raw, 257, 4, 1, 4); // ImageLength = 4
    pushIfdEntryLE(alloc, &raw, 258, 3, 1, 8); // BitsPerSample = 8
    pushIfdEntryLE(alloc, &raw, 259, 3, 1, 1); // Compression = None
    pushIfdEntryLE(alloc, &raw, 262, 3, 1, 1); // Photometric = BlackIsZero
    pushIfdEntryLE(alloc, &raw, 273, 3, 2, strip0_inline); // StripOffsets (SHORT[2])
    pushIfdEntryLE(alloc, &raw, 277, 3, 1, 1); // SamplesPerPixel = 1
    pushIfdEntryLE(alloc, &raw, 278, 4, 1, 2); // RowsPerStrip = 2
    pushIfdEntryLE(alloc, &raw, 279, 3, 2, cnt_inline); // StripByteCounts (SHORT[2])
    pushU32LE(alloc, &raw, 0); // NextIFD = 0

    const expected_multi_strip: [16]u8 = [10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33, 40, 41, 42, 43];
    raw.appendSlice(alloc, expected_multi_strip[0..16]) catch unreachable;

    let ms_doc = decode(alloc, raw.items)?;
    defer ms_doc.deinit(alloc);
    expect_eq(ms_doc.pages.len, 1);
    expect_eq(std.mem.eql(u8, ms_doc.pages[0].pixels, expected_multi_strip[0..16]), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // Predictor = 2 (Horizontal Differencing) on 3x2 Rgb24
    // Original pixels:
    // Row 0: (10, 20, 30), (15, 25, 35), (255, 5, 10)
    // Row 1: (100, 100, 100), (100, 100, 100), (90, 110, 120)
    // Differenced (mod 256):
    // Row 0: (10, 20, 30), (5, 5, 5), (240, 236, 231)
    // Row 1: (100, 100, 100), (0, 0, 0), (246, 10, 20)
    const orig: [18]u8 = [
        10, 20, 30, 15, 25, 35, 255, 5, 10,
        100, 100, 100, 100, 100, 100, 90, 110, 120,
    ];
    const diffed: [18]u8 = [
        10, 20, 30, 5, 5, 5, 240, 236, 231,
        100, 100, 100, 0, 0, 0, 246, 10, 20,
    ];

    let raw: std.ArrayList(u8) = .empty;
    defer raw.deinit(alloc);

    raw.append(alloc, 'I') catch unreachable;
    raw.append(alloc, 'I') catch unreachable;
    pushU16LE(alloc, &raw, 42);
    pushU32LE(alloc, &raw, 8);

    // 10 entries -> 2 + 10*12 + 4 = 126 bytes -> ends at 134
    // BitsPerSample[3] at 134..140, Strip at 140..158
    pushU16LE(alloc, &raw, 10);
    pushIfdEntryLE(alloc, &raw, 256, 4, 1, 3); // Width = 3
    pushIfdEntryLE(alloc, &raw, 257, 4, 1, 2); // Height = 2
    pushIfdEntryLE(alloc, &raw, 258, 3, 3, 134); // BitsPerSample offset
    pushIfdEntryLE(alloc, &raw, 259, 3, 1, 1); // Compression = None
    pushIfdEntryLE(alloc, &raw, 262, 3, 1, 2); // Photometric = RGB
    pushIfdEntryLE(alloc, &raw, 273, 4, 1, 140); // StripOffsets = 140
    pushIfdEntryLE(alloc, &raw, 277, 3, 1, 3); // SamplesPerPixel = 3
    pushIfdEntryLE(alloc, &raw, 278, 4, 1, 2); // RowsPerStrip = 2
    pushIfdEntryLE(alloc, &raw, 279, 4, 1, 18); // StripByteCounts = 18
    pushIfdEntryLE(alloc, &raw, 317, 3, 1, 2); // Predictor = 2
    pushU32LE(alloc, &raw, 0);

    pushU16LE(alloc, &raw, 8);
    pushU16LE(alloc, &raw, 8);
    pushU16LE(alloc, &raw, 8);
    raw.appendSlice(alloc, diffed[0..18]) catch unreachable;

    let doc = decode(alloc, raw.items)?;
    defer doc.deinit(alloc);
    expect_eq(doc.pages.len, 1);
    expect_eq(std.mem.eql(u8, doc.pages[0].pixels, orig[0..18]), true);
}

fn expectDecodeError(alloc: std.mem.Allocator, data: []const u8, expected: TiffError) {
    let doc = decode(alloc, data) catch |e| {
        expect_eq(e == expected, true);
        return;
    };
    doc.deinit(alloc);
    expect_eq(false, true);
}

fn expectDecodeLimitsError(
    alloc: std.mem.Allocator,
    data: []const u8,
    max_w: u32,
    max_h: u32,
    max_pages: usize,
    expected: TiffError,
) {
    let doc = decodeWithLimits(alloc, data, max_w, max_h, max_pages) catch |e| {
        expect_eq(e == expected, true);
        return;
    };
    doc.deinit(alloc);
    expect_eq(false, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. Invalid "XX" signature
    const bad_sig: [8]u8 = ['X', 'X', 42, 0, 8, 0, 0, 0];
    expectDecodeError(alloc, bad_sig[0..8], TiffError.InvalidSignature);

    // 2. Truncated header
    const short_hdr: [4]u8 = ['I', 'I', 42, 0];
    expectDecodeError(alloc, short_hdr[0..4], TiffError.TruncatedData);

    // 3. Out-of-bounds IFD offset
    const oob_ifd: [8]u8 = ['I', 'I', 42, 0, 0, 1, 0, 0];
    expectDecodeError(alloc, oob_ifd[0..8], TiffError.InvalidIfd);

    // 4. Cyclic IFD linked list (NextIFD points back to offset 8)
    let cyc: std.ArrayList(u8) = .empty;
    defer cyc.deinit(alloc);
    cyc.append(alloc, 'I') catch unreachable;
    cyc.append(alloc, 'I') catch unreachable;
    pushU16LE(alloc, &cyc, 42);
    pushU32LE(alloc, &cyc, 8);
    pushU16LE(alloc, &cyc, 9);
    const strip_off: u32 = 8 + 2 + 9 * 12 + 4;
    pushIfdEntryLE(alloc, &cyc, 256, 4, 1, 1);
    pushIfdEntryLE(alloc, &cyc, 257, 4, 1, 1);
    pushIfdEntryLE(alloc, &cyc, 258, 3, 1, 8);
    pushIfdEntryLE(alloc, &cyc, 259, 3, 1, 1);
    pushIfdEntryLE(alloc, &cyc, 262, 3, 1, 1);
    pushIfdEntryLE(alloc, &cyc, 273, 4, 1, strip_off);
    pushIfdEntryLE(alloc, &cyc, 277, 3, 1, 1);
    pushIfdEntryLE(alloc, &cyc, 278, 4, 1, 1);
    pushIfdEntryLE(alloc, &cyc, 279, 4, 1, 1);
    pushU32LE(alloc, &cyc, 8); // Cycle back to IFD0!
    cyc.append(alloc, 42) catch unreachable;
    expectDecodeError(alloc, cyc.items, TiffError.InvalidIfd);

    // 5. Unsupported compression (e.g. Compression = 7)
    let unsup_comp: std.ArrayList(u8) = .empty;
    defer unsup_comp.deinit(alloc);
    unsup_comp.append(alloc, 'I') catch unreachable;
    unsup_comp.append(alloc, 'I') catch unreachable;
    pushU16LE(alloc, &unsup_comp, 42);
    pushU32LE(alloc, &unsup_comp, 8);
    pushU16LE(alloc, &unsup_comp, 9);
    pushIfdEntryLE(alloc, &unsup_comp, 256, 4, 1, 1);
    pushIfdEntryLE(alloc, &unsup_comp, 257, 4, 1, 1);
    pushIfdEntryLE(alloc, &unsup_comp, 258, 3, 1, 8);
    pushIfdEntryLE(alloc, &unsup_comp, 259, 3, 1, 7); // Unsupported JPEG
    pushIfdEntryLE(alloc, &unsup_comp, 262, 3, 1, 1);
    pushIfdEntryLE(alloc, &unsup_comp, 273, 4, 1, strip_off);
    pushIfdEntryLE(alloc, &unsup_comp, 277, 3, 1, 1);
    pushIfdEntryLE(alloc, &unsup_comp, 278, 4, 1, 1);
    pushIfdEntryLE(alloc, &unsup_comp, 279, 4, 1, 1);
    pushU32LE(alloc, &unsup_comp, 0);
    unsup_comp.append(alloc, 0) catch unreachable;
    expectDecodeError(alloc, unsup_comp.items, TiffError.UnsupportedCompression);

    // 6. ImageTooLarge & TooManyPages via decodeWithLimits
    const pix4x4 = buildPatternPixels(alloc, 4, 4, 1);
    defer alloc.free(pix4x4);
    const two_pages: [2]TiffPage = [
        TiffPage{ width: 4, height: 4, format: TiffPixelFormat.Gray8, compression: TiffCompression.None, pixels: pix4x4 },
        TiffPage{ width: 4, height: 4, format: TiffPixelFormat.Gray8, compression: TiffCompression.None, pixels: pix4x4 },
    ];
    const encoded_two = encodeMultiPage(alloc, two_pages[0..2], TiffEndian.Little)?;
    defer alloc.free(encoded_two);

    expectDecodeLimitsError(alloc, encoded_two, 2, 2, 10, TiffError.ImageTooLarge);
    expectDecodeLimitsError(alloc, encoded_two, 100, 100, 1, TiffError.TooManyPages);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;
    const raw_rgb: [12]u8 = [255, 0, 0, 0, 255, 0, 0, 0, 255, 128, 128, 128];

    const tiff_bytes = cap.tiff.encodeSingle(
        alloc,
        2,
        2,
        cap.tiff.TiffPixelFormat.Rgb24,
        cap.tiff.TiffCompression.Deflate,
        cap.tiff.TiffEndian.Little,
        raw_rgb[0..12],
    )?;
    defer alloc.free(tiff_bytes);

    let doc = cap.tiff.decode(alloc, tiff_bytes)?;
    defer doc.deinit(alloc);

    expect_eq(doc.pages.len, 1);
    expect_eq(doc.pages[0].width, 2);
    expect_eq(doc.pages[0].height, 2);
    expect_eq(std.mem.eql(u8, doc.pages[0].pixels, raw_rgb[0..12]), true);
}

test("cap:tiff: II/MM Endian x Gray8/Rgb24/Rgba32 x None/PackBits/Lzw/Deflate Roundtrip") {
    runTest1() catch unreachable;
}

test("cap:tiff: Multi-Page TIFF & Multi-Strip SHORT Array Decoding") {
    runTest2() catch unreachable;
}

test("cap:tiff: Predictor=2 Horizontal Differencing Restoration") {
    runTest3() catch unreachable;
}

test("cap:tiff: Invalid Signature, Cyclic/OOB IFD, UnsupportedCompression, Limits & TruncatedData") {
    runTest4() catch unreachable;
}

test("cap:tiff: Dynamic cap.tiff Namespace & std.testing.allocator Zero Leaks") {
    runTest5() catch unreachable;
}
