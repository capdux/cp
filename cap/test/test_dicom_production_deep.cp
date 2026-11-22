// Source: cp/cap/test/test_dicom_production_deep.cp
// Deep Production Test Suite for cap:dicom
// Covers:
//   1. Explicit VR Little Endian CT/MR DICOM (128B preamble + "DICM" + Group 0002 + short/long VR)
//   2. Implicit VR Little Endian (1.2.840.10008.1.2) automatic mode switch after Group 0002
//   3. Explicit VR Big Endian (1.2.840.10008.1.2.2) automatic big-endian switch after Group 0002
//   4. 0xFFFFFFFF undefined-length SQ (defined & undefined items) and encapsulated PixelData delimitation
//   5. Invalid "DICM" magic, short buffer, truncated element length & unterminated SQ guards

import { test, expect_eq } from "cap:test";
import {
    DICOM_PREAMBLE_SIZE,
    DICOM_MAGIC,
    DICOM_DATA_OFFSET,
    DICOM_UNDEFINED_LEN,
    TS_IMPLICIT_VR_LE,
    TS_EXPLICIT_VR_LE,
    TS_EXPLICIT_VR_BE,
    DicomTag,
    TAG_TRANSFER_SYNTAX,
    TAG_PATIENT_NAME,
    TAG_PATIENT_ID,
    TAG_MODALITY,
    TAG_ROWS,
    TAG_COLUMNS,
    TAG_BITS_ALLOCATED,
    TAG_PIXEL_DATA,
    DicomElement,
    DicomIter,
    DicomSummary,
    isDicom,
    parseDicomSummary,
} from "cap:image";

struct DicomBuilder {
    buf: [2048]u8 = [0; 2048],
    len: usize = 0,

    fn writeU8(*_, v: u8) {
        _.buf[_.len] = v;
        _.len += 1;
    }

    fn writeBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeU8(s[i]);
            i += 1;
        }
    }

    fn writeU16(*_, v: u16, is_be: bool) {
        if (is_be) {
            _.writeU8(((v >> 8) & 0xFF) as u8);
            _.writeU8((v & 0xFF) as u8);
        } else {
            _.writeU8((v & 0xFF) as u8);
            _.writeU8(((v >> 8) & 0xFF) as u8);
        }
    }

    fn writeU32(*_, v: u32, is_be: bool) {
        if (is_be) {
            _.writeU8(((v >> 24) & 0xFF) as u8);
            _.writeU8(((v >> 16) & 0xFF) as u8);
            _.writeU8(((v >> 8) & 0xFF) as u8);
            _.writeU8((v & 0xFF) as u8);
        } else {
            _.writeU8((v & 0xFF) as u8);
            _.writeU8(((v >> 8) & 0xFF) as u8);
            _.writeU8(((v >> 16) & 0xFF) as u8);
            _.writeU8(((v >> 24) & 0xFF) as u8);
        }
    }

    fn writePreambleAndMagic(*_) {
        let i: usize = 0;
        while (i < DICOM_PREAMBLE_SIZE) {
            _.writeU8(0);
            i += 1;
        }
        _.writeBytes("DICM");
    }

    fn writeExplicitShort(*_, g: u16, e: u16, vr: []const u8, payload: []const u8, is_be: bool) {
        _.writeU16(g, is_be);
        _.writeU16(e, is_be);
        _.writeU8(vr[0]);
        _.writeU8(vr[1]);
        _.writeU16(payload.len as u16, is_be);
        _.writeBytes(payload);
    }

    fn writeExplicitLong(*_, g: u16, e: u16, vr: []const u8, payload: []const u8, is_be: bool) {
        _.writeU16(g, is_be);
        _.writeU16(e, is_be);
        _.writeU8(vr[0]);
        _.writeU8(vr[1]);
        _.writeU16(0, is_be);
        _.writeU32(payload.len as u32, is_be);
        _.writeBytes(payload);
    }

    fn writeExplicitU16(*_, g: u16, e: u16, vr: []const u8, val: u16, is_be: bool) {
        _.writeU16(g, is_be);
        _.writeU16(e, is_be);
        _.writeU8(vr[0]);
        _.writeU8(vr[1]);
        _.writeU16(2, is_be);
        _.writeU16(val, is_be);
    }

    fn writeExplicitU32(*_, g: u16, e: u16, vr: []const u8, val: u32, is_be: bool) {
        _.writeU16(g, is_be);
        _.writeU16(e, is_be);
        _.writeU8(vr[0]);
        _.writeU8(vr[1]);
        _.writeU16(4, is_be);
        _.writeU32(val, is_be);
    }

    fn writeImplicitBytes(*_, g: u16, e: u16, payload: []const u8) {
        _.writeU16(g, false);
        _.writeU16(e, false);
        _.writeU32(payload.len as u32, false);
        _.writeBytes(payload);
    }

    fn writeImplicitU16(*_, g: u16, e: u16, val: u16) {
        _.writeU16(g, false);
        _.writeU16(e, false);
        _.writeU32(2, false);
        _.writeU16(val, false);
    }

    fn writeDelimTag(*_, e: u16, len: u32, is_be: bool) {
        _.writeU16(0xFFFE, is_be);
        _.writeU16(e, is_be);
        _.writeU32(len, is_be);
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
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

test("dicom: Explicit VR Little Endian CT file with short & long VRs") {
    let b = DicomBuilder {};
    b.writePreambleAndMagic();

    // Group 0x0002 File Meta Information (Explicit VR LE)
    b.writeExplicitU32(0x0002, 0x0000, "UL", 44, false);
    // Long VR: OB (File Meta Information Version)
    b.writeExplicitLong(0x0002, 0x0001, "OB", "\x00\x01", false);
    // Transfer Syntax UID with trailing NUL padding (even length 20)
    b.writeExplicitShort(0x0002, 0x0010, "UI", "1.2.840.10008.1.2.1\x00", false);

    // Dataset (Explicit VR LE)
    b.writeExplicitShort(0x0008, 0x0060, "CS", "CT  ", false);
    b.writeExplicitShort(0x0010, 0x0010, "PN", "DOE^JANE  ", false);
    b.writeExplicitShort(0x0010, 0x0020, "LO", "PT-0042 ", false);
    b.writeExplicitU16(0x0028, 0x0010, "US", 512, false);
    b.writeExplicitU16(0x0028, 0x0011, "US", 512, false);
    b.writeExplicitU16(0x0028, 0x0100, "US", 16, false);
    // Long VR: OW PixelData (8 bytes)
    b.writeExplicitLong(0x7FE0, 0x0010, "OW", "\x10\x00\x20\x00\x30\x00\x40\x00", false);

    const data = b.slice();
    expect_eq(isDicom(data), true);
    expect_eq(DICOM_DATA_OFFSET, 132);
    expect_eq(strEq(DICOM_MAGIC, "DICM"), true);

    if (summary := parseDicomSummary(data)) {
        expect_eq(strEq(summary.transfer_syntax, TS_EXPLICIT_VR_LE), true);
        expect_eq(strEq(summary.modality, "CT"), true);
        expect_eq(strEq(summary.patient_name, "DOE^JANE"), true);
        expect_eq(strEq(summary.patient_id, "PT-0042"), true);
        expect_eq(summary.rows, 512);
        expect_eq(summary.columns, 512);
        expect_eq(summary.bits_allocated, 16);
        expect_eq(summary.has_pixel_data, true);
        expect_eq(summary.element_count, 10);
    } else {
        expect_eq(true, false);
    }

    // Verify DicomIter element-level details (asU32, long VR, isTag)
    if (it_val := DicomIter.init(data)) {
        let it = it_val;
        if (first := it.next()) {
            expect_eq(first.isTag(0x0002, 0x0000), true);
            expect_eq(first.vr[0], 'U');
            expect_eq(first.vr[1], 'L');
            if (v := first.asU32(false)) {
                expect_eq(v, 44);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        if (second := it.next()) {
            expect_eq(second.isTag(0x0002, 0x0001), true);
            expect_eq(second.vr[0], 'O');
            expect_eq(second.vr[1], 'B');
            expect_eq(second.value.len, 2);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("dicom: Implicit VR Little Endian (1.2.840.10008.1.2) auto-switch after Group 0002") {
    let b = DicomBuilder {};
    b.writePreambleAndMagic();

    // Group 0x0002 is ALWAYS Explicit VR LE
    b.writeExplicitShort(0x0002, 0x0010, "UI", "1.2.840.10008.1.2\x00", false);

    // Group > 0x0002 switches automatically to Implicit VR LE (no VR bytes, 4-byte length)
    b.writeImplicitBytes(0x0008, 0x0060, "MR");
    b.writeImplicitBytes(0x0010, 0x0010, "SMITH^JOHN ");
    b.writeImplicitBytes(0x0010, 0x0020, "MR-9001 ");
    b.writeImplicitU16(0x0028, 0x0010, 256);
    b.writeImplicitU16(0x0028, 0x0011, 320);
    b.writeImplicitU16(0x0028, 0x0100, 12);
    b.writeImplicitBytes(0x7FE0, 0x0010, "\x01\x02\x03\x04");

    const data = b.slice();
    if (summary := parseDicomSummary(data)) {
        expect_eq(strEq(summary.transfer_syntax, TS_IMPLICIT_VR_LE), true);
        expect_eq(strEq(summary.modality, "MR"), true);
        expect_eq(strEq(summary.patient_name, "SMITH^JOHN"), true);
        expect_eq(strEq(summary.patient_id, "MR-9001"), true);
        expect_eq(summary.rows, 256);
        expect_eq(summary.columns, 320);
        expect_eq(summary.bits_allocated, 12);
        expect_eq(summary.has_pixel_data, true);
        expect_eq(summary.element_count, 8);
    } else {
        expect_eq(true, false);
    }

    // Verify Implicit VR placeholder is "  "
    if (it_val := DicomIter.init(data)) {
        let it = it_val;
        if (meta_el := it.next()) {
            expect_eq(meta_el.vr[0], 'U');
            expect_eq(meta_el.vr[1], 'I');
        } else {
            expect_eq(true, false);
        }
        expect_eq(it.isExplicitVr(), false);
        expect_eq(it.isBigEndian(), false);
        if (mod_el := it.next()) {
            expect_eq(mod_el.matchesTag(TAG_MODALITY), true);
            expect_eq(mod_el.vr[0], ' ');
            expect_eq(mod_el.vr[1], ' ');
            expect_eq(strEq(mod_el.asTrimmedString(), "MR"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("dicom: Explicit VR Big Endian (1.2.840.10008.1.2.2) auto-switch after Group 0002") {
    let b = DicomBuilder {};
    b.writePreambleAndMagic();

    // Group 0x0002 is ALWAYS Explicit VR LE
    b.writeExplicitShort(0x0002, 0x0010, "UI", "1.2.840.10008.1.2.2\x00", false);

    // Group > 0x0002 switches automatically to Explicit VR Big Endian
    b.writeExplicitShort(0x0008, 0x0060, "CS", "US", true);
    b.writeExplicitShort(0x0010, 0x0010, "PN", "WANG^LI ", true);
    b.writeExplicitShort(0x0010, 0x0020, "LO", "BE-77 ", true);
    b.writeExplicitU16(0x0028, 0x0010, "US", 1024, true);
    b.writeExplicitU16(0x0028, 0x0011, "US", 768, true);
    b.writeExplicitU16(0x0028, 0x0100, "US", 8, true);
    b.writeExplicitLong(0x7FE0, 0x0010, "OB", "\xAA\xBB\xCC\xDD", true);

    const data = b.slice();
    if (summary := parseDicomSummary(data)) {
        expect_eq(strEq(summary.transfer_syntax, TS_EXPLICIT_VR_BE), true);
        expect_eq(strEq(summary.modality, "US"), true);
        expect_eq(strEq(summary.patient_name, "WANG^LI"), true);
        expect_eq(strEq(summary.patient_id, "BE-77"), true);
        expect_eq(summary.rows, 1024);
        expect_eq(summary.columns, 768);
        expect_eq(summary.bits_allocated, 8);
        expect_eq(summary.has_pixel_data, true);
        expect_eq(summary.element_count, 8);
    } else {
        expect_eq(true, false);
    }

    if (it_val := DicomIter.init(data)) {
        let it = it_val;
        * = it.next(); // Transfer Syntax
        expect_eq(it.isExplicitVr(), true);
        expect_eq(it.isBigEndian(), true);
    } else {
        expect_eq(true, false);
    }
}

test("dicom: 0xFFFFFFFF undefined-length SQ and encapsulated PixelData delimitation") {
    let b = DicomBuilder {};
    b.writePreambleAndMagic();

    // Group 0x0002: Explicit VR LE
    b.writeExplicitShort(0x0002, 0x0010, "UI", "1.2.840.10008.1.2.1\x00", false);

    // Undefined-length SQ (0x0008, 0x1115) with both defined-length & undefined-length items
    b.writeU16(0x0008, false);
    b.writeU16(0x1115, false);
    b.writeBytes("SQ");
    b.writeU16(0, false);
    b.writeU32(DICOM_UNDEFINED_LEN, false);

    // Item 1: defined length = 10 bytes (a short CS element inside)
    b.writeDelimTag(0xE000, 10, false);
    b.writeExplicitShort(0x0008, 0x0060, "CS", "XX", false);

    // Item 2: undefined length containing an inner element + Item Delimitation (FFFE,E00D)
    b.writeDelimTag(0xE000, DICOM_UNDEFINED_LEN, false);
    b.writeExplicitShort(0x0010, 0x0020, "LO", "IN", false);
    b.writeDelimTag(0xE00D, 0, false);

    // Sequence Delimitation Item (FFFE,E0DD)
    b.writeDelimTag(0xE0DD, 0, false);

    // Elements AFTER the SQ must be parsed accurately!
    b.writeExplicitShort(0x0008, 0x0060, "CS", "CR", false);
    b.writeExplicitU16(0x0028, 0x0010, "US", 640, false);
    b.writeExplicitU16(0x0028, 0x0011, "US", 480, false);

    // Encapsulated PixelData (0x7FE0, 0x0010) OB with 0xFFFFFFFF length
    b.writeU16(0x7FE0, false);
    b.writeU16(0x0010, false);
    b.writeBytes("OB");
    b.writeU16(0, false);
    b.writeU32(DICOM_UNDEFINED_LEN, false);
    // Basic Offset Table (empty item)
    b.writeDelimTag(0xE000, 0, false);
    // Compressed frame fragment (6 bytes)
    b.writeDelimTag(0xE000, 6, false);
    b.writeBytes("\xFF\xD8\xFF\xE0\xFF\xD9");
    // Sequence Delimitation Item (FFFE,E0DD)
    b.writeDelimTag(0xE0DD, 0, false);

    const data = b.slice();
    if (summary := parseDicomSummary(data)) {
        expect_eq(strEq(summary.modality, "CR"), true);
        expect_eq(summary.rows, 640);
        expect_eq(summary.columns, 480);
        expect_eq(summary.has_pixel_data, true);
        expect_eq(summary.element_count, 6);
    } else {
        expect_eq(true, false);
    }

    // Verify is_undefined_len flag on SQ and encapsulated PixelData
    if (it_val := DicomIter.init(data)) {
        let it = it_val;
        * = it.next(); // (0002,0010)
        if (sq := it.next()) {
            expect_eq(sq.isTag(0x0008, 0x1115), true);
            expect_eq(sq.is_undefined_len, true);
            expect_eq(sq.vr[0], 'S');
            expect_eq(sq.vr[1], 'Q');
            expect_eq(sq.value.len > 0, true);
        } else {
            expect_eq(true, false);
        }
        * = it.next(); // Modality
        * = it.next(); // Rows
        * = it.next(); // Columns
        if (px := it.next()) {
            expect_eq(px.matchesTag(TAG_PIXEL_DATA), true);
            expect_eq(px.is_undefined_len, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("dicom: invalid magic, short buffer, truncated element & unterminated SQ guards") {
    // 1. Buffer shorter than 132 bytes
    expect_eq(isDicom("DICM"), false);
    if (s0 := parseDicomSummary("DICM")) {
        * = s0;
        expect_eq(true, false);
    }

    // 2. 132+ bytes but wrong magic ("DICO")
    let bad_magic = DicomBuilder {};
    let i: usize = 0;
    while (i < 128) {
        bad_magic.writeU8(0);
        i += 1;
    }
    bad_magic.writeBytes("DICO");
    expect_eq(isDicom(bad_magic.slice()), false);
    if (it0 := DicomIter.init(bad_magic.slice())) {
        * = it0;
        expect_eq(true, false);
    }

    // 3. Truncated element value length (claims 100 bytes, only 4 available)
    let trunc = DicomBuilder {};
    trunc.writePreambleAndMagic();
    trunc.writeU16(0x0008, false);
    trunc.writeU16(0x0060, false);
    trunc.writeBytes("CS");
    trunc.writeU16(100, false);
    trunc.writeBytes("CT  ");
    expect_eq(isDicom(trunc.slice()), true);
    if (s1 := parseDicomSummary(trunc.slice())) {
        * = s1;
        expect_eq(true, false);
    }
    if (it_val := DicomIter.init(trunc.slice())) {
        let it = it_val;
        if (el0 := it.next()) {
            * = el0;
            expect_eq(true, false);
        }
        expect_eq(it.hasError(), true);
    } else {
        expect_eq(true, false);
    }

    // 4. Unterminated 0xFFFFFFFF SQ without (0xFFFE, 0xE0DD)
    let unterm = DicomBuilder {};
    unterm.writePreambleAndMagic();
    unterm.writeU16(0x0008, false);
    unterm.writeU16(0x1115, false);
    unterm.writeBytes("SQ");
    unterm.writeU16(0, false);
    unterm.writeU32(DICOM_UNDEFINED_LEN, false);
    unterm.writeDelimTag(0xE000, 2, false);
    unterm.writeBytes("AB");
    if (s2 := parseDicomSummary(unterm.slice())) {
        * = s2;
        expect_eq(true, false);
    }
}
