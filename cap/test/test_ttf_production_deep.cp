// Source: cp/cap/test/test_ttf_production_deep.cp (Production deep tests for cap:ttf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SfntFlavor,
    SfntOffsetTable,
    SfntTableRecord,
    TtfHeadMetrics,
    parseSfntHeader,
    SfntTableIter,
    findSfntTable,
    parseTtfHead,
    parseTtfNumGlyphs,
    calcSfntChecksum,
} from "cap:gfx";

fn putU16BE(buf: *[]u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn putI16BE(buf: *[]u8, off: usize, v: i16) {
    const u: u16 = if (v < 0) ((v as i32) + 65536) as u16 else (v as u16);
    putU16BE(buf, off, u);
}

fn putU32BE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putTag(buf: *[]u8, off: usize, t: []const u8) {
    buf[off] = t[0];
    buf[off + 1] = t[1];
    buf[off + 2] = t[2];
    buf[off + 3] = t[3];
}

/// Build a valid minimal TrueType SFNT buffer (12 header + 2*16 dir + 56 padded head + 8 padded maxp = 108 bytes)
fn buildMinimalTtf(out: *[]u8) {
    let i: usize = 0;
    while (i < out.len) {
        out[i] = 0;
        i += 1;
    }

    // Offset Table (12 bytes)
    putU32BE(out, 0, 0x00010000); // TrueType
    putU16BE(out, 4, 2);          // numTables = 2
    putU16BE(out, 6, 32);         // searchRange
    putU16BE(out, 8, 1);          // entrySelector
    putU16BE(out, 10, 0);         // rangeShift

    // Table 0: "head" at offset 44, length 54
    putTag(out, 12, "head");
    putU32BE(out, 20, 44);
    putU32BE(out, 24, 54);

    // Table 1: "maxp" at offset 100, length 6
    putTag(out, 28, "maxp");
    putU32BE(out, 36, 100);
    putU32BE(out, 40, 6);

    // Populate "head" (offset 44..98)
    putU32BE(out, 44 + 0, 0x00010000);  // version 1.0
    putU32BE(out, 44 + 12, 0x5F0F3CF5); // magicNumber
    putU16BE(out, 44 + 18, 2048);       // unitsPerEm
    putI16BE(out, 44 + 36, -250);       // xMin
    putI16BE(out, 44 + 38, -512);       // yMin
    putI16BE(out, 44 + 40, 1800);       // xMax
    putI16BE(out, 44 + 42, 1950);       // yMax
    putI16BE(out, 44 + 50, 1);          // indexToLocFormat (long)

    // Populate "maxp" (offset 100..106)
    putU32BE(out, 100 + 0, 0x00010000); // version 1.0
    putU16BE(out, 100 + 4, 428);        // numGlyphs

    // Write real checksums into directory
    const head_cs = calcSfntChecksum(out[44..98]);
    putU32BE(out, 16, head_cs);
    const maxp_cs = calcSfntChecksum(out[100..106]);
    putU32BE(out, 32, maxp_cs);
}

test("1. parseSfntHeader recognizes TrueType, OpenType CFF (OTTO), Apple true, and rejects bad/truncated headers", () => {
    let raw: [108]u8 = undefined;
    let s = raw[0..108];
    buildMinimalTtf(*s);

    const h_ttf = parseSfntHeader(s) ?? unreachable;
    expect_eq(h_ttf.flavor == SfntFlavor.truetype, true);
    expect_eq(h_ttf.num_tables, 2);
    expect_eq(h_ttf.search_range, 32);
    expect_eq(h_ttf.entry_selector, 1);

    // OpenType CFF ("OTTO")
    let otf: [12]u8 = undefined;
    let s_otf = otf[0..12];
    putU32BE(*s_otf, 0, 0x4F54544F);
    putU16BE(*s_otf, 4, 0);
    putU16BE(*s_otf, 6, 0);
    putU16BE(*s_otf, 8, 0);
    putU16BE(*s_otf, 10, 0);
    const h_otf = parseSfntHeader(s_otf) ?? unreachable;
    expect_eq(h_otf.flavor == SfntFlavor.cff, true);

    // Apple "true"
    putU32BE(*s_otf, 0, 0x74727565);
    const h_apple = parseSfntHeader(s_otf) ?? unreachable;
    expect_eq(h_apple.flavor == SfntFlavor.apple_true, true);

    // Unknown magic & truncated directory
    putU32BE(*s_otf, 0, 0x12345678);
    expect_eq(parseSfntHeader(s_otf) == null, true);
    expect_eq(parseSfntHeader(s[0..20]) == null, true); // declares 2 tables (needs 44 bytes)
});

test("2. SfntTableIter and findSfntTable enumerate records and verify zero-copy slices & checksums", () => {
    let raw: [108]u8 = undefined;
    let s = raw[0..108];
    buildMinimalTtf(*s);

    let it = SfntTableIter.init(s);
    const r0 = it.next() ?? unreachable;
    expect_eq(r0.tagEquals("head"), true);
    expect_eq(r0.offset, 44);
    expect_eq(r0.length, 54);
    const head_slice = r0.slice(s) ?? unreachable;
    expect_eq(head_slice.len, 54);
    expect_eq(calcSfntChecksum(head_slice), r0.checksum);

    const r1 = it.next() ?? unreachable;
    expect_eq(r1.tagEquals("maxp"), true);
    expect_eq(r1.offset, 100);
    expect_eq(r1.length, 6);
    const maxp_slice = r1.slice(s) ?? unreachable;
    expect_eq(calcSfntChecksum(maxp_slice), r1.checksum);

    expect_eq(it.next() == null, true);

    const found = findSfntTable(s, "maxp") ?? unreachable;
    expect_eq(found.length, 6);
    expect_eq(findSfntTable(s, "cmap") == null, true);
    expect_eq(findSfntTable(s, "ab") == null, true);
});

test("3. parseTtfHead and parseTtfNumGlyphs decode signed bbox metrics and glyph count", () => {
    let raw: [108]u8 = undefined;
    let s = raw[0..108];
    buildMinimalTtf(*s);

    const hm = parseTtfHead(s) ?? unreachable;
    expect_eq(hm.units_per_em, 2048);
    expect_eq(hm.x_min, -250);
    expect_eq(hm.y_min, -512);
    expect_eq(hm.x_max, 1800);
    expect_eq(hm.y_max, 1950);
    expect_eq(hm.index_to_loc_format, 1);

    const glyphs = parseTtfNumGlyphs(s) ?? unreachable;
    expect_eq(glyphs, 428);
});

test("4. parseTtfHead rejects corrupted magicNumber and SfntTableRecord.slice guards against overflow/OOB", () => {
    let raw: [108]u8 = undefined;
    let s = raw[0..108];
    buildMinimalTtf(*s);

    // Corrupt head magicNumber
    putU32BE(*s, 44 + 12, 0xDEADBEEF);
    expect_eq(parseTtfHead(s) == null, true);

    // Out-of-bounds offset + length overflow protection
    const oob_rec = SfntTableRecord {
        tag: ['t', 'e', 's', 't'],
        checksum: 0,
        offset: 0xFFFFFFF0,
        length: 0x00000020,
    };
    expect_eq(oob_rec.slice(s) == null, true);
});

test("5. calcSfntChecksum handles 4-byte aligned, unaligned tail padding, and 32-bit wrapping addition", () => {
    // 0xFFFFFFFF + 0x00000002 wraps to 0x00000001
    const wrap_bytes: [8]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x02];
    expect_eq(calcSfntChecksum(wrap_bytes[0..8]), 1);

    // 5-byte unaligned input: 0x01020304 + 0x05000000 = 0x06020304
    const unaligned5: [5]u8 = [0x01, 0x02, 0x03, 0x04, 0x05];
    expect_eq(calcSfntChecksum(unaligned5[0..5]), 0x06020304);

    // 6-byte unaligned input: 0x00010000 + 0x01AC0000 = 0x01AD0000
    const unaligned6: [6]u8 = [0x00, 0x01, 0x00, 0x00, 0x01, 0xAC];
    expect_eq(calcSfntChecksum(unaligned6[0..6]), 0x01AD0000);

    // Empty input
    expect_eq(calcSfntChecksum(""), 0);
});
