// Source: cp/cap/test/test_fits_production_deep.cp (Production deep tests for cap:fits)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FITS_BLOCK_SIZE,
    FITS_CARD_SIZE,
    FITS_CARDS_PER_BLOCK,
    FitsCard,
    FitsHeader,
    FitsCardIter,
    isFits,
    parseFitsHeader,
    fitsBytesPerPixel,
    findFitsCard,
    parseFitsInt,
    parseFitsBool,
} from "cap:fits";

fn fillSpaces(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = ' ';
        i += 1;
    }
}

fn writeCard(buf: []u8, card_idx: usize, text: []const u8) {
    const off = card_idx * FITS_CARD_SIZE;
    let i: usize = 0;
    while (i < text.len && i < FITS_CARD_SIZE) {
        buf[off + i] = text[i];
        i += 1;
    }
}

test("1. Standard 2D scientific image Primary HDU parsing (2880B single block, BITPIX=-32)", () => {
    expect_eq(FITS_BLOCK_SIZE, 2880);
    expect_eq(FITS_CARD_SIZE, 80);
    expect_eq(FITS_CARDS_PER_BLOCK, 36);

    let blk: [2880]u8 = undefined;
    fillSpaces(blk[0..]);
    writeCard(blk[0..], 0, "SIMPLE  =                    T / conforms to FITS standard");
    writeCard(blk[0..], 1, "BITPIX  =                  -32 / IEEE single precision float");
    writeCard(blk[0..], 2, "NAXIS   =                    2 / 2D image");
    writeCard(blk[0..], 3, "NAXIS1  =                 1024 / width");
    writeCard(blk[0..], 4, "NAXIS2  =                  512 / height");
    writeCard(blk[0..], 5, "END");

    expect_eq(isFits(blk[0..]), true);
    const hdr = parseFitsHeader(blk[0..]) ?? unreachable;
    expect_eq(hdr.simple, true);
    expect_eq(hdr.bitpix, -32);
    expect_eq(hdr.naxis, 2);
    expect_eq(hdr.naxis1, 1024);
    expect_eq(hdr.naxis2, 512);
    expect_eq(hdr.naxis3, 0);
    expect_eq(hdr.header_bytes, 2880);
    // 1024 * 512 * 4 = 2097152 bytes
    expect_eq(hdr.data_bytes, 2097152);
    expect_eq(hdr.card_count, 5);
});

test("2. Multi-block 5760B header alignment (40 cards) and 3D cube data_bytes calculation", () => {
    let buf: [5760]u8 = undefined;
    fillSpaces(buf[0..]);
    writeCard(buf[0..], 0, "SIMPLE  =                    T");
    writeCard(buf[0..], 1, "BITPIX  =                   16");
    writeCard(buf[0..], 2, "NAXIS   =                    3");
    writeCard(buf[0..], 3, "NAXIS1  =                  100");
    writeCard(buf[0..], 4, "NAXIS2  =                   50");
    writeCard(buf[0..], 5, "NAXIS3  =                    4");

    // Fill cards 6..38 with COMMENT cards so END lands in block 2 (card 39)
    let c: usize = 6;
    while (c < 39) {
        writeCard(buf[0..], c, "COMMENT calibration frame metadata");
        c += 1;
    }
    writeCard(buf[0..], 39, "END");

    const hdr = parseFitsHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.header_bytes, 5760);
    expect_eq(hdr.card_count, 39);
    // 100 * 50 * 4 * 2 bytes = 40000
    expect_eq(hdr.data_bytes, 40000);
});

test("3. fitsBytesPerPixel, parseFitsInt, and parseFitsBool edge cases", () => {
    expect_eq(fitsBytesPerPixel(8) ?? 0, 1);
    expect_eq(fitsBytesPerPixel(16) ?? 0, 2);
    expect_eq(fitsBytesPerPixel(32) ?? 0, 4);
    expect_eq(fitsBytesPerPixel(64) ?? 0, 8);
    expect_eq(fitsBytesPerPixel(-32) ?? 0, 4);
    expect_eq(fitsBytesPerPixel(-64) ?? 0, 8);
    expect_eq(fitsBytesPerPixel(24) == null, true);
    expect_eq(fitsBytesPerPixel(0) == null, true);

    expect_eq(parseFitsBool("   T  ") ?? false, true);
    expect_eq(parseFitsBool("F") ?? true, false);
    expect_eq(parseFitsBool("TRUE") == null, true);

    expect_eq(parseFitsInt("  -64 ") ?? 0, -64);
    expect_eq(parseFitsInt("+4096") ?? 0, 4096);
    expect_eq(parseFitsInt("0") ?? 1, 0);
    expect_eq(parseFitsInt("12x") == null, true);
    expect_eq(parseFitsInt("") == null, true);
});

test("4. FitsCardIter and findFitsCard preserve slashes/escaped quotes inside strings and strip comments", () => {
    let blk: [2880]u8 = undefined;
    fillSpaces(blk[0..]);
    writeCard(blk[0..], 0, "SIMPLE  =                    T / standard FITS");
    writeCard(blk[0..], 1, "TELESCOP= 'JWST/NIRCam'        / space telescope");
    writeCard(blk[0..], 2, "OBSERVER= 'O''Connor'          / principal investigator");
    writeCard(blk[0..], 3, "COMMENT   Dark subtracted frame");
    writeCard(blk[0..], 4, "END");

    const tel = findFitsCard(blk[0..], "TELESCOP") ?? unreachable;
    expect_eq(tel.has_value, true);
    expect_eq(std.mem.eql(u8, tel.value, "'JWST/NIRCam'"), true);
    expect_eq(std.mem.eql(u8, tel.comment ?? "", "space telescope"), true);

    const obs = findFitsCard(blk[0..], "OBSERVER") ?? unreachable;
    expect_eq(std.mem.eql(u8, obs.value, "'O''Connor'"), true);
    expect_eq(std.mem.eql(u8, obs.comment ?? "", "principal investigator"), true);

    // Iterate all cards before END (exactly 4 cards)
    let it = FitsCardIter.init(blk[0..]);
    let count: u32 = 0;
    while (card := it.next()) {
        if (std.mem.eql(u8, card.keyword, "COMMENT")) {
            expect_eq(card.has_value, false);
            expect_eq(std.mem.eql(u8, card.comment ?? "", "Dark subtracted frame"), true);
        }
        count += 1;
    }
    expect_eq(count, 4);
    expect_eq(findFitsCard(blk[0..], "MISSING") == null, true);
});

test("5. Malformed FITS protection: short buffer, non-SIMPLE magic, missing END, truncated 2nd block", () => {
    // Too short (< 2880)
    expect_eq(isFits("SIMPLE  =                    T"), false);
    expect_eq(parseFitsHeader("SIMPLE  =                    T") == null, true);

    // Non-SIMPLE first card
    let bad_magic: [2880]u8 = undefined;
    fillSpaces(bad_magic[0..]);
    writeCard(bad_magic[0..], 0, "XTENSION= 'IMAGE   '");
    writeCard(bad_magic[0..], 1, "END");
    expect_eq(isFits(bad_magic[0..]), false);
    expect_eq(parseFitsHeader(bad_magic[0..]) == null, true);

    // Missing END card
    let no_end: [2880]u8 = undefined;
    fillSpaces(no_end[0..]);
    writeCard(no_end[0..], 0, "SIMPLE  =                    T");
    writeCard(no_end[0..], 1, "BITPIX  =                    8");
    writeCard(no_end[0..], 2, "NAXIS   =                    0");
    expect_eq(parseFitsHeader(no_end[0..]) == null, true);

    // Missing required NAXIS2 when NAXIS=2
    let miss_axis: [2880]u8 = undefined;
    fillSpaces(miss_axis[0..]);
    writeCard(miss_axis[0..], 0, "SIMPLE  =                    T");
    writeCard(miss_axis[0..], 1, "BITPIX  =                    8");
    writeCard(miss_axis[0..], 2, "NAXIS   =                    2");
    writeCard(miss_axis[0..], 3, "NAXIS1  =                   16");
    writeCard(miss_axis[0..], 4, "END");
    expect_eq(parseFitsHeader(miss_axis[0..]) == null, true);

    // END in block 2 (card 36 -> offset 2960) but total buffer is only 3000 bytes (< 5760 aligned block!)
    let trunc_blk2: [3000]u8 = undefined;
    fillSpaces(trunc_blk2[0..]);
    writeCard(trunc_blk2[0..], 0, "SIMPLE  =                    T");
    writeCard(trunc_blk2[0..], 1, "BITPIX  =                    8");
    writeCard(trunc_blk2[0..], 2, "NAXIS   =                    0");
    writeCard(trunc_blk2[0..], 36, "END");
    expect_eq(parseFitsHeader(trunc_blk2[0..]) == null, true);
});
