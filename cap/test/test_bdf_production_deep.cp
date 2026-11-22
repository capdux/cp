// Source: cp/cap/test/test_bdf_production_deep.cp
// Deep Production Test Suite for cap:bdf
// Covers:
//   1. BDF 2.1 global header + STARTPROPERTIES (FONT_ASCENT, FONT_DESCENT, FAMILY_NAME with "" quotes)
//   2. BdfGlyphIter multi-glyph parsing (incl. -1 unencoded glyph) & MSB-first bdfGlyphPixel verification
//   3. findBdfGlyph lookup by Unicode/ASCII codepoint
//   4. CRLF line endings & 12px-wide multi-byte glyph rows (4 hex chars per row)
//   5. Non-BDF input, missing ENDCHAR truncation, out-of-bounds pixel queries & malformed hex guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BdfBBox,
    BdfHeader,
    BdfProperty,
    BdfGlyph,
    BdfPropIter,
    BdfGlyphIter,
    isBdf,
    parseBdfHeader,
    bdfGlyphPixel,
    findBdfGlyph,
} from "cap:gfx";

const SAMPLE_BDF: []const u8 =
    "STARTFONT 2.1\n" ++
    "COMMENT Generated for CP BDF unit test\n" ++
    "FONT -Misc-Fixed-Medium-R-Normal--10-100-75-75-C-60-ISO10646-1\n" ++
    "SIZE 10 75 75\n" ++
    "FONTBOUNDINGBOX 6 10 0 -2\n" ++
    "STARTPROPERTIES 4\n" ++
    "FAMILY_NAME \"Fixed \"\"Mono\"\"\"\n" ++
    "FOUNDRY \"Misc\"\n" ++
    "FONT_ASCENT 8\n" ++
    "FONT_DESCENT 2\n" ++
    "ENDPROPERTIES\n" ++
    "CHARS 2\n" ++
    "STARTCHAR A\n" ++
    "ENCODING 65\n" ++
    "SWIDTH 600 0\n" ++
    "DWIDTH 6 0\n" ++
    "BBX 5 4 0 0\n" ++
    "BITMAP\n" ++
    "70\n" ++
    "88\n" ++
    "F8\n" ++
    "88\n" ++
    "ENDCHAR\n" ++
    "STARTCHAR notdef\n" ++
    "ENCODING -1\n" ++
    "DWIDTH 6 0\n" ++
    "BBX 4 2 0 0\n" ++
    "BITMAP\n" ++
    "F0\n" ++
    "90\n" ++
    "ENDCHAR\n" ++
    "ENDFONT\n";

fn testBdfHeaderAndProperties() {
    expect_eq(isBdf(SAMPLE_BDF), true);

    const hdr = parseBdfHeader(SAMPLE_BDF).?;
    expect_eq(std.mem.eql(u8, hdr.version, "2.1"), true);
    expect_eq(std.mem.eql(u8, hdr.font_name, "-Misc-Fixed-Medium-R-Normal--10-100-75-75-C-60-ISO10646-1"), true);
    expect_eq(hdr.point_size, 10);
    expect_eq(hdr.x_res, 75);
    expect_eq(hdr.y_res, 75);
    expect_eq(hdr.bbox.width, 6);
    expect_eq(hdr.bbox.height, 10);
    expect_eq(hdr.bbox.x_off, 0);
    expect_eq(hdr.bbox.y_off, -2);
    expect_eq(hdr.ascent, 8);
    expect_eq(hdr.descent, 2);
    expect_eq(hdr.declared_chars, 2);
    expect_eq(hdr.chars_offset > 0 && hdr.chars_offset < SAMPLE_BDF.len, true);

    let pit = BdfPropIter.init(SAMPLE_BDF);
    const p0 = pit.next().?;
    expect_eq(std.mem.eql(u8, p0.name, "FAMILY_NAME"), true);
    expect_eq(std.mem.eql(u8, p0.value, "Fixed \"\"Mono\"\""), true);

    const p1 = pit.next().?;
    expect_eq(std.mem.eql(u8, p1.name, "FOUNDRY"), true);
    expect_eq(std.mem.eql(u8, p1.value, "Misc"), true);

    const p2 = pit.next().?;
    expect_eq(std.mem.eql(u8, p2.name, "FONT_ASCENT"), true);
    expect_eq(std.mem.eql(u8, p2.value, "8"), true);

    const p3 = pit.next().?;
    expect_eq(std.mem.eql(u8, p3.name, "FONT_DESCENT"), true);
    expect_eq(std.mem.eql(u8, p3.value, "2"), true);

    expect_eq(pit.next() == null, true);
}

fn testGlyphIterAndMsbFirstPixels() {
    const hdr = parseBdfHeader(SAMPLE_BDF).?;
    let git = BdfGlyphIter.initFromHeader(SAMPLE_BDF, hdr);

    const g0 = git.next().?;
    expect_eq(std.mem.eql(u8, g0.name, "A"), true);
    expect_eq(g0.encoding, 65);
    expect_eq(g0.dwidth_x, 6);
    expect_eq(g0.dwidth_y, 0);
    expect_eq(g0.bbox.width, 5);
    expect_eq(g0.bbox.height, 4);

    // Row 0 = 0x70 = 0111_0000 (MSB-first cols 0..4 => 0,1,1,1,0)
    expect_eq(bdfGlyphPixel(g0, 0, 0).?, false);
    expect_eq(bdfGlyphPixel(g0, 0, 1).?, true);
    expect_eq(bdfGlyphPixel(g0, 0, 2).?, true);
    expect_eq(bdfGlyphPixel(g0, 0, 3).?, true);
    expect_eq(bdfGlyphPixel(g0, 0, 4).?, false);

    // Row 1 = 0x88 = 1000_1000 (cols 0..4 => 1,0,0,0,1)
    expect_eq(bdfGlyphPixel(g0, 1, 0).?, true);
    expect_eq(bdfGlyphPixel(g0, 1, 1).?, false);
    expect_eq(bdfGlyphPixel(g0, 1, 4).?, true);

    // Row 2 = 0xF8 = 1111_1000 (all 5 cols lit)
    expect_eq(bdfGlyphPixel(g0, 2, 0).?, true);
    expect_eq(bdfGlyphPixel(g0, 2, 2).?, true);
    expect_eq(bdfGlyphPixel(g0, 2, 4).?, true);

    // Glyph 1: unencoded (-1)
    const g1 = git.next().?;
    expect_eq(std.mem.eql(u8, g1.name, "notdef"), true);
    expect_eq(g1.encoding, -1);
    expect_eq(bdfGlyphPixel(g1, 0, 0).?, true);
    expect_eq(bdfGlyphPixel(g1, 1, 1).?, false);
    expect_eq(bdfGlyphPixel(g1, 1, 3).?, true);

    expect_eq(git.next() == null, true);
}

fn testFindBdfGlyphByCodepoint() {
    const gA = findBdfGlyph(SAMPLE_BDF, 65).?;
    expect_eq(std.mem.eql(u8, gA.name, "A"), true);

    const gUndef = findBdfGlyph(SAMPLE_BDF, -1).?;
    expect_eq(std.mem.eql(u8, gUndef.name, "notdef"), true);

    expect_eq(findBdfGlyph(SAMPLE_BDF, 9999) == null, true);
}

fn testCrlfAnd12PxWideMultiByteGlyph() {
    // 12px wide -> ((12 + 7) / 8) * 2 = 4 hex chars per row
    // Row 0: 0xA050 = 1010_0000 0101_0000
    //   col 0=1, col 1=0, col 2=1, col 8=0, col 9=1, col 10=0, col 11=1
    const crlf_bdf =
        "STARTFONT 2.2\r\n" ++
        "FONT WideTest\r\n" ++
        "SIZE 12 72 72\r\n" ++
        "FONTBOUNDINGBOX 12 2 0 0\r\n" ++
        "CHARS 1\r\n" ++
        "STARTCHAR wide\r\n" ++
        "ENCODING 12354\r\n" ++
        "DWIDTH 12 0\r\n" ++
        "BBX 12 2 0 0\r\n" ++
        "BITMAP\r\n" ++
        "A050\r\n" ++
        "0FF0\r\n" ++
        "ENDCHAR\r\n" ++
        "ENDFONT\r\n";

    const hdr = parseBdfHeader(crlf_bdf).?;
    expect_eq(std.mem.eql(u8, hdr.version, "2.2"), true);

    const g = findBdfGlyph(crlf_bdf, 12354).?;
    expect_eq(g.bbox.width, 12);
    expect_eq(g.bbox.height, 2);

    // Row 0: A0 50
    expect_eq(bdfGlyphPixel(g, 0, 0).?, true);
    expect_eq(bdfGlyphPixel(g, 0, 1).?, false);
    expect_eq(bdfGlyphPixel(g, 0, 2).?, true);
    expect_eq(bdfGlyphPixel(g, 0, 8).?, false);
    expect_eq(bdfGlyphPixel(g, 0, 9).?, true);
    expect_eq(bdfGlyphPixel(g, 0, 10).?, false);
    expect_eq(bdfGlyphPixel(g, 0, 11).?, true);

    // Row 1: 0F F0 -> cols 4..11 lit, cols 0..3 dark
    expect_eq(bdfGlyphPixel(g, 1, 3).?, false);
    expect_eq(bdfGlyphPixel(g, 1, 4).?, true);
    expect_eq(bdfGlyphPixel(g, 1, 11).?, true);
}

fn testMalformedAndOutOfBoundsGuards() {
    // Non-BDF inputs
    expect_eq(isBdf(""), false);
    expect_eq(isBdf("NOT A FONT"), false);
    expect_eq(parseBdfHeader("NOT A FONT") == null, true);
    expect_eq(findBdfGlyph("NOT A FONT", 65) == null, true);

    // Missing ENDCHAR must not yield truncated glyph
    const missing_endchar =
        "STARTFONT 2.1\n" ++
        "CHARS 1\n" ++
        "STARTCHAR broken\n" ++
        "ENCODING 66\n" ++
        "BBX 4 2 0 0\n" ++
        "BITMAP\n" ++
        "F0\n";
    let git = BdfGlyphIter.init(missing_endchar);
    expect_eq(git.next() == null, true);

    // Out-of-bounds row/col pixel query
    const gA = findBdfGlyph(SAMPLE_BDF, 65).?;
    expect_eq(bdfGlyphPixel(gA, 4, 0) == null, true);  // height is 4 -> row 4 OOB
    expect_eq(bdfGlyphPixel(gA, 0, 5) == null, true);  // width is 5 -> col 5 OOB

    // Invalid hex in bitmap returns null
    const bad_hex_glyph = BdfGlyph {
        name: "bad",
        encoding: 67,
        dwidth_x: 4,
        dwidth_y: 0,
        bbox: BdfBBox { width: 4, height: 1, x_off: 0, y_off: 0 },
        bitmap_slice: "ZZ\n",
    };
    expect_eq(bdfGlyphPixel(bad_hex_glyph, 0, 0) == null, true);
}

test("1. BDF 2.1 Global Header & STARTPROPERTIES Iteration", () => {
    testBdfHeaderAndProperties();
});

test("2. BdfGlyphIter Multi-Glyph Parsing & MSB-First Pixel Decoding", () => {
    testGlyphIterAndMsbFirstPixels();
});

test("3. findBdfGlyph Lookup by Codepoint (ASCII & -1 Unencoded)", () => {
    testFindBdfGlyphByCodepoint();
});

test("4. CRLF Line Endings & 12px Wide Multi-Byte Glyph Rows", () => {
    testCrlfAnd12PxWideMultiByteGlyph();
});

test("5. Non-BDF, Missing ENDCHAR & Out-of-Bounds Pixel Guards", () => {
    testMalformedAndOutOfBoundsGuards();
});
