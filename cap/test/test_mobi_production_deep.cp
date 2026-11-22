// Source: cp/cap/test/test_mobi_production_deep.cp (Production deep tests for cap:mobi)

import { test, expect_eq } from "cap:test";
import {
    PDB_HEADER_SIZE,
    PDB_RECORD_ENTRY_SIZE,
    MOBI_MAGIC,
    EXTH_MAGIC,
    EXTH_AUTHOR,
    EXTH_PUBLISHER,
    EXTH_ISBN,
    EXTH_LANGUAGE,
    MobiCompression,
    MobiEncoding,
    decodeMobiCompression,
    decodeMobiEncoding,
    PdbHeader,
    MobiHeader,
    ExthRecord,
    isMobi,
    parsePdbHeader,
    parseMobiHeader,
    ExthIter,
    findExthRecord,
} from "cap:text";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
}

fn writeBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn writeU16BE(dst: []u8, off: usize, val: u16) {
    dst[off + 0] = ((val >> 8) & 0xFF) as u8;
    dst[off + 1] = (val & 0xFF) as u8;
}

fn writeU32BE(dst: []u8, off: usize, val: u32) {
    dst[off + 0] = ((val >> 24) & 0xFF) as u8;
    dst[off + 1] = ((val >> 16) & 0xFF) as u8;
    dst[off + 2] = ((val >> 8) & 0xFF) as u8;
    dst[off + 3] = (val & 0xFF) as u8;
}

fn writeExthRec(dst: []u8, off: usize, rtype: u32, val: []const u8): usize {
    const total: usize = (8 as usize) + val.len;
    writeU32BE(dst, off, rtype);
    writeU32BE(dst, off + 4, total as u32);
    * = writeBytes(dst, off + 8, val);
    return off + total;
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. parses complete PalmDB + PalmDOC + MOBI + full_name + EXTH Author/Publisher/ISBN/Language", () => {
    let raw: [512]u8 = undefined;
    let buf = raw[0..512];
    zeroBytes(buf);

    // PalmDB Header (78B) + 2 record entries (16B) = 94B -> Record 0 starts at 96
    * = writeBytes(buf, 0, "CP_Handbook");
    writeU16BE(buf, 32, 0);
    writeU16BE(buf, 34, 0);
    writeU32BE(buf, 36, 1700000000);
    writeU32BE(buf, 40, 1700000500);
    * = writeBytes(buf, 60, "BOOKMOBI");
    writeU16BE(buf, 76, 2); // 2 records

    const rec0_off: usize = 96;
    writeU32BE(buf, 78, rec0_off as u32); // Record 0 offset

    // Record 0: 16B PalmDOC + 232B MOBI header (ends at rec0 + 248)
    writeU16BE(buf, rec0_off + 0, 2); // PalmDOC compression
    writeU32BE(buf, rec0_off + 4, 8192); // text_length
    writeU16BE(buf, rec0_off + 8, 2); // text_record_count
    writeU16BE(buf, rec0_off + 10, 4096); // record_size
    writeU16BE(buf, rec0_off + 12, 0); // no DRM

    // MOBI header at rec0 + 16, length = 232
    * = writeBytes(buf, rec0_off + 16, MOBI_MAGIC);
    writeU32BE(buf, rec0_off + 20, 232); // header_len
    writeU32BE(buf, rec0_off + 24, 2); // mobi_type (Mobipocket Book)
    writeU32BE(buf, rec0_off + 28, 65001); // UTF-8
    writeU32BE(buf, rec0_off + 32, 0x12345678); // unique_id
    writeU32BE(buf, rec0_off + 36, 6); // file_version
    writeU32BE(buf, rec0_off + 92, 0x0409); // en-US locale
    writeU32BE(buf, rec0_off + 128, 0x50); // exth_flags with bit 0x40 set

    // EXTH header at rec0 + 16 + 232 = rec0 + 248
    const exth_off = rec0_off + 248;
    * = writeBytes(buf, exth_off, EXTH_MAGIC);
    let ep = exth_off + 12;
    ep = writeExthRec(buf, ep, EXTH_AUTHOR, "Ada Lovelace");
    ep = writeExthRec(buf, ep, EXTH_PUBLISHER, "CP Press");
    ep = writeExthRec(buf, ep, EXTH_ISBN, "978-0-123456-78-9");
    ep = writeExthRec(buf, ep, EXTH_LANGUAGE, "en");
    const exth_len = ep - exth_off;
    writeU32BE(buf, exth_off + 4, exth_len as u32);
    writeU32BE(buf, exth_off + 8, 4); // 4 EXTH records

    // Place full_name right after EXTH inside Record 0
    const fn_rel = ep - rec0_off;
    const title = "The CP Architecture Book";
    const rec0_end = writeBytes(buf, ep, title);
    writeU32BE(buf, rec0_off + 84, fn_rel as u32);
    writeU32BE(buf, rec0_off + 88, title.len as u32);

    // Record 1 starts at rec0_end
    const rec1_off = rec0_end;
    writeU32BE(buf, 78 + 8, rec1_off as u32);
    const total_len = writeBytes(buf, rec1_off, "CHAPTER_1_BODY");

    const mobi = buf[0..total_len];
    expect_eq(PDB_HEADER_SIZE, 78);
    expect_eq(PDB_RECORD_ENTRY_SIZE, 8);
    expect_eq(isMobi(mobi), true);

    if (pdb := parsePdbHeader(mobi)) {
        expect_eq(sliceEq(pdb.name, "CP_Handbook"), true);
        expect_eq(pdb.record_count, 2);
        if (r1 := pdb.recordSlice(mobi, 1)) {
            expect_eq(sliceEq(r1, "CHAPTER_1_BODY"), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }

    if (hdr := parseMobiHeader(mobi)) {
        expect_eq(hdr.compression == MobiCompression.palmdoc, true);
        expect_eq(hdr.text_length, 8192);
        expect_eq(hdr.record_size, 4096);
        expect_eq(hdr.text_encoding == MobiEncoding.utf8, true);
        expect_eq(hdr.file_version, 6);
        expect_eq(hdr.locale, 0x0409);
        expect_eq(sliceEq(hdr.full_name, title), true);
        expect_eq(hdr.has_exth, true);
        expect_eq(hdr.exth_offset, exth_off);

        if (a := findExthRecord(mobi, hdr, EXTH_AUTHOR)) {
            expect_eq(sliceEq(a, "Ada Lovelace"), true);
        } else { expect_eq(true, false); }
        if (pub_name := findExthRecord(mobi, hdr, EXTH_PUBLISHER)) {
            expect_eq(sliceEq(pub_name, "CP Press"), true);
        } else { expect_eq(true, false); }
        if (isbn := findExthRecord(mobi, hdr, EXTH_ISBN)) {
            expect_eq(sliceEq(isbn, "978-0-123456-78-9"), true);
        } else { expect_eq(true, false); }
        if (lang := findExthRecord(mobi, hdr, EXTH_LANGUAGE)) {
            expect_eq(sliceEq(lang, "en"), true);
        } else { expect_eq(true, false); }
        expect_eq(findExthRecord(mobi, hdr, 999) == null, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. verifies MobiCompression and MobiEncoding enum decoding", () => {
    expect_eq(decodeMobiCompression(1) == MobiCompression.none, true);
    expect_eq(decodeMobiCompression(2) == MobiCompression.palmdoc, true);
    expect_eq(decodeMobiCompression(17480) == MobiCompression.huff_cdic, true);
    expect_eq(decodeMobiCompression(99) == MobiCompression.unknown, true);

    expect_eq(decodeMobiEncoding(1252) == MobiEncoding.cp1252, true);
    expect_eq(decodeMobiEncoding(65001) == MobiEncoding.utf8, true);
    expect_eq(decodeMobiEncoding(42) == MobiEncoding.unknown, true);
});

test("3. parses compact MOBI header without EXTH flag (has_exth == false)", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    zeroBytes(buf);

    * = writeBytes(buf, 60, "BOOKMOBI");
    writeU16BE(buf, 76, 1);
    writeU32BE(buf, 78, 96);

    writeU16BE(buf, 96, 17480); // HUFF/CDIC
    * = writeBytes(buf, 96 + 16, MOBI_MAGIC);
    writeU32BE(buf, 96 + 20, 132); // header_len
    writeU32BE(buf, 96 + 28, 1252); // CP1252
    writeU32BE(buf, 96 + 128, 0x00); // no EXTH flag

    if (hdr := parseMobiHeader(buf[0..250])) {
        expect_eq(hdr.compression == MobiCompression.huff_cdic, true);
        expect_eq(hdr.text_encoding == MobiEncoding.cp1252, true);
        expect_eq(hdr.has_exth, false);

        let eit = ExthIter.init(buf[0..250], hdr);
        expect_eq(eit.next() == null, true);
        expect_eq(eit.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("4. rejects non-monotonic PalmDB record offsets", () => {
    let raw: [160]u8 = undefined;
    let buf = raw[0..160];
    zeroBytes(buf);

    * = writeBytes(buf, 60, "BOOKMOBI");
    writeU16BE(buf, 76, 2);
    writeU32BE(buf, 78, 120); // rec 0 starts at 120
    writeU32BE(buf, 86, 100); // rec 1 starts at 100 < 120 (non-monotonic!)

    if (pdb := parsePdbHeader(buf)) {
        expect_eq(pdb.recordSlice(buf, 0) == null, true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(parseMobiHeader(buf) == null, true);
});

test("5. rejects bad BOOKMOBI/MOBI/EXTH magic and out-of-bounds full_name", () => {
    let raw: [300]u8 = undefined;
    let buf = raw[0..300];
    zeroBytes(buf);

    // Bad BOOKMOBI magic
    * = writeBytes(buf, 60, "NOTAMOBI");
    expect_eq(isMobi(buf), false);
    expect_eq(parseMobiHeader(buf) == null, true);

    // Valid BOOKMOBI, but bad MOBI magic in record 0
    * = writeBytes(buf, 60, "BOOKMOBI");
    writeU16BE(buf, 76, 1);
    writeU32BE(buf, 78, 96);
    * = writeBytes(buf, 96 + 16, "NOPE");
    expect_eq(parseMobiHeader(buf) == null, true);

    // Valid MOBI, but out-of-bounds full_name
    * = writeBytes(buf, 96 + 16, MOBI_MAGIC);
    writeU32BE(buf, 96 + 20, 120);
    writeU32BE(buf, 96 + 84, 5000); // full_name_offset > rec0.len
    writeU32BE(buf, 96 + 88, 10);
    expect_eq(parseMobiHeader(buf) == null, true);

    // Valid MOBI with has_exth=true, but corrupted EXTH magic
    writeU32BE(buf, 96 + 84, 0);
    writeU32BE(buf, 96 + 88, 0);
    writeU32BE(buf, 96 + 128, 0x40);
    * = writeBytes(buf, 96 + 16 + 120, "XXXX");
    if (hdr := parseMobiHeader(buf)) {
        let eit = ExthIter.init(buf, hdr);
        expect_eq(eit.next() == null, true);
        expect_eq(eit.had_error, true);
    } else {
        expect_eq(true, false);
    }
});
