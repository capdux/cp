// Source: cp/cap/test/test_topojson_production_deep.cp
// Deep Production Test Suite for cap:topojson
// Covers:
//   1. Quantized Topology with `transform` delta-encoded Arc decoding to exact lon/lat
//   2. Unquantized Topology without `transform` raw floating-point Arc decoding & arcSliceAt
//   3. Signed Arc reference decoding (`resolveArcRef`: forward >= 0, reversed ~idx < 0, edge cases)
//   4. Multi-object `TopoObjectIter` zero-copy iteration & 2D/3D `bbox` extraction
//   5. Malformed / non-Topology / truncated JSON & out-of-bounds arc index resilience

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TopoTransform,
    TopoPoint,
    TopoBBox,
    TopoArcRef,
    TopoHeader,
    TopoObjectEntry,
    TopoArcIter,
    TopoArcPointIter,
    TopoObjectIter,
    isTopoJson,
    parseTopoHeader,
    resolveArcRef,
    arcSliceAt,
    countArcPoints,
} from "cap:topojson";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) < 0.000001;
}

fn testQuantizedTransformDeltaDecoding() {
    // Classic quantized TopoJSON v1.0 with affine scale & translate
    const raw =
        "  { \n" ++
        "  \"type\": \"Topology\",\n" ++
        "  \"transform\": {\n" ++
        "    \"scale\": [0.001, 0.002],\n" ++
        "    \"translate\": [120.0, 30.0]\n" ++
        "  },\n" ++
        "  \"objects\": {\n" ++
        "    \"route\": { \"type\": \"LineString\", \"arcs\": [0] }\n" ++
        "  },\n" ++
        "  \"arcs\": [\n" ++
        "    [[1000, 500], [500, -250], [-200, 750]],\n" ++
        "    [[0, 0], [100, 100]]\n" ++
        "  ]\n" ++
        "}  ";

    expect_eq(isTopoJson(raw), true);

    const hdr_opt = parseTopoHeader(raw);
    expect_eq(hdr_opt != null, true);
    const hdr = hdr_opt.?;

    expect_eq(hdr.hasTransform(), true);
    expect_eq(hdr.hasBBox(), false);
    expect_eq(hdr.arc_count, 2);
    expect_eq(approxEq(hdr.transform.scale_x, 0.001), true);
    expect_eq(approxEq(hdr.transform.scale_y, 0.002), true);
    expect_eq(approxEq(hdr.transform.translate_x, 120.0), true);
    expect_eq(approxEq(hdr.transform.translate_y, 30.0), true);

    // Direct transform.apply check
    const direct_pt = hdr.transform.apply(1000.0, 500.0);
    expect_eq(approxEq(direct_pt.x, 121.0), true);
    expect_eq(approxEq(direct_pt.y, 31.0), true);

    // Decode Arc 0 with delta accumulation:
    //   [1000, 500]  -> acc=(1000, 500)  -> (121.0, 31.0)
    //   [500, -250]  -> acc=(1500, 250)  -> (121.5, 30.5)
    //   [-200, 750]  -> acc=(1300, 1000) -> (121.3, 32.0)
    const arc0 = arcSliceAt(hdr, 0).?;
    expect_eq(countArcPoints(arc0), 3);

    let pt_it = TopoArcPointIter.init(arc0, hdr.transform);
    const p0 = pt_it.next().?;
    expect_eq(approxEq(p0.x, 121.0), true);
    expect_eq(approxEq(p0.y, 31.0), true);

    const p1 = pt_it.next().?;
    expect_eq(approxEq(p1.x, 121.5), true);
    expect_eq(approxEq(p1.y, 30.5), true);

    const p2 = pt_it.next().?;
    expect_eq(approxEq(p2.x, 121.3), true);
    expect_eq(approxEq(p2.y, 32.0), true);

    expect_eq(pt_it.next() == null, true);

    // Decode Arc 1 via TopoArcIter
    let arc_it = hdr.arcs();
    const s0 = arc_it.next().?;
    const s1 = arc_it.next().?;
    expect_eq(arc_it.next() == null, true);
    expect_eq(std.mem.eql(u8, s0, arc0), true);

    let pt_it1 = TopoArcPointIter.init(s1, hdr.transform);
    const q0 = pt_it1.next().?;
    expect_eq(approxEq(q0.x, 120.0), true);
    expect_eq(approxEq(q0.y, 30.0), true);
    const q1 = pt_it1.next().?;
    expect_eq(approxEq(q1.x, 120.1), true);
    expect_eq(approxEq(q1.y, 30.2), true);
    expect_eq(pt_it1.next() == null, true);
}

fn testUnquantizedRawFloatArcDecoding() {
    // No `transform` present -> coordinates are raw unquantized floats (not delta-accumulated)
    const raw =
        "{\"type\":\"Topology\",\"objects\":{},\"arcs\":[" ++
        "[[-122.4194, 37.7749, 12.5], [-73.935242, 40.73061], [1.5e2, -3.25e1]]," ++
        "[[0, 0], [10.25, -20.5]]" ++
        "]}";

    const hdr = parseTopoHeader(raw).?;
    expect_eq(hdr.hasTransform(), false);
    expect_eq(hdr.arc_count, 2);

    const arc0 = arcSliceAt(hdr, 0).?;
    let pt_it = TopoArcPointIter.init(arc0, hdr.transform);

    const p0 = pt_it.next().?;
    expect_eq(approxEq(p0.x, -122.4194), true);
    expect_eq(approxEq(p0.y, 37.7749), true);

    const p1 = pt_it.next().?;
    expect_eq(approxEq(p1.x, -73.935242), true);
    expect_eq(approxEq(p1.y, 40.73061), true);

    const p2 = pt_it.next().?;
    expect_eq(approxEq(p2.x, 150.0), true);
    expect_eq(approxEq(p2.y, -32.5), true);

    expect_eq(pt_it.next() == null, true);

    const arc1 = arcSliceAt(hdr, 1).?;
    expect_eq(countArcPoints(arc1), 2);
}

fn testSignedArcRefResolution() {
    const r0 = resolveArcRef(0);
    expect_eq(r0.index, 0);
    expect_eq(r0.reversed, false);

    const r42 = resolveArcRef(42);
    expect_eq(r42.index, 42);
    expect_eq(r42.reversed, false);

    // ~0 == -1 -> index 0, reversed
    const rn1 = resolveArcRef(-1);
    expect_eq(rn1.index, 0);
    expect_eq(rn1.reversed, true);

    // ~1 == -2 -> index 1, reversed
    const rn2 = resolveArcRef(-2);
    expect_eq(rn2.index, 1);
    expect_eq(rn2.reversed, true);

    // ~99 == -100 -> index 99, reversed
    const rn100 = resolveArcRef(-100);
    expect_eq(rn100.index, 99);
    expect_eq(rn100.reversed, true);

    // Extreme negative i64 boundary without signed overflow
    const min_i64: i64 = -9223372036854775807 - 1;
    const r_min = resolveArcRef(min_i64);
    expect_eq(r_min.index, 9223372036854775807);
    expect_eq(r_min.reversed, true);
}

fn testObjectsIterationAndBBoxExtraction() {
    // Field order shuffled (`bbox` and `objects` before `type`) + 4-element 2D bbox
    const raw_2d =
        "{\"bbox\":[-180.0,-85.0,180.0,85.0]," ++
        "\"objects\":{" ++
        "\"cities\":{\"type\":\"GeometryCollection\",\"geometries\":[{\"type\":\"Point\",\"coordinates\":[1,2]}]}," ++
        "\"border\":{\"type\":\"LineString\",\"arcs\":[0]}," ++
        "\"island\":{\"type\":\"Polygon\",\"arcs\":[[0,-1]]}" ++
        "}," ++
        "\"arcs\":[[[0,0],[1,1]]]," ++
        "\"type\":\"Topology\"}";

    expect_eq(isTopoJson(raw_2d), true);
    const hdr = parseTopoHeader(raw_2d).?;
    expect_eq(hdr.hasBBox(), true);
    expect_eq(approxEq(hdr.bbox.min_x, -180.0), true);
    expect_eq(approxEq(hdr.bbox.min_y, -85.0), true);
    expect_eq(approxEq(hdr.bbox.max_x, 180.0), true);
    expect_eq(approxEq(hdr.bbox.max_y, 85.0), true);

    let obj_it = hdr.objects();

    const e0 = obj_it.next().?;
    expect_eq(std.mem.eql(u8, e0.name, "cities"), true);
    expect_eq(std.mem.eql(u8, e0.geom_type, "GeometryCollection"), true);
    expect_eq(e0.raw_json.len > 0 && e0.raw_json[0] == '{', true);

    const e1 = obj_it.next().?;
    expect_eq(std.mem.eql(u8, e1.name, "border"), true);
    expect_eq(std.mem.eql(u8, e1.geom_type, "LineString"), true);

    const e2 = obj_it.next().?;
    expect_eq(std.mem.eql(u8, e2.name, "island"), true);
    expect_eq(std.mem.eql(u8, e2.geom_type, "Polygon"), true);

    expect_eq(obj_it.next() == null, true);

    // 6-element 3D bbox [min_x, min_y, min_z, max_x, max_y, max_z]
    const raw_3d =
        "{\"type\":\"Topology\",\"bbox\":[10.0,20.0,0.0,30.0,40.0,100.0],\"objects\":{},\"arcs\":[]}";
    const hdr3d = parseTopoHeader(raw_3d).?;
    expect_eq(hdr3d.hasBBox(), true);
    expect_eq(approxEq(hdr3d.bbox.min_x, 10.0), true);
    expect_eq(approxEq(hdr3d.bbox.min_y, 20.0), true);
    expect_eq(approxEq(hdr3d.bbox.max_x, 30.0), true);
    expect_eq(approxEq(hdr3d.bbox.max_y, 40.0), true);
    expect_eq(hdr3d.arc_count, 0);
}

fn testMalformedAndBoundaryGuards() {
    // Non-Topology GeoJSON
    const geojson = "{\"type\":\"FeatureCollection\",\"features\":[]}";
    expect_eq(isTopoJson(geojson), false);
    expect_eq(parseTopoHeader(geojson) == null, true);

    // Empty / whitespace / non-object
    expect_eq(isTopoJson(""), false);
    expect_eq(isTopoJson("   "), false);
    expect_eq(isTopoJson("[]"), false);
    expect_eq(isTopoJson("{}"), false);

    // Truncated Topology JSON
    const truncated = "{\"type\":\"Topology\",\"arcs\":[[[0,0],[1,2]]";
    expect_eq(isTopoJson(truncated), false);
    expect_eq(parseTopoHeader(truncated) == null, true);

    // Trailing garbage after closing brace
    const trailing = "{\"type\":\"Topology\",\"arcs\":[]} trailing";
    expect_eq(isTopoJson(trailing), false);
    expect_eq(parseTopoHeader(trailing) == null, true);

    // Invalid transform (missing translate)
    const bad_transform = "{\"type\":\"Topology\",\"transform\":{\"scale\":[1,1]},\"arcs\":[]}";
    expect_eq(parseTopoHeader(bad_transform) == null, true);

    // Invalid bbox (only 2 elements)
    const bad_bbox = "{\"type\":\"Topology\",\"bbox\":[0,0],\"arcs\":[]}";
    expect_eq(parseTopoHeader(bad_bbox) == null, true);

    // Out-of-bounds arcSliceAt
    const valid = "{\"type\":\"Topology\",\"objects\":{},\"arcs\":[[[1,2],[3,4]]]}";
    const hdr = parseTopoHeader(valid).?;
    expect_eq(arcSliceAt(hdr, 0) != null, true);
    expect_eq(arcSliceAt(hdr, 1) == null, true);
    expect_eq(arcSliceAt(hdr, 9999) == null, true);

    // Malformed arc point stream terminates cleanly with null
    let bad_pt_it = TopoArcPointIter.init("[[10,20],[30,]]", TopoTransform {});
    const ok_pt = bad_pt_it.next().?;
    expect_eq(approxEq(ok_pt.x, 10.0), true);
    expect_eq(approxEq(ok_pt.y, 20.0), true);
    expect_eq(bad_pt_it.next() == null, true);
    expect_eq(bad_pt_it.next() == null, true);
}

test("1. Quantized Transform Delta-Encoded Arc Decoding to Exact Lon/Lat", () => {
    testQuantizedTransformDeltaDecoding();
});

test("2. Unquantized Raw Float Arc Decoding & Extra Dimensions", () => {
    testUnquantizedRawFloatArcDecoding();
});

test("3. Signed Arc Reference Decoding (~idx Reversed & Boundary)", () => {
    testSignedArcRefResolution();
});

test("4. Multi-Object Zero-Copy Iteration & 2D/3D BBox Extraction", () => {
    testObjectsIterationAndBBoxExtraction();
});

test("5. Non-Topology, Truncated JSON & Out-of-Bounds Arc Guards", () => {
    testMalformedAndBoundaryGuards();
});
