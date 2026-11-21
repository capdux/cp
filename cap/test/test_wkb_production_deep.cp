// Source: cp/cap/test/test_wkb_production_deep.cp (Production deep tests for cap:wkb)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WKB_XDR,
    WKB_NDR,
    EWKB_FLAG_Z,
    EWKB_FLAG_M,
    EWKB_FLAG_SRID,
    WkbGeomType,
    WkbPoint,
    WkbHeader,
    parseWkbHeader,
    wkbByteLength,
    WkbPointIter,
    WkbRingIter,
    WkbChildIter,
    WkbReader,
    WkbWriter,
} from "cap:wkb";

test("1. NDR (LE) and XDR (BE) Point, LineString, and Polygon roundtrip", () => {
    // NDR LineString with 3 points
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = WkbWriter.initNdr(buf[0..]);
    const line: [3]WkbPoint = [
        WkbPoint.xy(1.0, 2.0),
        WkbPoint.xy(3.5, 4.5),
        WkbPoint.xy(-10.0, 20.0),
    ];
    expect_eq(w.writeLineString(line[0..], false, false, null), true);
    const enc_line = w.written();
    expect_eq(wkbByteLength(enc_line) ?? 0, enc_line.len);

    let pit = WkbReader.linePoints(enc_line) ?? unreachable;
    expect_eq(pit.len(), 3);
    const p0 = pit.next() ?? unreachable;
    const p1 = pit.next() ?? unreachable;
    const p2 = pit.next() ?? unreachable;
    expect_eq(p0.x > 0.99 && p0.x < 1.01, true);
    expect_eq(p1.y > 4.49 && p1.y < 4.51, true);
    expect_eq(p2.x < -9.99 && p2.x > -10.01, true);
    expect_eq(pit.next() == null, true);

    // XDR (Big-Endian) Polygon with outer ring (4 pts) + hole ring (4 pts)
    let pbuf: [256]u8 = std.mem.zeroes([256]u8);
    let pw = WkbWriter.initXdr(pbuf[0..]);
    expect_eq(pw.writePolygonHeader(2, false, false, null), true);
    const outer: [4]WkbPoint = [
        WkbPoint.xy(0.0, 0.0),
        WkbPoint.xy(10.0, 0.0),
        WkbPoint.xy(10.0, 10.0),
        WkbPoint.xy(0.0, 0.0),
    ];
    const hole: [4]WkbPoint = [
        WkbPoint.xy(2.0, 2.0),
        WkbPoint.xy(4.0, 2.0),
        WkbPoint.xy(4.0, 4.0),
        WkbPoint.xy(2.0, 2.0),
    ];
    expect_eq(pw.writeRing(outer[0..], false, false), true);
    expect_eq(pw.writeRing(hole[0..], false, false), true);

    const enc_poly = pw.written();
    expect_eq(enc_poly[0], WKB_XDR);
    expect_eq(wkbByteLength(enc_poly) ?? 0, enc_poly.len);

    let rit = WkbReader.polygonRings(enc_poly) ?? unreachable;
    expect_eq(rit.len(), 2);
    let r0 = rit.next() ?? unreachable;
    expect_eq(r0.len(), 4);
    * = r0.next() ?? unreachable;
    const r0_p1 = r0.next() ?? unreachable;
    expect_eq(r0_p1.x > 9.99 && r0_p1.x < 10.01, true);

    let r1 = rit.next() ?? unreachable;
    const r1_p0 = r1.next() ?? unreachable;
    expect_eq(r1_p0.x > 1.99 && r1_p0.x < 2.01, true);
    expect_eq(rit.next() == null, true);
});

test("2. PostGIS EWKB with SRID=4326 and 4D ZM coordinates", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = WkbWriter.initNdr(buf[0..]);
    const pt_in = WkbPoint.xyzm(116.397, 39.908, 43.5, 1000.25);
    expect_eq(w.writePoint(pt_in, 4326), true);

    const enc = w.written();
    expect_eq(enc.len, 1 + 4 + 4 + 32); // order + type + srid + 4*f64

    const hdr = parseWkbHeader(enc) ?? unreachable;
    expect_eq(hdr.little_endian, true);
    expect_eq(hdr.geom_type, WkbGeomType.point);
    expect_eq(hdr.has_z, true);
    expect_eq(hdr.has_m, true);
    expect_eq(hdr.srid ?? 0, 4326);
    expect_eq(hdr.coord_dims, 4);
    expect_eq(hdr.body_offset, 9);

    const pt = WkbReader.readPoint(enc) ?? unreachable;
    expect_eq(pt.x > 116.39 && pt.x < 116.40, true);
    expect_eq(pt.y > 39.90 && pt.y < 39.91, true);
    expect_eq(pt.z > 43.49 && pt.z < 43.51, true);
    expect_eq(pt.m > 1000.24 && pt.m < 1000.26, true);
});

test("3. ISO WKB 1001 (PointZ), 2001 (PointM), and 3001 (PointZM) decoding", () => {
    // Helper: write ISO type code directly
    let buf: [48]u8 = std.mem.zeroes([48]u8);

    // 1001 = PointZ (LE)
    let w1 = WkbWriter.initNdr(buf[0..]);
    * = w1.writeU8(WKB_NDR);
    * = w1.writeU32(1001);
    * = w1.writeF64(1.0);
    * = w1.writeF64(2.0);
    * = w1.writeF64(3.0);
    const h1 = parseWkbHeader(w1.written()) ?? unreachable;
    expect_eq(h1.geom_type, WkbGeomType.point);
    expect_eq(h1.has_z, true);
    expect_eq(h1.has_m, false);
    expect_eq(h1.coord_dims, 3);
    const pt1 = WkbReader.readPoint(w1.written()) ?? unreachable;
    expect_eq(pt1.z > 2.99 && pt1.z < 3.01, true);

    // 2001 = PointM (LE)
    let w2 = WkbWriter.initNdr(buf[0..]);
    * = w2.writeU8(WKB_NDR);
    * = w2.writeU32(2001);
    * = w2.writeF64(4.0);
    * = w2.writeF64(5.0);
    * = w2.writeF64(6.0);
    const h2 = parseWkbHeader(w2.written()) ?? unreachable;
    expect_eq(h2.has_z, false);
    expect_eq(h2.has_m, true);
    const pt2 = WkbReader.readPoint(w2.written()) ?? unreachable;
    expect_eq(pt2.m > 5.99 && pt2.m < 6.01, true);

    // 3001 = PointZM (BE)
    let w3 = WkbWriter.initXdr(buf[0..]);
    * = w3.writeU8(WKB_XDR);
    * = w3.writeU32(3001);
    * = w3.writeF64(7.0);
    * = w3.writeF64(8.0);
    * = w3.writeF64(9.0);
    * = w3.writeF64(10.0);
    const h3 = parseWkbHeader(w3.written()) ?? unreachable;
    expect_eq(h3.has_z, true);
    expect_eq(h3.has_m, true);
    expect_eq(h3.coord_dims, 4);
    const pt3 = WkbReader.readPoint(w3.written()) ?? unreachable;
    expect_eq(pt3.z > 8.99 && pt3.z < 9.01, true);
    expect_eq(pt3.m > 9.99 && pt3.m < 10.01, true);
});

test("4. MultiPoint and GeometryCollection WkbChildIter zero-copy traversal", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = WkbWriter.initNdr(buf[0..]);

    expect_eq(w.writeCollectionHeader(WkbGeomType.geometrycollection, 2, 4326), true);
    expect_eq(w.writePoint(WkbPoint.xy(10.0, 20.0), null), true);
    const seg: [2]WkbPoint = [WkbPoint.xy(1.0, 1.0), WkbPoint.xy(2.0, 2.0)];
    expect_eq(w.writeLineString(seg[0..], false, false, null), true);

    const enc = w.written();
    expect_eq(wkbByteLength(enc) ?? 0, enc.len);

    let cit = WkbReader.children(enc) ?? unreachable;
    expect_eq(cit.len(), 2);

    const c0 = cit.next() ?? unreachable;
    const p0 = WkbReader.readPoint(c0) ?? unreachable;
    expect_eq(p0.x > 9.99 && p0.x < 10.01, true);

    const c1 = cit.next() ?? unreachable;
    let l1 = WkbReader.linePoints(c1) ?? unreachable;
    expect_eq(l1.len(), 2);
    expect_eq(cit.next() == null, true);
});

test("5. Malformed WKB protection: invalid byte order, unknown geom type, truncated SRID/coords, and OOB counts", () => {
    // Invalid byte order 0x02
    const bad_ord: [5]u8 = [2, 1, 0, 0, 0];
    expect_eq(parseWkbHeader(bad_ord[0..]) == null, true);

    // Unknown geometry type 99
    const bad_type: [5]u8 = [WKB_NDR, 99, 0, 0, 0];
    expect_eq(parseWkbHeader(bad_type[0..]) == null, true);

    // EWKB SRID flag set without 4B SRID payload
    const trunc_srid: [6]u8 = [WKB_NDR, 1, 0, 0, 0x20, 0];
    expect_eq(parseWkbHeader(trunc_srid[0..]) == null, true);

    // LineString claiming 1_000_000 points on a 9-byte buffer
    const oob_line: [9]u8 = [WKB_NDR, 2, 0, 0, 0, 0x40, 0x42, 0x0F, 0x00];
    expect_eq(wkbByteLength(oob_line[0..]) == null, true);
    expect_eq(WkbReader.linePoints(oob_line[0..]) == null, true);
});
