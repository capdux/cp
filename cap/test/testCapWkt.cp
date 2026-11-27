import { test, expect_eq } from "cap:test";
import {
    WKT_POINT,
    WKT_LINESTRING,
    WKT_POLYGON,
    WKT_MULTIPOINT,
    Coord2D,
    WktGeometry,
    WktBbox,
    parseWkt,
    formatWkt,
    bboxWkt
} from "../../packages/wkt/src/index.cp";

fn wktBufEq(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn dummyGeom(): WktGeometry {
    let z: Coord2D = Coord2D { x: 0.0, y: 0.0 };
    return WktGeometry {
        kind: 0,
        is_empty: true,
        coords: [
            z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
            z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z
        ],
        coord_count: 0,
    };
}

fn main() {
    test("1. @cap/wkt — parse POINT & POINT EMPTY (case-insensitive, negative & decimal)", () => {
        let g = parseWkt("point (-12.5 34.25)");
        expect_eq(g == null, false);
        let pt: WktGeometry = g ?? dummyGeom();
        expect_eq(pt.kind, WKT_POINT);
        expect_eq(pt.is_empty, false);
        expect_eq(pt.coord_count, 1);
        expect_eq(pt.coords[0].x, -12.5);
        expect_eq(pt.coords[0].y, 34.25);

        let e = parseWkt("  Point   Empty  ");
        expect_eq(e == null, false);
        let emp: WktGeometry = e ?? dummyGeom();
        expect_eq(emp.kind, WKT_POINT);
        expect_eq(emp.is_empty, true);
        expect_eq(emp.coord_count, 0);
    });

    test("2. @cap/wkt — parse LINESTRING & POLYGON", () => {
        let l = parseWkt("LINESTRING (0 0, 10 10, 20 25)");
        expect_eq(l == null, false);
        let line: WktGeometry = l ?? dummyGeom();
        expect_eq(line.kind, WKT_LINESTRING);
        expect_eq(line.coord_count, 3);
        expect_eq(line.coords[2].x, 20.0);
        expect_eq(line.coords[2].y, 25.0);

        let p = parseWkt("POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0))");
        expect_eq(p == null, false);
        let poly: WktGeometry = p ?? dummyGeom();
        expect_eq(poly.kind, WKT_POLYGON);
        expect_eq(poly.coord_count, 5);
        expect_eq(poly.coords[1].x, 10.0);
        expect_eq(poly.coords[2].y, 10.0);
    });

    test("3. @cap/wkt — parse MULTIPOINT in nested & flat forms", () => {
        let m1 = parseWkt("MULTIPOINT ((10 40), (40 30), (20 20))");
        expect_eq(m1 == null, false);
        let mp1: WktGeometry = m1 ?? dummyGeom();
        expect_eq(mp1.kind, WKT_MULTIPOINT);
        expect_eq(mp1.coord_count, 3);
        expect_eq(mp1.coords[1].x, 40.0);
        expect_eq(mp1.coords[1].y, 30.0);

        let m2 = parseWkt("multipoint (1.5 -2.5, 3.0 4.0)");
        expect_eq(m2 == null, false);
        let mp2: WktGeometry = m2 ?? dummyGeom();
        expect_eq(mp2.kind, WKT_MULTIPOINT);
        expect_eq(mp2.coord_count, 2);
        expect_eq(mp2.coords[0].x, 1.5);
        expect_eq(mp2.coords[0].y, -2.5);
    });

    test("4. @cap/wkt — strict rejection of malformed brackets, empty coords & unknown types", () => {
        expect_eq(parseWkt("") == null, true);
        expect_eq(parseWkt("CIRCLE (1 2)") == null, true);
        expect_eq(parseWkt("POINT ()") == null, true);
        expect_eq(parseWkt("POINT (1)") == null, true);
        expect_eq(parseWkt("POINT (1 2") == null, true);
        expect_eq(parseWkt("POINT (1 2) extra") == null, true);
        expect_eq(parseWkt("POLYGON (0 0, 1 0, 0 1)") == null, true);
        expect_eq(parseWkt("LINESTRING (0 0, )") == null, true);
    });

    test("5. @cap/wkt — formatWkt canonical output & round-trip", () => {
        let buf: [128]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        ];

        let p: WktGeometry = parseWkt("point (-3.25 10)") ?? dummyGeom();
        let n1: usize = formatWkt(p, buf[0..128]);
        expect_eq(wktBufEq(buf[0..128], n1, "POINT (-3.25 10)"), true);

        let emp: WktGeometry = parseWkt("POINT EMPTY") ?? dummyGeom();
        let n2: usize = formatWkt(emp, buf[0..128]);
        expect_eq(wktBufEq(buf[0..128], n2, "POINT EMPTY"), true);

        let poly: WktGeometry = parseWkt("polygon ((0 0, 4 0, 4 3, 0 0))") ?? dummyGeom();
        let n3: usize = formatWkt(poly, buf[0..128]);
        expect_eq(wktBufEq(buf[0..128], n3, "POLYGON ((0 0, 4 0, 4 3, 0 0))"), true);

        let mp: WktGeometry = parseWkt("MULTIPOINT (1 2, 3 4)") ?? dummyGeom();
        let n4: usize = formatWkt(mp, buf[0..128]);
        expect_eq(wktBufEq(buf[0..128], n4, "MULTIPOINT ((1 2), (3 4))"), true);
    });

    test("6. @cap/wkt — bboxWkt computes bounding box & invalid for EMPTY", () => {
        let poly: WktGeometry = parseWkt("POLYGON ((-5 2, 10 -3, 7 15, -5 2))") ?? dummyGeom();
        let bb: WktBbox = bboxWkt(poly);
        expect_eq(bb.valid, true);
        expect_eq(bb.min_x, -5.0);
        expect_eq(bb.min_y, -3.0);
        expect_eq(bb.max_x, 10.0);
        expect_eq(bb.max_y, 15.0);

        let emp: WktGeometry = parseWkt("LINESTRING EMPTY") ?? dummyGeom();
        let eb: WktBbox = bboxWkt(emp);
        expect_eq(eb.valid, false);
    });
}
