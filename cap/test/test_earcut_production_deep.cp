// Source: cp/cap/test/test_earcut_production_deep.cp (Production deep tests for cap:earcut)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Point2D,
    TriangleIdx,
    signedArea,
    polygonArea,
    isCounterClockwise,
    isConvex,
    pointInTriangle,
    pointInPolygon,
    triangulate,
    triangulationTotalArea,
} from "cap:earcut";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn triSignedArea(pts: []const Point2D, t: TriangleIdx): f64 {
    const a = pts[t.a];
    const b = pts[t.b];
    const c = pts[t.c];
    return 0.5 * ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
}

test("1. Convex triangle, square & pentagon triangulation with automatic CW/CCW normalization", () => {
    // CW square (side 4 -> signedArea = -16, polygonArea = 16)
    const sq_cw: [4]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 0.0, y: 4.0 },
        Point2D { x: 4.0, y: 4.0 },
        Point2D { x: 4.0, y: 0.0 },
    ];
    expect_eq(absF64(signedArea(sq_cw[0..]) + 16.0) < 1.0e-12, true);
    expect_eq(absF64(polygonArea(sq_cw[0..]) - 16.0) < 1.0e-12, true);
    expect_eq(isCounterClockwise(sq_cw[0..]), false);
    expect_eq(isConvex(sq_cw[0..]), true);

    let tris4: [2]TriangleIdx = std.mem.zeroes([2]TriangleIdx);
    let scratch4: [4]usize = std.mem.zeroes([4]usize);
    const n4 = triangulate(sq_cw[0..], tris4[0..], scratch4[0..]) ?? unreachable;
    expect_eq(n4, 2 as usize);
    // Output triangles must be positively oriented (CCW) even for CW input
    expect_eq(triSignedArea(sq_cw[0..], tris4[0]) > 0.0, true);
    expect_eq(triSignedArea(sq_cw[0..], tris4[1]) > 0.0, true);
    expect_eq(absF64(triangulationTotalArea(sq_cw[0..], tris4[0..n4]) - 16.0) < 1.0e-11, true);

    // CCW convex pentagon
    const pent: [5]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 3.0, y: 0.0 },
        Point2D { x: 4.0, y: 2.0 },
        Point2D { x: 1.5, y: 4.0 },
        Point2D { x: -1.0, y: 2.0 },
    ];
    expect_eq(isCounterClockwise(pent[0..]), true);
    expect_eq(isConvex(pent[0..]), true);

    let tris5: [3]TriangleIdx = std.mem.zeroes([3]TriangleIdx);
    let scratch5: [5]usize = std.mem.zeroes([5]usize);
    const n5 = triangulate(pent[0..], tris5[0..], scratch5[0..]) ?? unreachable;
    expect_eq(n5, 3 as usize);
    expect_eq(absF64(triangulationTotalArea(pent[0..], tris5[0..n5]) - polygonArea(pent[0..])) < 1.0e-11, true);
});

test("2. Concave L-shape & arrowhead ear-clipping triangulation with area conservation", () => {
    // L-shape (6 vertices): 4x4 square minus 2x2 top-right cutout -> exact area = 12.0
    const l_shape: [6]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 4.0, y: 0.0 },
        Point2D { x: 4.0, y: 2.0 },
        Point2D { x: 2.0, y: 2.0 },
        Point2D { x: 2.0, y: 4.0 },
        Point2D { x: 0.0, y: 4.0 },
    ];
    expect_eq(isConvex(l_shape[0..]), false);
    expect_eq(absF64(polygonArea(l_shape[0..]) - 12.0) < 1.0e-12, true);

    let tris_l: [4]TriangleIdx = std.mem.zeroes([4]TriangleIdx);
    let scratch_l: [6]usize = std.mem.zeroes([6]usize);
    const nl = triangulate(l_shape[0..], tris_l[0..], scratch_l[0..]) ?? unreachable;
    expect_eq(nl, 4 as usize);
    let i: usize = 0;
    while (i < nl) {
        expect_eq(triSignedArea(l_shape[0..], tris_l[i]) > 0.0, true);
        i += 1;
    }
    expect_eq(absF64(triangulationTotalArea(l_shape[0..], tris_l[0..nl]) - 12.0) < 1.0e-11, true);

    // Concave arrowhead (4 vertices)
    const arrow: [4]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 4.0, y: 2.0 },
        Point2D { x: 0.0, y: 4.0 },
        Point2D { x: 1.5, y: 2.0 },
    ];
    expect_eq(isConvex(arrow[0..]), false);
    let tris_a: [2]TriangleIdx = std.mem.zeroes([2]TriangleIdx);
    let scratch_a: [4]usize = std.mem.zeroes([4]usize);
    const na = triangulate(arrow[0..], tris_a[0..], scratch_a[0..]) ?? unreachable;
    expect_eq(na, 2 as usize);
    expect_eq(absF64(triangulationTotalArea(arrow[0..], tris_a[0..na]) - polygonArea(arrow[0..])) < 1.0e-11, true);
});

test("3. 10-vertex 5-pointed star concave polygon triangulation (8 CCW triangles, area exact)", () => {
    // Regular 5-pointed star: outer radius 10, inner radius 4
    const two_pi: f64 = 6.28318530717958647692;
    let star: [10]Point2D = std.mem.zeroes([10]Point2D);
    let i: usize = 0;
    while (i < 10) {
        const angle = two_pi * (i as f64) / 10.0;
        const r: f64 = if ((i % 2) == 0) 10.0 else 4.0;
        star[i] = Point2D {
            x: r * std.math.cos(angle),
            y: r * std.math.sin(angle),
        };
        i += 1;
    }

    expect_eq(isConvex(star[0..]), false);
    expect_eq(isCounterClockwise(star[0..]), true);

    let tris: [8]TriangleIdx = std.mem.zeroes([8]TriangleIdx);
    let scratch: [10]usize = std.mem.zeroes([10]usize);
    const ntris = triangulate(star[0..], tris[0..], scratch[0..]) ?? unreachable;
    expect_eq(ntris, 8 as usize);

    let k: usize = 0;
    while (k < ntris) {
        expect_eq(triSignedArea(star[0..], tris[k]) > 0.0, true);
        k += 1;
    }

    const poly_area = polygonArea(star[0..]);
    const tri_area = triangulationTotalArea(star[0..], tris[0..ntris]);
    expect_eq(absF64(tri_area - poly_area) < 1.0e-11, true);
});

test("4. pointInTriangle & pointInPolygon ray-casting on concave L-shape", () => {
    const a = Point2D { x: 0.0, y: 0.0 };
    const b = Point2D { x: 4.0, y: 0.0 };
    const c = Point2D { x: 0.0, y: 4.0 };
    expect_eq(pointInTriangle(Point2D { x: 1.0, y: 1.0 }, a, b, c), true);
    expect_eq(pointInTriangle(Point2D { x: 2.0, y: 2.0 }, a, b, c), true);  // on hypotenuse
    expect_eq(pointInTriangle(Point2D { x: 3.0, y: 3.0 }, a, b, c), false); // outside

    const l_shape: [6]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 4.0, y: 0.0 },
        Point2D { x: 4.0, y: 2.0 },
        Point2D { x: 2.0, y: 2.0 },
        Point2D { x: 2.0, y: 4.0 },
        Point2D { x: 0.0, y: 4.0 },
    ];
    expect_eq(pointInPolygon(Point2D { x: 1.0, y: 1.0 }, l_shape[0..]), true);
    expect_eq(pointInPolygon(Point2D { x: 3.0, y: 1.0 }, l_shape[0..]), true);
    expect_eq(pointInPolygon(Point2D { x: 1.0, y: 3.0 }, l_shape[0..]), true);
    expect_eq(pointInPolygon(Point2D { x: 3.0, y: 3.0 }, l_shape[0..]), false); // inside concave notch!
    expect_eq(pointInPolygon(Point2D { x: -1.0, y: 2.0 }, l_shape[0..]), false);
});

test("5. Rejection of < 3 vertices, collinear zero-area polygons & insufficient output/scratch buffers", () => {
    const two_pts: [2]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 1.0, y: 1.0 },
    ];
    let tris: [4]TriangleIdx = std.mem.zeroes([4]TriangleIdx);
    let scratch: [4]usize = std.mem.zeroes([4]usize);
    expect_eq(triangulate(two_pts[0..], tris[0..], scratch[0..]) == null, true);
    expect_eq(isConvex(two_pts[0..]), false);
    expect_eq(pointInPolygon(Point2D { x: 0.5, y: 0.5 }, two_pts[0..]), false);

    // Collinear zero-area triangle
    const collinear: [3]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 2.0, y: 2.0 },
        Point2D { x: 4.0, y: 4.0 },
    ];
    expect_eq(isConvex(collinear[0..]), false);
    expect_eq(triangulate(collinear[0..], tris[0..], scratch[0..]) == null, true);

    // Insufficient out_tris or scratch_idx buffer
    const quad: [4]Point2D = [
        Point2D { x: 0.0, y: 0.0 },
        Point2D { x: 2.0, y: 0.0 },
        Point2D { x: 2.0, y: 2.0 },
        Point2D { x: 0.0, y: 2.0 },
    ];
    let short_tris: [1]TriangleIdx = std.mem.zeroes([1]TriangleIdx);
    let short_scratch: [3]usize = std.mem.zeroes([3]usize);
    expect_eq(triangulate(quad[0..], short_tris[0..], scratch[0..]) == null, true);
    expect_eq(triangulate(quad[0..], tris[0..], short_scratch[0..]) == null, true);
});
