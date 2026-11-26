import { test, expect_eq } from "cap:test";
import {
    WKT_POINT,
    WKT_LINESTRING,
    WKT_POLYGON,
    Coord2D,
    WktGeometry,
    WktBbox,
    parseWkt,
    bboxWkt
} from "../../packages/@cap/wkt/src/index.cp";
import {
    GeoJsonGeometry,
    GeoJsonFeature,
    formatGeoJsonGeometry,
    formatGeoJsonFeature,
    parseGeoJsonPoint,
    parseGeoJsonGeometry,
    wktToGeoJson,
    geoJsonToWkt
} from "../../packages/@cap/geojson/src/index.cp";

fn gjTestBufEq(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn gjTestDummyGeom(): WktGeometry {
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

fn gjTestZeroBuf(): [256]u8 {
    let b: [256]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ];
    return b;
}

fn main() {
    test("1. @cap/geojson — formatGeoJsonGeometry for Point, LineString & Polygon", () => {
        let buf: [256]u8 = gjTestZeroBuf();

        let pt: WktGeometry = parseWkt("POINT (-122.5 37.75)") ?? gjTestDummyGeom();
        let n1: usize = formatGeoJsonGeometry(pt, buf[0..256]);
        expect_eq(gjTestBufEq(buf[0..256], n1, "{\"type\":\"Point\",\"coordinates\":[-122.5,37.75]}"), true);

        let line: WktGeometry = parseWkt("LINESTRING (0 0, 10 20, 30 40)") ?? gjTestDummyGeom();
        let n2: usize = formatGeoJsonGeometry(line, buf[0..256]);
        expect_eq(gjTestBufEq(buf[0..256], n2, "{\"type\":\"LineString\",\"coordinates\":[[0,0],[10,20],[30,40]]}"), true);

        let poly: WktGeometry = parseWkt("POLYGON ((0 0, 4 0, 4 3, 0 0))") ?? gjTestDummyGeom();
        let n3: usize = formatGeoJsonGeometry(poly, buf[0..256]);
        expect_eq(gjTestBufEq(buf[0..256], n3, "{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[4,0],[4,3],[0,0]]]}"), true);

        let wrap: GeoJsonGeometry = GeoJsonGeometry.fromWkt(pt);
        let back: WktGeometry = wrap.toWkt();
        expect_eq(back.kind, WKT_POINT);
        expect_eq(back.coord_count, 1);
    });

    test("2. @cap/geojson — formatGeoJsonFeature with id & bbox from @cap/wkt", () => {
        let buf: [256]u8 = gjTestZeroBuf();
        let poly: WktGeometry = parseWkt("POLYGON ((-5 2, 10 -3, 7 15, -5 2))") ?? gjTestDummyGeom();
        let feat: GeoJsonFeature = GeoJsonFeature.init("zone-1", poly, true);
        expect_eq(feat.has_bbox, true);
        expect_eq(feat.bbox.min_x, -5.0);
        expect_eq(feat.bbox.max_y, 15.0);

        let n: usize = formatGeoJsonFeature(feat, buf[0..256]);
        expect_eq(
            gjTestBufEq(
                buf[0..256],
                n,
                "{\"type\":\"Feature\",\"id\":\"zone-1\",\"bbox\":[-5,-3,10,15],\"geometry\":{\"type\":\"Polygon\",\"coordinates\":[[[-5,2],[10,-3],[7,15],[-5,2]]]},\"properties\":{}}"
            ),
            true
        );
    });

    test("3. @cap/geojson — parseGeoJsonPoint with reversed key order & negative decimals", () => {
        let p1 = parseGeoJsonPoint("{\"type\": \"Point\", \"coordinates\": [-12.25, 45.5]}");
        expect_eq(p1 == null, false);
        let g1: WktGeometry = p1 ?? gjTestDummyGeom();
        expect_eq(g1.kind, WKT_POINT);
        expect_eq(g1.coord_count, 1);
        expect_eq(g1.coords[0].x, -12.25);
        expect_eq(g1.coords[0].y, 45.5);

        let p2 = parseGeoJsonPoint("{\"coordinates\": [1.5e1, -2.5], \"type\": \"Point\"}");
        expect_eq(p2 == null, false);
        let g2: WktGeometry = p2 ?? gjTestDummyGeom();
        expect_eq(g2.coords[0].x, 15.0);
        expect_eq(g2.coords[0].y, -2.5);

        // Non-Point rejected by parseGeoJsonPoint
        expect_eq(parseGeoJsonPoint("{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]}") == null, true);
    });

    test("4. @cap/geojson — parseGeoJsonGeometry for LineString & Polygon", () => {
        let l = parseGeoJsonGeometry("{\"type\":\"LineString\",\"coordinates\":[[1,2],[3.5,4.5],[5,6]]}");
        expect_eq(l == null, false);
        let line: WktGeometry = l ?? gjTestDummyGeom();
        expect_eq(line.kind, WKT_LINESTRING);
        expect_eq(line.coord_count, 3);
        expect_eq(line.coords[1].x, 3.5);
        expect_eq(line.coords[2].y, 6.0);

        let p = parseGeoJsonGeometry("{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[10,0],[10,10],[0,10],[0,0]]]}");
        expect_eq(p == null, false);
        let poly: WktGeometry = p ?? gjTestDummyGeom();
        expect_eq(poly.kind, WKT_POLYGON);
        expect_eq(poly.coord_count, 5);
        let bb: WktBbox = bboxWkt(poly);
        expect_eq(bb.valid, true);
        expect_eq(bb.max_x, 10.0);
        expect_eq(bb.max_y, 10.0);
    });

    test("5. @cap/geojson — strict rejection of invalid types, empty coords & malformed JSON", () => {
        expect_eq(parseGeoJsonGeometry("") == null, true);
        expect_eq(parseGeoJsonGeometry("{}") == null, true);
        expect_eq(parseGeoJsonGeometry("{\"type\":\"Circle\",\"coordinates\":[0,0]}") == null, true);
        expect_eq(parseGeoJsonGeometry("{\"type\":\"Point\",\"coordinates\":[]}") == null, true);
        expect_eq(parseGeoJsonGeometry("{\"type\":\"Point\",\"coordinates\":[1]}") == null, true);
        expect_eq(parseGeoJsonGeometry("{\"type\":\"LineString\",\"coordinates\":[]}") == null, true);
        expect_eq(parseGeoJsonGeometry("{\"type\":\"Polygon\",\"coordinates\":[]}") == null, true);
        expect_eq(parseGeoJsonGeometry("{\"type\":\"Polygon\",\"coordinates\":[[]]}") == null, true);
        expect_eq(parseGeoJsonGeometry("{\"type\":\"Point\",\"coordinates\":[1,2]} trailing") == null, true);
    });

    test("6. @cap/geojson — one-step wktToGeoJson & geoJsonToWkt round-trip interop", () => {
        let buf: [256]u8 = gjTestZeroBuf();
        let n1: usize = wktToGeoJson("LINESTRING (1 2, 3.5 -4.5)", buf[0..256]);
        expect_eq(gjTestBufEq(buf[0..256], n1, "{\"type\":\"LineString\",\"coordinates\":[[1,2],[3.5,-4.5]]}"), true);

        let wbuf: [256]u8 = gjTestZeroBuf();
        let n2: usize = geoJsonToWkt("{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[2,0],[2,2],[0,0]]]}", wbuf[0..256]);
        expect_eq(gjTestBufEq(wbuf[0..256], n2, "POLYGON ((0 0, 2 0, 2 2, 0 0))"), true);

        expect_eq(wktToGeoJson("POINT EMPTY", buf[0..256]), 0);
        expect_eq(wktToGeoJson("INVALID (1 2)", buf[0..256]), 0);
    });
}
