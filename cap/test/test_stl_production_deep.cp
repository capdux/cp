// Source: cp/cap/test/test_stl_production_deep.cp (Production deep tests for cap:stl)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    StlFormat,
    StlVec3,
    StlTriangle,
    StlBinaryHeader,
    StlBounds,
    detectStlFormat,
    parseStlBinaryHeader,
    StlBinaryIter,
    writeStlBinary,
    computeStlBinaryBounds,
    countAsciiStlFacets,
} from "cap:science";

const SAMPLE_ASCII_STL: []const u8 =
    "solid unit_quad\n" ++
    "  facet normal 0 0 1\n" ++
    "    outer loop\n" ++
    "      vertex 0 0 0\n" ++
    "      vertex 1 0 0\n" ++
    "      vertex 1 1 0\n" ++
    "    endloop\n" ++
    "  endfacet\n" ++
    "  FACET NORMAL 0 0 1\n" ++
    "    outer loop\n" ++
    "      vertex 0 0 0\n" ++
    "      vertex 1 1 0\n" ++
    "      vertex 0 1 0\n" ++
    "    endloop\n" ++
    "  endfacet\n" ++
    "endsolid unit_quad\n";

test("1. writeStlBinary and StlBinaryIter round-trip IEEE 754 f32 triangles and attribute counts", () => {
    const tris: [2]StlTriangle = [
        StlTriangle {
            normal: StlVec3 { x: 0.0, y: 0.0, z: 1.0 },
            v1: StlVec3 { x: -1.5, y: 0.0, z: 0.0 },
            v2: StlVec3 { x: 2.5, y: 0.0, z: 0.0 },
            v3: StlVec3 { x: 0.0, y: 3.25, z: 0.5 },
            attr_byte_count: 7,
        },
        StlTriangle {
            normal: StlVec3 { x: 1.0, y: 0.0, z: 0.0 },
            v1: StlVec3 { x: 2.5, y: -4.0, z: -2.0 },
            v2: StlVec3 { x: 2.5, y: 0.0, z: 5.5 },
            v3: StlVec3 { x: 2.5, y: 1.0, z: 0.0 },
            attr_byte_count: 42,
        },
    ];

    let buf: [256]u8 = undefined;
    const encoded = writeStlBinary(tris[0..2], "cap:stl test mesh", buf[0..256]) ?? unreachable;
    expect_eq(encoded.len, 84 + 2 * 50);

    const hdr = parseStlBinaryHeader(encoded) ?? unreachable;
    expect_eq(hdr.triangle_count, 2);
    expect_eq(std.mem.eql(u8, hdr.header_bytes[0..17], "cap:stl test mesh"), true);
    expect_eq(hdr.header_bytes[79], 0);

    let it = StlBinaryIter.init(encoded);
    const t0 = it.next() ?? unreachable;
    expect_eq(t0.normal.z, 1.0);
    expect_eq(t0.v1.x, -1.5);
    expect_eq(t0.v3.y, 3.25);
    expect_eq(t0.attr_byte_count, 7);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.normal.x, 1.0);
    expect_eq(t1.v1.y, -4.0);
    expect_eq(t1.v2.z, 5.5);
    expect_eq(t1.attr_byte_count, 42);

    expect_eq(it.next() == null, true);
});

test("2. detectStlFormat distinguishes binary STL starting with 'solid ' from true ASCII STL", () => {
    const tri: [1]StlTriangle = [
        StlTriangle {
            normal: StlVec3 { x: 0.0, y: 0.0, z: 1.0 },
            v1: StlVec3 { x: 0.0, y: 0.0, z: 0.0 },
            v2: StlVec3 { x: 1.0, y: 0.0, z: 0.0 },
            v3: StlVec3 { x: 0.0, y: 1.0, z: 0.0 },
            attr_byte_count: 0,
        },
    ];
    let buf: [134]u8 = undefined;
    // Header deliberately starts with "solid " like many real-world CAD exporters
    const bin_tricky = writeStlBinary(tri[0..1], "solid misleading_binary_header", buf[0..134]) ?? unreachable;

    expect_eq(detectStlFormat(bin_tricky) == StlFormat.binary, true);
    expect_eq(detectStlFormat(SAMPLE_ASCII_STL) == StlFormat.ascii, true);
    expect_eq(detectStlFormat("random garbage") == StlFormat.unknown, true);
});

test("3. computeStlBinaryBounds computes exact AABB min/max across all triangle vertices", () => {
    const tris: [2]StlTriangle = [
        StlTriangle {
            normal: StlVec3 { x: 0.0, y: 0.0, z: 1.0 },
            v1: StlVec3 { x: -10.0, y: 2.0, z: 0.5 },
            v2: StlVec3 { x: 4.0, y: -3.5, z: 1.0 },
            v3: StlVec3 { x: 0.0, y: 8.0, z: -6.0 },
            attr_byte_count: 0,
        },
        StlTriangle {
            normal: StlVec3 { x: 0.0, y: 1.0, z: 0.0 },
            v1: StlVec3 { x: 12.5, y: 0.0, z: 9.25 },
            v2: StlVec3 { x: -2.0, y: 1.0, z: 0.0 },
            v3: StlVec3 { x: 0.0, y: -1.0, z: 0.0 },
            attr_byte_count: 0,
        },
    ];
    let buf: [184]u8 = undefined;
    const encoded = writeStlBinary(tris[0..2], "aabb", buf[0..184]) ?? unreachable;

    const bounds = computeStlBinaryBounds(encoded) ?? unreachable;
    expect_eq(bounds.triangle_count, 2);
    expect_eq(bounds.min.x, -10.0);
    expect_eq(bounds.min.y, -3.5);
    expect_eq(bounds.min.z, -6.0);
    expect_eq(bounds.max.x, 12.5);
    expect_eq(bounds.max.y, 8.0);
    expect_eq(bounds.max.z, 9.25);
});

test("4. parseStlBinaryHeader and writeStlBinary guard against truncation, overflow, and small buffers", () => {
    // Header claiming 0xFFFFFFFF triangles -> overflow-safe rejection
    let bad: [84]u8 = undefined;
    let i: usize = 0;
    while (i < 80) {
        bad[i] = 0;
        i += 1;
    }
    bad[80] = 0xFF;
    bad[81] = 0xFF;
    bad[82] = 0xFF;
    bad[83] = 0xFF;
    expect_eq(parseStlBinaryHeader(bad[0..84]) == null, true);

    // Too short (< 84 bytes)
    expect_eq(parseStlBinaryHeader(bad[0..40]) == null, true);

    // writeStlBinary with insufficient output buffer returns null
    const tri: [1]StlTriangle = [
        StlTriangle {
            normal: StlVec3 { x: 0.0, y: 0.0, z: 1.0 },
            v1: StlVec3 { x: 0.0, y: 0.0, z: 0.0 },
            v2: StlVec3 { x: 1.0, y: 0.0, z: 0.0 },
            v3: StlVec3 { x: 0.0, y: 1.0, z: 0.0 },
            attr_byte_count: 0,
        },
    ];
    let small_out: [100]u8 = undefined; // needs 134 bytes
    expect_eq(writeStlBinary(tri[0..1], "hdr", small_out[0..100]) == null, true);
});

test("5. countAsciiStlFacets counts case-insensitive 'facet normal' clauses cleanly", () => {
    expect_eq(countAsciiStlFacets(SAMPLE_ASCII_STL), 2);
    expect_eq(countAsciiStlFacets("solid empty\nendsolid empty\n"), 0);
    expect_eq(countAsciiStlFacets("facet   \t\n  normal 0 0 1 endfacet facet bad"), 1);
});
