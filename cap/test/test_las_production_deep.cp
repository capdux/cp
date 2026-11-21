// Source: cp/cap/test/test_las_production_deep.cp (Production deep tests for cap:las)

import { test, expect_eq } from "cap:test";
import {
    LAS_MAGIC,
    LAS_HEADER_SIZE_1_2,
    LAS_HEADER_SIZE_1_3,
    LAS_HEADER_SIZE_1_4,
    LAS_VLR_HEADER_SIZE,
    LasHeader,
    LasVlr,
    LasPoint,
    isLas,
    parseLasHeader,
    LasVlrIter,
    LasPointIter,
} from "cap:las";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
}

fn writeBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn writeU16LE(dst: []u8, off: usize, val: u16) {
    dst[off + 0] = (val & 0xFF) as u8;
    dst[off + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(dst: []u8, off: usize, val: u32) {
    dst[off + 0] = (val & 0xFF) as u8;
    dst[off + 1] = ((val >> 8) & 0xFF) as u8;
    dst[off + 2] = ((val >> 16) & 0xFF) as u8;
    dst[off + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeI32LE(dst: []u8, off: usize, val: i32) {
    const bits: u32 = @bitCast(val);
    writeU32LE(dst, off, bits);
}

fn writeU64LE(dst: []u8, off: usize, val: u64) {
    writeU32LE(dst, off, (val & 0xFFFFFFFF) as u32);
    writeU32LE(dst, off + 4, ((val >> 32) & 0xFFFFFFFF) as u32);
}

fn writeF64LE(dst: []u8, off: usize, val: f64) {
    const bits: u64 = @bitCast(val);
    writeU64LE(dst, off, bits);
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

fn initLasHeader(
    buf: []u8,
    minor: u8,
    header_size: u16,
    point_off: u32,
    num_vlrs: u32,
    raw_fmt: u8,
    rec_len: u16,
    count: u64,
) {
    writeBytes(buf, 0, LAS_MAGIC);
    buf[24] = 1;
    buf[25] = minor;
    writeBytes(buf, 26, "LIDAR-SCANNER");
    writeBytes(buf, 58, "cap-las-gen");
    writeU16LE(buf, 94, header_size);
    writeU32LE(buf, 96, point_off);
    writeU32LE(buf, 100, num_vlrs);
    buf[104] = raw_fmt;
    writeU16LE(buf, 105, rec_len);
    if (minor >= 4) {
        writeU32LE(buf, 107, 0);
        writeU64LE(buf, 247, count);
    } else {
        writeU32LE(buf, 107, count as u32);
    }
    // Scale 0.01, offset (100.0, 200.0, 10.0)
    writeF64LE(buf, 131, 0.01);
    writeF64LE(buf, 139, 0.01);
    writeF64LE(buf, 147, 0.01);
    writeF64LE(buf, 155, 100.0);
    writeF64LE(buf, 163, 200.0);
    writeF64LE(buf, 171, 10.0);
    // Bounding box
    writeF64LE(buf, 179, 150.0); // x_max
    writeF64LE(buf, 187, 90.0); // x_min
    writeF64LE(buf, 195, 250.0); // y_max
    writeF64LE(buf, 203, 190.0); // y_min
    writeF64LE(buf, 211, 30.0); // z_max
    writeF64LE(buf, 219, 5.0); // z_min
}

test("1. parses LAS 1.2 227B header, VLR record, and Format 0 points with scale/offset conversion", () => {
    // 227B header + 1 VLR (54 + 6 = 60B) = 287B point_off + 2 points * 20B = 327B
    let raw: [360]u8 = undefined;
    let buf = raw[0..327];
    zeroBytes(buf);

    initLasHeader(buf, 2, 227, 287, 1, 0, 20, 2);

    // VLR at offset 227
    writeBytes(buf, 227 + 2, "LASF_Projection");
    writeU16LE(buf, 227 + 18, 34735); // GeoKeyDirectoryTag
    writeU16LE(buf, 227 + 20, 6);
    writeBytes(buf, 227 + 22, "WGS84 UTM");
    writeBytes(buf, 227 + 54, "GEOKEY");

    // Point 0 at 287: raw=(1000, -500, 250) => (110.0, 195.0, 12.5)
    writeI32LE(buf, 287, 1000);
    writeI32LE(buf, 287 + 4, -500);
    writeI32LE(buf, 287 + 8, 250);
    writeU16LE(buf, 287 + 12, 4096); // intensity
    buf[287 + 14] = (2 | (3 << 3)) as u8; // return 2 of 3
    buf[287 + 15] = 2; // Ground classification

    // Point 1 at 307: raw=(0, 0, 500) => (100.0, 200.0, 15.0)
    writeI32LE(buf, 307, 0);
    writeI32LE(buf, 307 + 4, 0);
    writeI32LE(buf, 307 + 8, 500);
    writeU16LE(buf, 307 + 12, 512);
    buf[307 + 14] = (1 | (1 << 3)) as u8;
    buf[307 + 15] = 6; // Building

    expect_eq(LAS_HEADER_SIZE_1_2, 227);
    expect_eq(LAS_VLR_HEADER_SIZE, 54);
    expect_eq(isLas(buf), true);

    if (hdr := parseLasHeader(buf)) {
        expect_eq(hdr.version_major, 1);
        expect_eq(hdr.version_minor, 2);
        expect_eq(sliceEq(hdr.system_id, "LIDAR-SCANNER"), true);
        expect_eq(sliceEq(hdr.generating_software, "cap-las-gen"), true);
        expect_eq(hdr.point_data_format, 0);
        expect_eq(hdr.is_laz_compressed, false);
        expect_eq(hdr.point_count, 2);
        expect_eq(hdr.x_max > 149.9 && hdr.x_min < 90.1, true);

        let vlrs = LasVlrIter.init(buf, hdr);
        if (v := vlrs.next()) {
            expect_eq(sliceEq(v.user_id, "LASF_Projection"), true);
            expect_eq(v.record_id, 34735);
            expect_eq(sliceEq(v.description, "WGS84 UTM"), true);
            expect_eq(sliceEq(v.payload, "GEOKEY"), true);
        } else { expect_eq(true, false); }
        expect_eq(vlrs.next() == null, true);
        expect_eq(vlrs.had_error, false);

        let pts = LasPointIter.init(buf, hdr);
        if (p0 := pts.next()) {
            expect_eq(p0.x > 109.99 && p0.x < 110.01, true);
            expect_eq(p0.y > 194.99 && p0.y < 195.01, true);
            expect_eq(p0.z > 12.49 && p0.z < 12.51, true);
            expect_eq(p0.intensity, 4096);
            expect_eq(p0.return_number, 2);
            expect_eq(p0.num_returns, 3);
            expect_eq(p0.classification, 2);
        } else { expect_eq(true, false); }
        if (p1 := pts.next()) {
            expect_eq(p1.z > 14.99 && p1.z < 15.01, true);
            expect_eq(p1.classification, 6);
        } else { expect_eq(true, false); }
        expect_eq(pts.next() == null, true);
        expect_eq(pts.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("2. parses LAS 1.4 375B header with 64-bit extended_point_count and Format 6 point decoding", () => {
    // 375B header + 1 Format 6 point (30B) = 405B
    let raw: [420]u8 = undefined;
    let buf = raw[0..405];
    zeroBytes(buf);

    initLasHeader(buf, 4, 375, 375, 0, 6, 30, 1);

    // Format 6 point at offset 375
    writeI32LE(buf, 375, 500); // x = 105.0
    writeI32LE(buf, 375 + 4, 1000); // y = 210.0
    writeI32LE(buf, 375 + 8, 2000); // z = 30.0
    writeU16LE(buf, 375 + 12, 8192);
    buf[375 + 14] = (5 | (12 << 4)) as u8; // 4-bit return 5 of 12
    buf[375 + 16] = 64; // Extended ASPRS classification 64

    expect_eq(LAS_HEADER_SIZE_1_4, 375);
    if (hdr := parseLasHeader(buf)) {
        expect_eq(hdr.version_minor, 4);
        expect_eq(hdr.point_data_format, 6);
        expect_eq(hdr.point_count, 1);

        let pts = LasPointIter.init(buf, hdr);
        if (pt := pts.next()) {
            expect_eq(pt.x > 104.99 && pt.x < 105.01, true);
            expect_eq(pt.return_number, 5);
            expect_eq(pt.num_returns, 12);
            expect_eq(pt.classification, 64);
        } else { expect_eq(true, false); }
    } else {
        expect_eq(true, false);
    }
});

test("3. detects LAZ compression bit (0x80) and suppresses raw LasPointIter decoding", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..247];
    zeroBytes(buf);

    // Format 3 with LAZ bit 0x80 => 0x83
    initLasHeader(buf, 2, 227, 227, 0, 0x83, 20, 1);

    if (hdr := parseLasHeader(buf)) {
        expect_eq(hdr.is_laz_compressed, true);
        expect_eq(hdr.point_data_format, 3);

        let pts = LasPointIter.init(buf, hdr);
        expect_eq(pts.next() == null, true);
        expect_eq(pts.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("4. guards VLR iteration against crossing offset_to_point_data boundary", () => {
    // 227B header + only 54B space before point_off=281, but VLR claims 100B payload
    let raw: [320]u8 = undefined;
    let buf = raw[0..301];
    zeroBytes(buf);

    initLasHeader(buf, 2, 227, 281, 1, 0, 20, 1);
    writeU16LE(buf, 227 + 20, 100); // record_len=100 exceeds limit 281

    if (hdr := parseLasHeader(buf)) {
        let vlrs = LasVlrIter.init(buf, hdr);
        expect_eq(vlrs.next() == null, true);
        expect_eq(vlrs.had_error, true);
    } else {
        expect_eq(true, false);
    }
});

test("5. rejects bad LASF magic, truncated header, invalid point_record_len, and out-of-bounds point_off", () => {
    let tiny: [64]u8 = undefined;
    zeroBytes(tiny[0..64]);
    expect_eq(isLas(tiny[0..64]), false);
    expect_eq(parseLasHeader(tiny[0..64]) == null, true);

    let raw: [240]u8 = undefined;
    let buf = raw[0..240];
    zeroBytes(buf);
    initLasHeader(buf, 2, 227, 227, 0, 0, 20, 0);
    expect_eq(parseLasHeader(buf) != null, true);

    // Bad magic
    buf[0] = 'X';
    expect_eq(isLas(buf), false);
    buf[0] = 'L';

    // point_record_len < 20
    writeU16LE(buf, 105, 12);
    expect_eq(parseLasHeader(buf) == null, true);
    writeU16LE(buf, 105, 20);

    // offset_to_point_data > buffer len
    writeU32LE(buf, 96, 9999);
    expect_eq(parseLasHeader(buf) == null, true);

    // LAS 1.4 with header_size < 375
    writeU32LE(buf, 96, 227);
    buf[25] = 4;
    expect_eq(parseLasHeader(buf) == null, true);
});
