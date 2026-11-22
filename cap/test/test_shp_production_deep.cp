// Source: cp/cap/test/test_shp_production_deep.cp (Production deep tests for cap:shp)

import { test, expect_eq } from "cap:test";
import {
    SHP_FILE_CODE,
    SHP_VERSION,
    SHP_HEADER_SIZE,
    SHP_RECORD_HEADER_SIZE,
    ShpType,
    ShpBBox,
    ShpHeader,
    ShpRecord,
    ShpPoint,
    ShpPolyHeader,
    ShpRecordIter,
    parseShpHeader,
    isShp,
    parseShpPoint,
    parseShpPoly,
} from "cap:binfmt";

fn putU32BE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putF64LE(buf: *[]u8, off: usize, v: f64) {
    const bits: u64 = @as(u64, @bitCast(v));
    putU32LE(buf, off, (bits & 0xFFFFFFFF) as u32);
    putU32LE(buf, off + 4, ((bits >> 32) & 0xFFFFFFFF) as u32);
}

fn writeShpHeader(buf: *[]u8, total_bytes: usize, shape_type: u32, xmin: f64, ymin: f64, xmax: f64, ymax: f64) {
    putU32BE(buf, 0, SHP_FILE_CODE);
    putU32BE(buf, 24, (total_bytes / 2) as u32); // 16-bit words
    putU32LE(buf, 28, SHP_VERSION);
    putU32LE(buf, 32, shape_type);
    putF64LE(buf, 36, xmin);
    putF64LE(buf, 44, ymin);
    putF64LE(buf, 52, xmax);
    putF64LE(buf, 60, ymax);
}

test("1. 100B mixed-endian header parsing, 16-bit words length check, and Bounding Box", () => {
    let buf: [100]u8 = [0; 100];
    let b = buf[0..100];
    writeShpHeader(*b, 100, 1, -122.5, 37.25, -121.0, 38.75);
    putF64LE(*b, 68, 10.0); // z_min
    putF64LE(*b, 76, 500.0); // z_max

    expect_eq(isShp(b), true);
    const hdr = parseShpHeader(b) ?? unreachable;
    expect_eq(hdr.file_code, 9994);
    expect_eq(hdr.file_length_words, 50);
    expect_eq(hdr.file_length_bytes, 100);
    expect_eq(hdr.version, 1000);
    expect_eq(hdr.shape_type == ShpType.point, true);
    expect_eq(hdr.bbox.x_min, -122.5);
    expect_eq(hdr.bbox.y_min, 37.25);
    expect_eq(hdr.bbox.x_max, -121.0);
    expect_eq(hdr.bbox.y_max, 38.75);
    expect_eq(hdr.bbox.z_min, 10.0);
    expect_eq(hdr.bbox.z_max, 500.0);
});

test("2. ShpRecordIter and parseShpPoint iterate multiple Point records accurately", () => {
    // 100B header + 2 * (8B rec hdr + 20B point) = 156 bytes (78 words)
    let buf: [156]u8 = [0; 156];
    let b = buf[0..156];
    writeShpHeader(*b, 156, 1, 1.0, 2.0, 3.0, 4.0);

    // Record 1 at offset 100
    putU32BE(*b, 100, 1);
    putU32BE(*b, 104, 10); // 10 words = 20 bytes
    putU32LE(*b, 108, 1); // point
    putF64LE(*b, 112, 1.5);
    putF64LE(*b, 120, 2.5);

    // Record 2 at offset 128
    putU32BE(*b, 128, 2);
    putU32BE(*b, 132, 10);
    putU32LE(*b, 136, 1);
    putF64LE(*b, 140, 3.25);
    putF64LE(*b, 148, 4.75);

    let it = ShpRecordIter.init(b);
    const r0 = it.next() ?? unreachable;
    expect_eq(r0.record_number, 1);
    expect_eq(r0.content_length_bytes, 20);
    expect_eq(r0.shape_type == ShpType.point, true);
    const p0 = parseShpPoint(r0.content) ?? unreachable;
    expect_eq(p0.x, 1.5);
    expect_eq(p0.y, 2.5);

    const r1 = it.next() ?? unreachable;
    expect_eq(r1.record_number, 2);
    const p1 = parseShpPoint(r1.content) ?? unreachable;
    expect_eq(p1.x, 3.25);
    expect_eq(p1.y, 4.75);

    expect_eq(it.next() == null, true);
});

test("3. parseShpPoly zero-copy random access to Polygon parts and points", () => {
    // Polygon with 1 part (offset 0) and 3 points => 44 + 4 + 48 = 96 bytes (48 words)
    // Total file = 100 + 8 + 96 = 204 bytes
    let buf: [204]u8 = [0; 204];
    let b = buf[0..204];
    writeShpHeader(*b, 204, 5, 0.0, 0.0, 10.0, 10.0);

    putU32BE(*b, 100, 1);
    putU32BE(*b, 104, 48); // 48 words = 96 bytes

    // Polygon content starts at 108
    putU32LE(*b, 108, 5); // polygon
    putF64LE(*b, 112, 0.0);
    putF64LE(*b, 120, 0.0);
    putF64LE(*b, 128, 10.0);
    putF64LE(*b, 136, 10.0);
    putU32LE(*b, 144, 1); // num_parts = 1
    putU32LE(*b, 148, 3); // num_points = 3
    putU32LE(*b, 152, 0); // part[0] = 0
    // 3 points starting at 156
    putF64LE(*b, 156, 0.0);
    putF64LE(*b, 164, 0.0);
    putF64LE(*b, 172, 10.0);
    putF64LE(*b, 180, 0.0);
    putF64LE(*b, 188, 5.0);
    putF64LE(*b, 196, 10.0);

    let it = ShpRecordIter.init(b);
    const rec = it.next() ?? unreachable;
    expect_eq(rec.shape_type == ShpType.polygon, true);

    const poly = parseShpPoly(rec.content) ?? unreachable;
    expect_eq(poly.num_parts, 1);
    expect_eq(poly.num_points, 3);
    expect_eq(poly.partAt(rec.content, 0) ?? 99, 0);
    expect_eq(poly.partAt(rec.content, 1) == null, true);

    const v0 = poly.pointAt(rec.content, 0) ?? unreachable;
    expect_eq(v0.x, 0.0);
    expect_eq(v0.y, 0.0);
    const v2 = poly.pointAt(rec.content, 2) ?? unreachable;
    expect_eq(v2.x, 5.0);
    expect_eq(v2.y, 10.0);
    expect_eq(poly.pointAt(rec.content, 3) == null, true);
});

test("4. NullShape (type=0) records are parsed cleanly alongside geometry records", () => {
    // 100B header + (8B + 4B NullShape) = 112 bytes
    let buf: [112]u8 = [0; 112];
    let b = buf[0..112];
    writeShpHeader(*b, 112, 1, 0.0, 0.0, 0.0, 0.0);
    putU32BE(*b, 100, 1);
    putU32BE(*b, 104, 2); // 2 words = 4 bytes
    putU32LE(*b, 108, 0); // null_shape

    let it = ShpRecordIter.init(b);
    const rec = it.next() ?? unreachable;
    expect_eq(rec.record_number, 1);
    expect_eq(rec.shape_type == ShpType.null_shape, true);
    expect_eq(rec.content_length_bytes, 4);
    expect_eq(parseShpPoint(rec.content) == null, true);
    expect_eq(it.next() == null, true);
});

test("5. wrong magic 9994, bad version, oversized file_length, and out-of-bounds content_words fail safely", () => {
    const short_buf: [32]u8 = [0; 32];
    expect_eq(isShp(short_buf[0..32]), false);
    expect_eq(parseShpHeader(short_buf[0..32]) == null, true);

    let buf: [120]u8 = [0; 120];
    let b = buf[0..120];
    writeShpHeader(*b, 120, 1, 0.0, 0.0, 0.0, 0.0);

    // Wrong file code
    putU32BE(*b, 0, 9995);
    expect_eq(isShp(b), false);
    putU32BE(*b, 0, SHP_FILE_CODE);

    // Wrong version
    putU32LE(*b, 28, 2000);
    expect_eq(isShp(b), false);
    putU32LE(*b, 28, SHP_VERSION);

    // File length exceeds actual buffer (1000 words = 2000 bytes > 120)
    putU32BE(*b, 24, 1000);
    expect_eq(isShp(b), false);
    putU32BE(*b, 24, 60); // restore 120 bytes

    // Record with out-of-bounds content_words (50 words = 100 bytes > 12 remaining)
    putU32BE(*b, 100, 1);
    putU32BE(*b, 104, 50);
    let it = ShpRecordIter.init(b);
    expect_eq(it.next() == null, true);
});
