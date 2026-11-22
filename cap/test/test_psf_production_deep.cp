// Source: cp/cap/test/test_psf_production_deep.cp (Production deep tests for cap:psf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PSF1_MAGIC0,
    PSF1_MAGIC1,
    PSF1_HEADER_SIZE,
    PSF1_MODE512,
    PSF1_MODEHASTAB,
    PSF1_MODEHASSEQ,
    PSF1_SEPARATOR,
    PSF1_STARTSEQ,
    PSF2_MAGIC,
    PSF2_HEADER_SIZE,
    PSF2_HAS_UNICODE_TABLE,
    PSF2_SEPARATOR,
    PSF2_STARTSEQ,
    PsfVersion,
    PsfHeader,
    detectPsfVersion,
    isPsf,
    parsePsfHeader,
    psfGlyphBitmap,
    psfGlyphPixel,
    findPsfGlyphByCodepoint,
} from "cap:gfx";

fn writeU16LE(buf: []u8, offset: usize, val: u16) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

test("1. PSF1 256-glyph 8x16 header, pixel MSB-first decode, and UCS-2 Unicode table with STARTSEQ", () => {
    // 4B header + 256 * 16B glyphs (4096B) + 20B unicode table = 4120B
    let buf: [4120]u8 = std.mem.zeroes([4120]u8);
    buf[0] = PSF1_MAGIC0;
    buf[1] = PSF1_MAGIC1;
    buf[2] = PSF1_MODEHASTAB | PSF1_MODEHASSEQ;
    buf[3] = 16; // 8x16

    // Set glyph #1 row 0 = 0b1000_0001 (0x81), row 1 = 0b0100_0010 (0x42)
    const g1_off = 4 + 16;
    buf[g1_off] = 0x81;
    buf[g1_off + 1] = 0x42;

    // Unicode table at offset 4100:
    // glyph 0: U+0020 + STARTSEQ U+0041 U+0301 + SEPARATOR (U+0041 inside seq must be skipped!)
    // glyph 1: U+0041 ('A') + U+2603 (snowman) + SEPARATOR
    let t = 4100;
    writeU16LE(buf[0..], t, 0x0020); t += 2;
    writeU16LE(buf[0..], t, PSF1_STARTSEQ); t += 2;
    writeU16LE(buf[0..], t, 0x0041); t += 2;
    writeU16LE(buf[0..], t, 0x0301); t += 2;
    writeU16LE(buf[0..], t, PSF1_SEPARATOR); t += 2;

    writeU16LE(buf[0..], t, 0x0041); t += 2;
    writeU16LE(buf[0..], t, 0x2603); t += 2;
    writeU16LE(buf[0..], t, PSF1_SEPARATOR); t += 2;

    expect_eq(isPsf(buf[0..]), true);
    expect_eq(detectPsfVersion(buf[0..]), PsfVersion.psf1);

    const hdr = parsePsfHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.version, PsfVersion.psf1);
    expect_eq(hdr.glyph_count, 256);
    expect_eq(hdr.width, 8);
    expect_eq(hdr.height, 16);
    expect_eq(hdr.bytes_per_glyph, 16);
    expect_eq(hdr.has_unicode_table, true);
    expect_eq(hdr.unicode_table_offset ?? 0, 4100);

    // Pixel check MSB-first
    expect_eq(psfGlyphPixel(buf[0..], hdr, 1, 0, 0) ?? false, true);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 1, 0, 1) ?? true, false);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 1, 0, 7) ?? false, true);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 1, 1, 1) ?? false, true);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 1, 1, 6) ?? false, true);

    // Unicode lookup: U+0041 resolves to glyph 1 (not glyph 0's STARTSEQ!), U+2603 -> glyph 1
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x0020) ?? 999, 0);
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x0041) ?? 999, 1);
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x2603) ?? 999, 1);
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x0301) == null, true);
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x1F600) == null, true);
});

test("2. PSF1 512-glyph mode without Unicode table", () => {
    // 4B header + 512 * 8B = 4100B
    let buf: [4100]u8 = std.mem.zeroes([4100]u8);
    buf[0] = PSF1_MAGIC0;
    buf[1] = PSF1_MAGIC1;
    buf[2] = PSF1_MODE512;
    buf[3] = 8;

    const hdr = parsePsfHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.glyph_count, 512);
    expect_eq(hdr.height, 8);
    expect_eq(hdr.has_unicode_table, false);
    expect_eq(hdr.unicode_table_offset == null, true);
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x41) == null, true);
});

test("3. PSF2 12x16 multi-byte-width glyph pixels and UTF-8 Unicode table (ASCII, Latin, CJK, STARTSEQ)", () => {
    // 12x16 => row_bytes = (12 + 7) / 8 = 2, bytes_per_glyph = 32
    // 32B header + 3 * 32B glyphs (96B) + 32B UTF-8 unicode table = 160B
    let buf: [160]u8 = std.mem.zeroes([160]u8);
    writeU32LE(buf[0..], 0, PSF2_MAGIC);
    writeU32LE(buf[0..], 4, 0); // version = 0
    writeU32LE(buf[0..], 8, 32); // header_size
    writeU32LE(buf[0..], 12, PSF2_HAS_UNICODE_TABLE);
    writeU32LE(buf[0..], 16, 3); // glyph_count = 3
    writeU32LE(buf[0..], 20, 32); // bytes_per_glyph = 32
    writeU32LE(buf[0..], 24, 16); // height = 16
    writeU32LE(buf[0..], 28, 12); // width = 12

    // Glyph 2, row 3: byte 0 = 0x80 (col 0 on), byte 1 = 0x10 (col 11 on: bit 3 of byte 1 => 0x10)
    const g2_row3 = 32 + 2 * 32 + 3 * 2;
    buf[g2_row3] = 0x80;
    buf[g2_row3 + 1] = 0x10;

    // UTF-8 Unicode table at offset 128:
    // glyph 0: 'A' (0x41) + STARTSEQ + 'e' + 0xCC 0x81 + SEPARATOR
    // glyph 1: 'é' (U+00E9 -> 0xC3 0xA9) + SEPARATOR
    // glyph 2: '中' (U+4E2D -> 0xE4 0xB8 0xAD) + SEPARATOR
    let t = 128;
    buf[t] = 0x41; t += 1;
    buf[t] = PSF2_STARTSEQ; t += 1;
    buf[t] = 0x65; t += 1;
    buf[t] = 0xCC; t += 1;
    buf[t] = 0x81; t += 1;
    buf[t] = PSF2_SEPARATOR; t += 1;

    buf[t] = 0xC3; t += 1;
    buf[t] = 0xA9; t += 1;
    buf[t] = PSF2_SEPARATOR; t += 1;

    buf[t] = 0xE4; t += 1;
    buf[t] = 0xB8; t += 1;
    buf[t] = 0xAD; t += 1;
    buf[t] = PSF2_SEPARATOR; t += 1;

    expect_eq(detectPsfVersion(buf[0..]), PsfVersion.psf2);
    const hdr = parsePsfHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.width, 12);
    expect_eq(hdr.height, 16);
    expect_eq(hdr.glyph_count, 3);
    expect_eq(hdr.unicode_table_offset ?? 0, 128);

    // Multi-byte row MSB-first pixel checks
    expect_eq(psfGlyphPixel(buf[0..], hdr, 2, 3, 0) ?? false, true);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 2, 3, 1) ?? true, false);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 2, 3, 11) ?? false, true);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 2, 3, 10) ?? true, false);

    // UTF-8 codepoint lookups
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x0041) ?? 999, 0);
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x0065) == null, true); // inside STARTSEQ
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x00E9) ?? 999, 1);
    expect_eq(findPsfGlyphByCodepoint(buf[0..], hdr, 0x4E2D) ?? 999, 2);
});

test("4. Out-of-bounds glyph index, row, and col return null safely", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    writeU32LE(buf[0..], 0, PSF2_MAGIC);
    writeU32LE(buf[0..], 4, 0);
    writeU32LE(buf[0..], 8, 32);
    writeU32LE(buf[0..], 12, 0);
    writeU32LE(buf[0..], 16, 2);
    writeU32LE(buf[0..], 20, 16);
    writeU32LE(buf[0..], 24, 16);
    writeU32LE(buf[0..], 28, 8);

    const hdr = parsePsfHeader(buf[0..]) ?? unreachable;
    expect_eq(psfGlyphBitmap(buf[0..], hdr, 1) != null, true);
    expect_eq(psfGlyphBitmap(buf[0..], hdr, 2) == null, true);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 0, 16, 0) == null, true);
    expect_eq(psfGlyphPixel(buf[0..], hdr, 0, 0, 8) == null, true);
});

test("5. Malformed PSF headers (bad magic, non-zero PSF2 version, truncated bitmap, undersized bytes_per_glyph) fail safely", () => {
    expect_eq(isPsf(""), false);
    expect_eq(isPsf("NOT A FONT"), false);
    expect_eq(parsePsfHeader("NOT A FONT") == null, true);

    // Build valid PSF2 then corrupt individual fields
    let valid: [64]u8 = std.mem.zeroes([64]u8);
    writeU32LE(valid[0..], 0, PSF2_MAGIC);
    writeU32LE(valid[0..], 4, 0);
    writeU32LE(valid[0..], 8, 32);
    writeU32LE(valid[0..], 12, 0);
    writeU32LE(valid[0..], 16, 2);
    writeU32LE(valid[0..], 20, 16);
    writeU32LE(valid[0..], 24, 16);
    writeU32LE(valid[0..], 28, 8);

    // Truncated bitmap region (63B < 32 + 32)
    expect_eq(parsePsfHeader(valid[0..63]) == null, true);

    // Non-zero PSF2 version
    let bad_ver: [64]u8 = valid;
    writeU32LE(bad_ver[0..], 4, 1);
    expect_eq(parsePsfHeader(bad_ver[0..]) == null, true);

    // Undersized bytes_per_glyph (15 < 16)
    let bad_bpg: [64]u8 = valid;
    writeU32LE(bad_bpg[0..], 20, 15);
    expect_eq(parsePsfHeader(bad_bpg[0..]) == null, true);

    // Truncated PSF1
    let bad_psf1: [20]u8 = std.mem.zeroes([20]u8);
    bad_psf1[0] = PSF1_MAGIC0;
    bad_psf1[1] = PSF1_MAGIC1;
    bad_psf1[2] = 0;
    bad_psf1[3] = 16; // needs 4 + 256*16 = 4100B
    expect_eq(parsePsfHeader(bad_psf1[0..]) == null, true);
});
