// Source: cp/cap/test/test_geojson_production_deep.cp
// Deep Production Test Suite for cap:geojson & cap.geojson
// Covers:
//   1. Point / LineString / Polygon (with hole) / MultiPolygon / FeatureCollection parse & stringify round-trip + computeBBox
//   2. pointInPolygon interior, exterior, boundary, and interior hole exclusion
//   3. simplifyDouglasPeucker across multiple epsilon thresholds preserving endpoints & key inflection points
//   4. RFC 7946 validation errors: InvalidType (missing type), InvalidCoordinates (<2 coords), InvalidRing (unclosed / <4 pts)
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.geojson` & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    GeoJsonError,
    Position,
    BBox,
    PolygonData,
    Geometry,
    Feature,
    FeatureCollection,
    GeoJsonDoc,
    parse,
    stringify,
    computeBBox,
    pointInPolygon,
    simplifyDouglasPeucker,
} from "cap:geojson";
import cap from "cap";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) < 0.000001;
}

fn testParseStringifyRoundTripAndBBox(): ! {
    const alloc = std.testing.allocator;

    // 1. Point with altitude
    let pt_doc = parse(alloc, "{\"type\":\"Point\",\"coordinates\":[121.4737,31.2304,15.5]}")?;
    defer pt_doc.deinit(alloc);
    const pt_json = stringify(alloc, pt_doc)?;
    defer alloc.free(pt_json);
    let pt_rt = parse(alloc, pt_json)?;
    defer pt_rt.deinit(alloc);
    switch (pt_rt) {
        .geometry => |g| {
            const bb = computeBBox(g).?;
            expect_eq(approxEq(bb.min_lon, 121.4737), true);
            expect_eq(approxEq(bb.max_lat, 31.2304), true);
            switch (g) {
                .point => |p| {
                    expect_eq(approxEq(p.alt.?, 15.5), true);
                },
                else => expect_eq(false, true),
            }
        },
        else => expect_eq(false, true),
    }

    // 2. LineString + Polygon with hole + MultiPolygon inside FeatureCollection
    const fc_raw =
        "{\"type\":\"FeatureCollection\",\"features\":[" ++
        "{\"type\":\"Feature\",\"id\":\"route-1\",\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[-10,-5],[0,5],[10,0]]},\"properties\":{\"name\":\"track\"}}," ++
        "{\"type\":\"Feature\",\"id\":42,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[20,0],[20,20],[0,20],[0,0]],[[5,5],[15,5],[15,15],[5,15],[5,5]]]},\"properties\":{\"zone\":\"ring\"}}," ++
        "{\"type\":\"Feature\",\"geometry\":{\"type\":\"MultiPolygon\",\"coordinates\":[[[[30,30],[32,30],[32,32],[30,32],[30,30]]]]},\"properties\":null}" ++
        "]}";

    let fc_doc = parse(alloc, fc_raw)?;
    defer fc_doc.deinit(alloc);

    const fc_json = stringify(alloc, fc_doc)?;
    defer alloc.free(fc_json);

    let fc_rt = parse(alloc, fc_json)?;
    defer fc_rt.deinit(alloc);

    switch (fc_rt) {
        .feature_collection => |fc| {
            expect_eq(fc.features.len, 3);
            expect_eq(std.mem.eql(u8, fc.features[0].id.?, "route-1"), true);
            expect_eq(std.mem.eql(u8, fc.features[1].id.?, "42"), true);
            expect_eq(fc.features[2].id == null, true);

            const ls_bb = computeBBox(fc.features[0].geometry.?).?;
            expect_eq(approxEq(ls_bb.min_lon, -10.0), true);
            expect_eq(approxEq(ls_bb.min_lat, -5.0), true);
            expect_eq(approxEq(ls_bb.max_lon, 10.0), true);
            expect_eq(approxEq(ls_bb.max_lat, 5.0), true);

            const mp_bb = computeBBox(fc.features[2].geometry.?).?;
            expect_eq(approxEq(mp_bb.min_lon, 30.0), true);
            expect_eq(approxEq(mp_bb.max_lat, 32.0), true);
        },
        else => expect_eq(false, true),
    }
}

fn testPointInPolygonWithHoles(): ! {
    const alloc = std.testing.allocator;
    // Outer square [0, 10] x [0, 10] with interior hole [3, 7] x [3, 7]
    const poly_raw =
        "{\"type\":\"Polygon\",\"coordinates\":[" ++
        "[[0,0],[10,0],[10,10],[0,10],[0,0]]," ++
        "[[3,3],[7,3],[7,7],[3,7],[3,3]]" ++
        "]}";

    let doc = parse(alloc, poly_raw)?;
    defer doc.deinit(alloc);

    switch (doc) {
        .geometry => |g| {
            switch (g) {
                .polygon => |poly| {
                    // Inside outer ring, outside hole
                    expect_eq(pointInPolygon(Position{ lon: 1.5, lat: 1.5, alt: null }, poly), true);
                    expect_eq(pointInPolygon(Position{ lon: 8.5, lat: 8.5, alt: null }, poly), true);
                    // On outer boundary edge and vertex
                    expect_eq(pointInPolygon(Position{ lon: 5.0, lat: 0.0, alt: null }, poly), true);
                    expect_eq(pointInPolygon(Position{ lon: 10.0, lat: 10.0, alt: null }, poly), true);
                    // Outside outer ring
                    expect_eq(pointInPolygon(Position{ lon: -0.1, lat: 5.0, alt: null }, poly), false);
                    expect_eq(pointInPolygon(Position{ lon: 10.5, lat: 5.0, alt: null }, poly), false);
                    // Strictly inside interior hole -> MUST be false
                    expect_eq(pointInPolygon(Position{ lon: 5.0, lat: 5.0, alt: null }, poly), false);
                    expect_eq(pointInPolygon(Position{ lon: 4.0, lat: 6.0, alt: null }, poly), false);
                },
                else => expect_eq(false, true),
            }
        },
        else => expect_eq(false, true),
    }
}

fn testSimplifyDouglasPeucker(): ! {
    const alloc = std.testing.allocator;
    // Zig-zag polyline with small jitter (0.1) and large inflection (3.0)
    let pts: [6]Position = .{
        Position{ lon: 0.0, lat: 0.0, alt: null },
        Position{ lon: 1.0, lat: 0.1, alt: null },
        Position{ lon: 2.0, lat: -0.05, alt: null },
        Position{ lon: 3.0, lat: 3.0, alt: null },
        Position{ lon: 4.0, lat: 1.55, alt: null },
        Position{ lon: 5.0, lat: 0.0, alt: null },
    };

    // Small epsilon keeps all points
    const s_fine = simplifyDouglasPeucker(alloc, &pts, 0.01)?;
    defer alloc.free(s_fine);
    expect_eq(s_fine.len, 6);

    // Medium epsilon (0.5) drops small jitter but keeps major inflection at (3,3)
    const s_med = simplifyDouglasPeucker(alloc, &pts, 0.5)?;
    defer alloc.free(s_med);
    expect_eq(s_med.len, 3);
    expect_eq(approxEq(s_med[0].lon, 0.0), true);
    expect_eq(approxEq(s_med[1].lon, 3.0), true);
    expect_eq(approxEq(s_med[1].lat, 3.0), true);
    expect_eq(approxEq(s_med[2].lon, 5.0), true);

    // Large epsilon (5.0) collapses down to start & end points only
    const s_coarse = simplifyDouglasPeucker(alloc, &pts, 5.0)?;
    defer alloc.free(s_coarse);
    expect_eq(s_coarse.len, 2);
    expect_eq(approxEq(s_coarse[0].lon, 0.0), true);
    expect_eq(approxEq(s_coarse[1].lon, 5.0), true);
}

fn testRfcValidationErrors(): ! {
    const alloc = std.testing.allocator;

    // 1. Missing "type" -> InvalidType
    let missing_type = parse(alloc, "{\"coordinates\":[1.0,2.0]}");
    expect_eq(missing_type == GeoJsonError.InvalidType, true);

    // 2. Coordinates with fewer than 2 elements -> InvalidCoordinates
    let short_coords = parse(alloc, "{\"type\":\"Point\",\"coordinates\":[120.5]}");
    expect_eq(short_coords == GeoJsonError.InvalidCoordinates, true);

    // 3. Ring with fewer than 4 positions -> InvalidRing
    let short_ring = parse(alloc, "{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[1,0],[0,0]]]}");
    expect_eq(short_ring == GeoJsonError.InvalidRing, true);

    // 4. Unclosed ring -> InvalidRing
    let unclosed_ring = parse(alloc, "{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[1,0],[1,1],[0,1]]]}");
    expect_eq(unclosed_ring == GeoJsonError.InvalidRing, true);
}

fn testDynamicCapNamespaceAndZeroLeaks(): ! {
    const alloc = std.testing.allocator;
    const raw = "{\"type\":\"Feature\",\"id\":\"ns-1\",\"geometry\":{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[4,0],[4,4],[0,4],[0,0]]]},\"properties\":{\"ok\":true}}";

    let doc = cap.geojson.parse(alloc, raw)?;
    defer doc.deinit(alloc);

    const out = cap.geojson.stringify(alloc, doc)?;
    defer alloc.free(out);

    switch (doc) {
        .feature => |f| {
            const g = f.geometry.?;
            const bb = cap.geojson.computeBBox(g).?;
            expect_eq(approxEq(bb.max_lon, 4.0), true);
            switch (g) {
                .polygon => |poly| {
                    expect_eq(cap.geojson.pointInPolygon(cap.geojson.Position{ lon: 2.0, lat: 2.0, alt: null }, poly), true);
                    expect_eq(cap.geojson.pointInPolygon(cap.geojson.Position{ lon: 5.0, lat: 2.0, alt: null }, poly), false);
                },
                else => expect_eq(false, true),
            }
        },
        else => expect_eq(false, true),
    }

    let line: [3]cap.geojson.Position = .{
        cap.geojson.Position{ lon: 0.0, lat: 0.0, alt: null },
        cap.geojson.Position{ lon: 1.0, lat: 0.01, alt: null },
        cap.geojson.Position{ lon: 2.0, lat: 0.0, alt: null },
    };
    const simplified = cap.geojson.simplifyDouglasPeucker(alloc, &line, 0.1)?;
    defer alloc.free(simplified);
    expect_eq(simplified.len, 2);
}

test("1. GeoJSON Point/LineString/Polygon/MultiPolygon/FeatureCollection Round-Trip & BBox", () => {
    try testParseStringifyRoundTripAndBBox();
});

test("2. Geofence pointInPolygon Interior, Exterior, Boundary & Hole Exclusion", () => {
    try testPointInPolygonWithHoles();
});

test("3. Douglas-Peucker Simplifier Across Epsilon Thresholds", () => {
    try testSimplifyDouglasPeucker();
});

test("4. RFC 7946 Validation Guards (InvalidType, InvalidCoordinates, InvalidRing)", () => {
    try testRfcValidationErrors();
});

test("5. Dynamic Lazy Namespace `cap.geojson` & std.testing.allocator Zero Leaks", () => {
    try testDynamicCapNamespaceAndZeroLeaks();
});
