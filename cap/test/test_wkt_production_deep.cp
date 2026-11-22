// Source: cp/cap/test/test_wkt_production_deep.cp
// Deep Production Test Suite for cap:wkt & cap.wkt
// Covers:
//   1. POINT / LINESTRING / POLYGON (with hole) / MULTIPOLYGON / GEOMETRYCOLLECTION & EMPTY case-insensitive WKT round-trip
//   2. encodeWkb / decodeWkb across LittleEndian & BigEndian + WKT equivalence
//   3. MULTIPOINT ((10 40), (40 30)) and MULTIPOINT (10 40, 40 30) compatibility
//   4. Invalid geometry type, unclosed/missing paren, invalid WKB byte order 0x02, truncated WKB & >32-level MaxDepthExceeded
//   5. Dynamic lazy namespace `import cap from "cap"; cap.wkt` & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    WktError,
    WkbByteOrder,
    WktCoord,
    WktPolygon,
    WktGeometry,
    parseWkt,
    formatWkt,
    encodeWkb,
    decodeWkb,
} from "cap:geo";
import cap from "cap";

fn testWktCaseInsensitiveRoundTripAndEmpty(): ! {
    const alloc = std.testing.allocator;

    // Mixed-case POINT
    let pt = parseWkt(alloc, "pOiNt ( 121.4737   31.2304 )")?;
    defer pt.deinit(alloc);
    const pt_str = formatWkt(alloc, pt)?;
    defer alloc.free(pt_str);
    expect_eq(std.mem.eql(u8, pt_str, "POINT (121.4737 31.2304)"), true);

    // Mixed-case LINESTRING Z
    let ls = parseWkt(alloc, "lineString z (-10 -5 1.5, 0 5 2.5, 10 0 3.5)")?;
    defer ls.deinit(alloc);
    const ls_str = formatWkt(alloc, ls)?;
    defer alloc.free(ls_str);
    expect_eq(std.mem.eql(u8, ls_str, "LINESTRING Z (-10 -5 1.5, 0 5 2.5, 10 0 3.5)"), true);

    // POLYGON with interior hole
    let poly = parseWkt(alloc, "Polygon ((0 0, 20 0, 20 20, 0 20, 0 0), (5 5, 15 5, 15 15, 5 15, 5 5))")?;
    defer poly.deinit(alloc);
    const poly_str = formatWkt(alloc, poly)?;
    defer alloc.free(poly_str);
    expect_eq(std.mem.eql(u8, poly_str, "POLYGON ((0 0, 20 0, 20 20, 0 20, 0 0), (5 5, 15 5, 15 15, 5 15, 5 5))"), true);

    // MULTIPOLYGON
    let mp = parseWkt(alloc, "multiPolygon (((0 0, 2 0, 2 2, 0 2, 0 0)), ((10 10, 12 10, 12 12, 10 12, 10 10)))")?;
    defer mp.deinit(alloc);
    const mp_str = formatWkt(alloc, mp)?;
    defer alloc.free(mp_str);
    expect_eq(std.mem.eql(u8, mp_str, "MULTIPOLYGON (((0 0, 2 0, 2 2, 0 2, 0 0)), ((10 10, 12 10, 12 12, 10 12, 10 10)))"), true);

    // GEOMETRYCOLLECTION + EMPTY variants
    let gc = parseWkt(alloc, "geometryCollection (point (1 2), lineString empty, polygon empty, point empty)")?;
    defer gc.deinit(alloc);
    const gc_str = formatWkt(alloc, gc)?;
    defer alloc.free(gc_str);
    expect_eq(std.mem.eql(u8, gc_str, "GEOMETRYCOLLECTION (POINT (1 2), LINESTRING EMPTY, POLYGON EMPTY, POINT EMPTY)"), true);
}

fn testWkbEndiannessRoundTrip(): ! {
    const alloc = std.testing.allocator;
    const raw = "GEOMETRYCOLLECTION (POINT (3.5 -7.25), LINESTRING (0 0, 10 20, 30 40), POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (2 2, 4 2, 4 4, 2 4, 2 2)), MULTILINESTRING ((1 1, 2 2), (3 3, 4 4)), MULTIPOLYGON (((50 50, 55 50, 55 55, 50 55, 50 50))))";

    let orig = parseWkt(alloc, raw)?;
    defer orig.deinit(alloc);

    // LittleEndian round-trip
    const wkb_le = encodeWkb(alloc, orig, WkbByteOrder.LittleEndian)?;
    defer alloc.free(wkb_le);
    expect_eq(wkb_le[0], 1);

    let dec_le = decodeWkb(alloc, wkb_le)?;
    defer dec_le.deinit(alloc);

    // Re-encode decoded geometry as BigEndian
    const wkb_be = encodeWkb(alloc, dec_le, WkbByteOrder.BigEndian)?;
    defer alloc.free(wkb_be);
    expect_eq(wkb_be[0], 0);

    let dec_be = decodeWkb(alloc, wkb_be)?;
    defer dec_be.deinit(alloc);

    const wkt_le = formatWkt(alloc, dec_le)?;
    defer alloc.free(wkt_le);
    const wkt_be = formatWkt(alloc, dec_be)?;
    defer alloc.free(wkt_be);

    expect_eq(std.mem.eql(u8, wkt_le, raw), true);
    expect_eq(std.mem.eql(u8, wkt_be, raw), true);
}

fn testMultiPointBothFormats(): ! {
    const alloc = std.testing.allocator;

    let mp_paren = parseWkt(alloc, "MULTIPOINT ((10 40), (40 30))")?;
    defer mp_paren.deinit(alloc);

    let mp_flat = parseWkt(alloc, "multipoint (10 40, 40 30)")?;
    defer mp_flat.deinit(alloc);

    const s1 = formatWkt(alloc, mp_paren)?;
    defer alloc.free(s1);
    const s2 = formatWkt(alloc, mp_flat)?;
    defer alloc.free(s2);

    expect_eq(std.mem.eql(u8, s1, "MULTIPOINT ((10 40), (40 30))"), true);
    expect_eq(std.mem.eql(u8, s1, s2), true);
}

fn testValidationAndSecurityGuards(): ! {
    const alloc = std.testing.allocator;

    // 1. Invalid geometry type
    let bad_type = parseWkt(alloc, "HEXAGON (0 0, 1 1)");
    expect_eq(bad_type == WktError.InvalidGeometryType, true);

    // 2. Unclosed / missing parenthesis
    let missing_paren = parseWkt(alloc, "LINESTRING (0 0, 1 1");
    expect_eq(missing_paren == WktError.UnexpectedEof, true);

    // 3. Unclosed polygon ring
    let bad_ring = parseWkt(alloc, "POLYGON ((0 0, 1 0, 1 1, 0 1))");
    expect_eq(bad_ring == WktError.InvalidRing, true);

    // 4. Invalid WKB byte order marker (0x02)
    let bad_wkb_hdr: [21]u8 = [
        0x02, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    ];
    let bad_bo = decodeWkb(alloc, &bad_wkb_hdr);
    expect_eq(bad_bo == WktError.InvalidWkbHeader, true);

    // 5. Truncated WKB buffer
    let trunc_wkb: [9]u8 = [0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    let trunc_res = decodeWkb(alloc, &trunc_wkb);
    expect_eq(trunc_res == WktError.UnexpectedEof, true);

    // 6. >32-level nested GEOMETRYCOLLECTION bomb -> MaxDepthExceeded
    let bomb: std.ArrayList(u8) = .empty;
    defer bomb.deinit(alloc);
    let d: usize = 0;
    while (d < 34) {
        bomb.appendSlice(alloc, "GEOMETRYCOLLECTION (")?;
        d += 1;
    }
    bomb.appendSlice(alloc, "POINT (1 2)")?;
    d = 0;
    while (d < 34) {
        bomb.append(alloc, ')')?;
        d += 1;
    }
    let bomb_res = parseWkt(alloc, bomb.items);
    expect_eq(bomb_res == WktError.MaxDepthExceeded, true);
}

fn testDynamicCapNamespaceAndZeroLeaks(): ! {
    const alloc = std.testing.allocator;
    let geom = cap.geo.parseWkt(alloc, "POLYGON ((0 0, 4 0, 4 4, 0 4, 0 0))")?;
    defer geom.deinit(alloc);

    const wkb = cap.geo.encodeWkb(alloc, geom, cap.geo.WkbByteOrder.BigEndian)?;
    defer alloc.free(wkb);

    let decoded = cap.geo.decodeWkb(alloc, wkb)?;
    defer decoded.deinit(alloc);

    const out = cap.geo.formatWkt(alloc, decoded)?;
    defer alloc.free(out);
    expect_eq(std.mem.eql(u8, out, "POLYGON ((0 0, 4 0, 4 4, 0 4, 0 0))"), true);
}

test("1. WKT Case-Insensitive Round-Trip & EMPTY Geometries", () => {
    try testWktCaseInsensitiveRoundTripAndEmpty();
});

test("2. ISO WKB LittleEndian & BigEndian Bidirectional Round-Trip", () => {
    try testWkbEndiannessRoundTrip();
});

test("3. MULTIPOINT Parenthesized & Flat Syntax Compatibility", () => {
    try testMultiPointBothFormats();
});

test("4. WKT/WKB Validation & 32-Level MaxDepthExceeded Bomb Guard", () => {
    try testValidationAndSecurityGuards();
});

test("5. Dynamic Lazy Namespace `cap.wkt` & std.testing.allocator Zero Leaks", () => {
    try testDynamicCapNamespaceAndZeroLeaks();
});
