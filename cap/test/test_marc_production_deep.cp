// Source: cp/cap/test/test_marc_production_deep.cp
// Deep Production Test Suite for cap:marc
// Covers:
//   1. Complete MARC 21 UTF-8 bibliographic record (001/008 control + 020/100/245/264 data fields)
//   2. Targeted findSubfield extraction ($a/$b/$c) & control vs data field behavior
//   3. MarcRecordIter multi-record concatenated .mrc catalog stream (UTF-8 + MARC-8)
//   4. Exact indicator & subfield iteration on multi-subfield 245/264 fields
//   5. Non-12B directory alignment, missing 0x1D / 0x1E terminators & out-of-bounds field_len guards

import { test, expect_eq } from "cap:test";
import {
    MARC_LEADER_SIZE,
    MARC_DIR_ENTRY_SIZE,
    MARC_RECORD_TERMINATOR,
    MARC_FIELD_TERMINATOR,
    MARC_SUBFIELD_DELIM,
    MarcLeader,
    MarcSubfield,
    MarcSubfieldIter,
    MarcField,
    MarcFieldIter,
    MarcRecord,
    MarcRecordIter,
    isMarcRecord,
    parseMarcLeader,
} from "cap:marc";

struct MarcBuilder {
    dir_buf: [512]u8 = [0; 512],
    dir_len: usize = 0,
    data_buf: [1024]u8 = [0; 1024],
    data_len: usize = 0,
    out_buf: [2048]u8 = [0; 2048],
    out_len: usize = 0,

    fn writeDec(dst: []u8, off: usize, width: usize, val: usize) {
        let v = val;
        let i = width;
        while (i > 0) {
            i -= 1;
            dst[off + i] = ('0' as u8) + ((v % 10) as u8);
            v = v / 10;
        }
    }

    fn addRawField(*_, tag3: []const u8, body: []const u8) {
        const start_pos = _.data_len;
        const field_len = body.len + 1; // + 0x1E terminator

        // Append 12-byte directory entry
        _.dir_buf[_.dir_len] = tag3[0];
        _.dir_buf[_.dir_len + 1] = tag3[1];
        _.dir_buf[_.dir_len + 2] = tag3[2];
        MarcBuilder.writeDec(_.dir_buf[0..512], _.dir_len + 3, 4, field_len);
        MarcBuilder.writeDec(_.dir_buf[0..512], _.dir_len + 7, 5, start_pos);
        _.dir_len += 12;

        // Append field bytes + 0x1E
        let i: usize = 0;
        while (i < body.len) {
            _.data_buf[_.data_len] = body[i];
            _.data_len += 1;
            i += 1;
        }
        _.data_buf[_.data_len] = MARC_FIELD_TERMINATOR;
        _.data_len += 1;
    }

    fn finish(*_, char_enc: u8): []const u8 {
        const base_addr = MARC_LEADER_SIZE + _.dir_len + 1; // + dir 0x1E
        const rec_len = base_addr + _.data_len + 1;         // + record 0x1D

        // 24-byte Leader
        let i: usize = 0;
        while (i < 24) {
            _.out_buf[i] = ' ';
            i += 1;
        }
        MarcBuilder.writeDec(_.out_buf[0..2048], 0, 5, rec_len);
        _.out_buf[5] = 'n';
        _.out_buf[6] = 'a';
        _.out_buf[7] = 'm';
        _.out_buf[9] = char_enc;
        _.out_buf[10] = '2';
        _.out_buf[11] = '2';
        MarcBuilder.writeDec(_.out_buf[0..2048], 12, 5, base_addr);
        _.out_buf[17] = ' ';
        _.out_buf[20] = '4';
        _.out_buf[21] = '5';
        _.out_buf[22] = '0';
        _.out_buf[23] = '0';

        let pos: usize = 24;
        i = 0;
        while (i < _.dir_len) {
            _.out_buf[pos] = _.dir_buf[i];
            pos += 1;
            i += 1;
        }
        _.out_buf[pos] = MARC_FIELD_TERMINATOR;
        pos += 1;

        i = 0;
        while (i < _.data_len) {
            _.out_buf[pos] = _.data_buf[i];
            pos += 1;
            i += 1;
        }
        _.out_buf[pos] = MARC_RECORD_TERMINATOR;
        pos += 1;

        _.out_len = pos;
        return _.out_buf[0.._.out_len];
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
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

test("marc: complete MARC 21 UTF-8 bibliographic record (001/008 + 020/100/245/264)") {
    let b = MarcBuilder {};
    b.addRawField("001", "ocn987654321");
    b.addRawField("008", "261121s2026    xxu           000 0 eng d");
    b.addRawField("020", "  \x1Fa9780131103627");
    b.addRawField("100", "1 \x1FaKernighan, Brian W.");
    b.addRawField("245", "14\x1FaThe C programming language /\x1Fbsecond edition /\x1FcBrian W. Kernighan, Dennis M. Ritchie.");
    b.addRawField("264", " 1\x1FaEnglewood Cliffs :\x1FbPrentice Hall,\x1Fc1988.");

    const rec = b.finish('a');
    expect_eq(MARC_LEADER_SIZE, 24);
    expect_eq(MARC_DIR_ENTRY_SIZE, 12);
    expect_eq(isMarcRecord(rec), true);

    if (ldr := parseMarcLeader(rec)) {
        expect_eq(ldr.record_length, rec.len);
        expect_eq(ldr.record_status, 'n');
        expect_eq(ldr.record_type, 'a');
        expect_eq(ldr.bib_level, 'm');
        expect_eq(ldr.isUtf8(), true);
        expect_eq(ldr.fieldCount(), 6);
    } else {
        expect_eq(true, false);
    }

    if (it_val := MarcFieldIter.init(rec)) {
        let it = it_val;
        let count: usize = 0;
        while (f := it.next()) {
            count += 1;
            if (f.isTag("001")) {
                expect_eq(f.isControlField(), true);
                expect_eq(f.ind1, ' ');
                expect_eq(f.ind2, ' ');
                expect_eq(strEq(f.data, "ocn987654321"), true);
            } else if (f.isTag("020")) {
                expect_eq(f.isControlField(), false);
                if (isbn := f.findSubfield('a')) {
                    expect_eq(strEq(isbn, "9780131103627"), true);
                } else {
                    expect_eq(true, false);
                }
            } else if (f.isTag("100")) {
                expect_eq(f.ind1, '1');
                expect_eq(f.ind2, ' ');
                if (author := f.findSubfield('a')) {
                    expect_eq(strEq(author, "Kernighan, Brian W."), true);
                } else {
                    expect_eq(true, false);
                }
            } else if (f.isTag("245")) {
                expect_eq(f.ind1, '1');
                expect_eq(f.ind2, '4');
                if (ta := f.findSubfield('a')) {
                    expect_eq(strEq(ta, "The C programming language /"), true);
                } else {
                    expect_eq(true, false);
                }
                if (tb := f.findSubfield('b')) {
                    expect_eq(strEq(tb, "second edition /"), true);
                } else {
                    expect_eq(true, false);
                }
                if (tc := f.findSubfield('c')) {
                    expect_eq(strEq(tc, "Brian W. Kernighan, Dennis M. Ritchie."), true);
                } else {
                    expect_eq(true, false);
                }
            }
        }
        expect_eq(count, 6);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("marc: control field subfields() empty & missing subfield returns null") {
    let b = MarcBuilder {};
    b.addRawField("001", "ctrl-001");
    b.addRawField("245", "00\x1FaOnly Title");
    const rec = b.finish('a');

    if (it_val := MarcFieldIter.init(rec)) {
        let it = it_val;
        if (f0 := it.next()) {
            expect_eq(f0.isControlField(), true);
            let sfit = f0.subfields();
            if (sf := sfit.next()) {
                * = sf;
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
        if (f1 := it.next()) {
            if (z := f1.findSubfield('z')) {
                * = z;
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("marc: MarcRecordIter multi-record concatenated .mrc stream (UTF-8 + MARC-8)") {
    let b1 = MarcBuilder {};
    b1.addRawField("001", "REC-1");
    b1.addRawField("245", "10\x1FaFirst Book");
    const r1 = b1.finish('a');

    let b2 = MarcBuilder {};
    b2.addRawField("001", "REC-2");
    b2.addRawField("245", "10\x1FaSecond Book");
    const r2 = b2.finish(' ');

    let stream: [1024]u8 = [0; 1024];
    let pos: usize = 0;
    let i: usize = 0;
    while (i < r1.len) {
        stream[pos] = r1[i];
        pos += 1;
        i += 1;
    }
    i = 0;
    while (i < r2.len) {
        stream[pos] = r2[i];
        pos += 1;
        i += 1;
    }

    let recs = MarcRecordIter.init(stream[0..pos]);
    let seen: usize = 0;
    while (rec := recs.next()) {
        seen += 1;
        if (seen == 1) {
            expect_eq(rec.leader.isUtf8(), true);
            let fit = rec.fields();
            if (f0 := fit.next()) {
                expect_eq(strEq(f0.data, "REC-1"), true);
            } else {
                expect_eq(true, false);
            }
        } else if (seen == 2) {
            expect_eq(rec.leader.isUtf8(), false);
            let fit = rec.fields();
            if (f0 := fit.next()) {
                expect_eq(strEq(f0.data, "REC-2"), true);
            } else {
                expect_eq(true, false);
            }
        }
    }
    expect_eq(seen, 2);
    expect_eq(recs.hasError(), false);
}

test("marc: MarcSubfieldIter enumerates all subfields in order") {
    const sf_raw = "\x1FaCity :\x1FbPublisher,\x1Fc2026.";
    let it = MarcSubfieldIter.init(sf_raw);
    let count: usize = 0;
    while (sf := it.next()) {
        count += 1;
        if (count == 1) {
            expect_eq(sf.code, 'a');
            expect_eq(strEq(sf.value, "City :"), true);
        } else if (count == 2) {
            expect_eq(sf.code, 'b');
            expect_eq(strEq(sf.value, "Publisher,"), true);
        } else if (count == 3) {
            expect_eq(sf.code, 'c');
            expect_eq(strEq(sf.value, "2026."), true);
        }
    }
    expect_eq(count, 3);
}

test("marc: non-12B directory, missing 0x1D/0x1E terminators & out-of-bounds field_len guards") {
    let b = MarcBuilder {};
    b.addRawField("001", "12345");
    const valid = b.finish('a');

    // Copy to mutable scratch buffer
    let buf: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < valid.len) {
        buf[i] = valid[i];
        i += 1;
    }
    const n = valid.len;

    // 1. Missing 0x1D record terminator
    buf[n - 1] = 0x00;
    expect_eq(isMarcRecord(buf[0..n]), false);
    if (l := parseMarcLeader(buf[0..n])) {
        * = l;
        expect_eq(true, false);
    }
    buf[n - 1] = MARC_RECORD_TERMINATOR;

    // 2. Missing 0x1E directory terminator (at base_addr - 1 = 36)
    buf[36] = ' ';
    if (l := parseMarcLeader(buf[0..n])) {
        * = l;
        expect_eq(true, false);
    }
    buf[36] = MARC_FIELD_TERMINATOR;

    // 3. Directory size not a multiple of 12 (change base_addr from 00037 to 00038)
    buf[16] = '8';
    buf[37] = MARC_FIELD_TERMINATOR;
    if (l := parseMarcLeader(buf[0..n])) {
        * = l;
        expect_eq(true, false);
    }
    buf[16] = '7';
    buf[37] = valid[37];

    // 4. Out-of-bounds field_len in directory entry (set 4B field_len at offset 27..31 to "9999")
    buf[27] = '9';
    buf[28] = '9';
    buf[29] = '9';
    buf[30] = '9';
    if (it_val := MarcFieldIter.init(buf[0..n])) {
        let it = it_val;
        if (f := it.next()) {
            * = f;
            expect_eq(true, false);
        }
        expect_eq(it.hasError(), true);
    } else {
        expect_eq(true, false);
    }
}
