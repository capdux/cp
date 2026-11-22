// Source: cp/cap/test/test_delaunay_production_deep.cp (Production deep tests for cap:delaunay)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Point2,
    Triangle,
    Edge2,
    Circumcircle,
    DelaunayWorkspace,
    delaunayMinTriPool,
    delaunayMinEdgePool,
    orient2d,
    circumcircle,
    inCircumcircle,
    pointInTriangle,
    triangulate,
    verifyDelaunayEmptyCircle,
    locateTriangle,
} from "cap:graph";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn totalMeshArea(points: []const Point2, tris: []const Triangle): f64 {
    let area2: f64 = 0.0;
    let i: usize = 0;
    while (i < tris.len) {
        const t: Triangle = tris[i];
        area2 += orient2d(points[t.a], points[t.b], points[t.c]);
        i += 1;
    }
    return 0.5 * area2;
}

test("1. Square, regular pentagon & interior-point convex hull exact triangle counts & area conservation", () => {
    let tri_pool: [64]Triangle = std.mem.zeroes([64]Triangle);
    let bad_pool: [64]u8 = std.mem.zeroes([64]u8);
    let edge_pool: [192]Edge2 = std.mem.zeroes([192]Edge2);
    let dup_pool: [192]u8 = std.mem.zeroes([192]u8);
    let ws: DelaunayWorkspace = DelaunayWorkspace {
        tris: tri_pool[0..],
        bad: bad_pool[0..],
        edges: edge_pool[0..],
        edge_dup: dup_pool[0..],
    };
    let out_buf: [32]Triangle = std.mem.zeroes([32]Triangle);
    let out: []Triangle = out_buf[0..];

    // (a) Unit square (4 cocircular vertices) -> 2 triangles, area == 1.0
    const sq: [4]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 1.0, y: 0.0 },
        Point2 { x: 1.0, y: 1.0 },
        Point2 { x: 0.0, y: 1.0 },
    ];
    const sq_cnt: usize = triangulate(sq[0..], *out, *ws) ?? unreachable;
    expect_eq(sq_cnt, 2 as usize);
    expect_eq(absF64(totalMeshArea(sq[0..], out[0..sq_cnt]) - 1.0) < 1.0e-12, true);
    expect_eq(verifyDelaunayEmptyCircle(sq[0..], out[0..sq_cnt], 1.0e-9), true);

    // (b) Unit square + center point (5 points, 4 hull + 1 interior) -> 4 triangles, area == 1.0
    const sq_mid: [5]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 1.0, y: 0.0 },
        Point2 { x: 1.0, y: 1.0 },
        Point2 { x: 0.0, y: 1.0 },
        Point2 { x: 0.5, y: 0.5 },
    ];
    const mid_cnt: usize = triangulate(sq_mid[0..], *out, *ws) ?? unreachable;
    expect_eq(mid_cnt, 4 as usize);
    expect_eq(absF64(totalMeshArea(sq_mid[0..], out[0..mid_cnt]) - 1.0) < 1.0e-12, true);
    expect_eq(verifyDelaunayEmptyCircle(sq_mid[0..], out[0..mid_cnt], 1.0e-9), true);

    // (c) Regular pentagon on unit circle -> 3 triangles, area == 2.5 * sin(2*pi/5)
    const two_pi_5: f64 = 2.0 * 3.14159265358979323846 / 5.0;
    let pent: [5]Point2 = std.mem.zeroes([5]Point2);
    let k: usize = 0;
    while (k < 5) {
        const ang: f64 = (k as f64) * two_pi_5;
        pent[k] = Point2 { x: std.math.cos(ang), y: std.math.sin(ang) };
        k += 1;
    }
    const pent_cnt: usize = triangulate(pent[0..], *out, *ws) ?? unreachable;
    expect_eq(pent_cnt, 3 as usize);
    const expected_pent_area: f64 = 2.5 * std.math.sin(two_pi_5);
    expect_eq(absF64(totalMeshArea(pent[0..], out[0..pent_cnt]) - expected_pent_area) < 1.0e-11, true);
    expect_eq(verifyDelaunayEmptyCircle(pent[0..], out[0..pent_cnt], 1.0e-9), true);
});

test("2. 25 deterministic pseudo-random points pass full Delaunay empty-circumcircle verification", () => {
    const N: usize = 25;
    let pts: [25]Point2 = std.mem.zeroes([25]Point2);
    let state: u64 = 0xDE1A09A720261121;
    let i: usize = 0;
    while (i < N) {
        state = state +% 0x9E3779B97F4A7C15;
        let zx: u64 = (state ^ (state >> 30)) *% 0xBF58476D1CE4E5B9;
        zx = (zx ^ (zx >> 27)) *% 0x94D049BB133111EB;
        zx = zx ^ (zx >> 31);

        state = state +% 0x9E3779B97F4A7C15;
        let zy: u64 = (state ^ (state >> 30)) *% 0xBF58476D1CE4E5B9;
        zy = (zy ^ (zy >> 27)) *% 0x94D049BB133111EB;
        zy = zy ^ (zy >> 31);

        const fx: f64 = ((zx >> 11) as f64) / 9007199254740992.0;
        const fy: f64 = ((zy >> 11) as f64) / 9007199254740992.0;
        pts[i] = Point2 { x: fx * 100.0 - 50.0, y: fy * 100.0 - 50.0 };
        i += 1;
    }

    expect_eq(delaunayMinTriPool(N) <= 128, true);
    expect_eq(delaunayMinEdgePool(N) <= 384, true);

    let tri_pool: [128]Triangle = std.mem.zeroes([128]Triangle);
    let bad_pool: [128]u8 = std.mem.zeroes([128]u8);
    let edge_pool: [384]Edge2 = std.mem.zeroes([384]Edge2);
    let dup_pool: [384]u8 = std.mem.zeroes([384]u8);
    let ws: DelaunayWorkspace = DelaunayWorkspace {
        tris: tri_pool[0..],
        bad: bad_pool[0..],
        edges: edge_pool[0..],
        edge_dup: dup_pool[0..],
    };

    let out_buf: [64]Triangle = std.mem.zeroes([64]Triangle);
    let out: []Triangle = out_buf[0..];
    const cnt: usize = triangulate(pts[0..], *out, *ws) ?? unreachable;

    // For 25 general-position points, Euler bound: N - 2 <= cnt <= 2*N - 5
    expect_eq(cnt >= 23 && cnt <= 45, true);
    expect_eq(verifyDelaunayEmptyCircle(pts[0..], out[0..cnt], 1.0e-9), true);
    expect_eq(totalMeshArea(pts[0..], out[0..cnt]) > 0.0, true);
});

test("3. Duplicate & coincident points are skipped cleanly without corrupting triangulation", () => {
    const pts_with_dups: [8]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 4.0, y: 0.0 },
        Point2 { x: 4.0, y: 3.0 },
        Point2 { x: 0.0, y: 3.0 },
        Point2 { x: 0.0, y: 0.0 }, // duplicate of 0
        Point2 { x: 2.0, y: 1.5 }, // interior center
        Point2 { x: 4.0, y: 3.0 }, // duplicate of 2
        Point2 { x: 2.0, y: 1.5 }, // duplicate of 5
    ];

    let tri_pool: [64]Triangle = std.mem.zeroes([64]Triangle);
    let bad_pool: [64]u8 = std.mem.zeroes([64]u8);
    let edge_pool: [192]Edge2 = std.mem.zeroes([192]Edge2);
    let dup_pool: [192]u8 = std.mem.zeroes([192]u8);
    let ws: DelaunayWorkspace = DelaunayWorkspace {
        tris: tri_pool[0..],
        bad: bad_pool[0..],
        edges: edge_pool[0..],
        edge_dup: dup_pool[0..],
    };
    let out_buf: [32]Triangle = std.mem.zeroes([32]Triangle);
    let out: []Triangle = out_buf[0..];

    const cnt: usize = triangulate(pts_with_dups[0..], *out, *ws) ?? unreachable;
    expect_eq(cnt, 4 as usize);
    expect_eq(absF64(totalMeshArea(pts_with_dups[0..], out[0..cnt]) - 12.0) < 1.0e-11, true);
    expect_eq(verifyDelaunayEmptyCircle(pts_with_dups[0..], out[0..cnt], 1.0e-9), true);
});

test("4. Geometric predicates & locateTriangle for interior, boundary, vertex & exterior points", () => {
    const a: Point2 = Point2 { x: 0.0, y: 0.0 };
    const b: Point2 = Point2 { x: 2.0, y: 0.0 };
    const c: Point2 = Point2 { x: 0.0, y: 2.0 };

    // Right triangle circumcircle: center (1, 1), r^2 = 2
    const cc: Circumcircle = circumcircle(a, b, c) ?? unreachable;
    expect_eq(absF64(cc.cx - 1.0) < 1.0e-12, true);
    expect_eq(absF64(cc.cy - 1.0) < 1.0e-12, true);
    expect_eq(absF64(cc.radiusSq - 2.0) < 1.0e-12, true);

    expect_eq(inCircumcircle(a, b, c, Point2 { x: 1.0, y: 1.0 }), true);
    expect_eq(inCircumcircle(a, b, c, Point2 { x: 3.0, y: 3.0 }), false);
    expect_eq(pointInTriangle(a, b, c, Point2 { x: 0.5, y: 0.5 }), true);
    expect_eq(pointInTriangle(a, b, c, Point2 { x: 1.0, y: 1.0 }), true); // hypotenuse midpoint
    expect_eq(pointInTriangle(a, b, c, Point2 { x: 1.5, y: 1.5 }), false);

    // locateTriangle on triangulated 2x2 square with center
    const pts: [5]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 2.0, y: 0.0 },
        Point2 { x: 2.0, y: 2.0 },
        Point2 { x: 0.0, y: 2.0 },
        Point2 { x: 1.0, y: 1.0 },
    ];
    let tri_pool: [48]Triangle = std.mem.zeroes([48]Triangle);
    let bad_pool: [48]u8 = std.mem.zeroes([48]u8);
    let edge_pool: [144]Edge2 = std.mem.zeroes([144]Edge2);
    let dup_pool: [144]u8 = std.mem.zeroes([144]u8);
    let ws: DelaunayWorkspace = DelaunayWorkspace {
        tris: tri_pool[0..], bad: bad_pool[0..], edges: edge_pool[0..], edge_dup: dup_pool[0..],
    };
    let out_buf: [16]Triangle = std.mem.zeroes([16]Triangle);
    let out: []Triangle = out_buf[0..];
    const cnt: usize = triangulate(pts[0..], *out, *ws) ?? unreachable;

    // Interior, boundary edge midpoint, vertex -> all located
    const h_in: usize = locateTriangle(pts[0..], out[0..cnt], Point2 { x: 1.0, y: 0.25 }) ?? unreachable;
    expect_eq(h_in < cnt, true);
    const h_edge: usize = locateTriangle(pts[0..], out[0..cnt], Point2 { x: 2.0, y: 1.0 }) ?? unreachable;
    expect_eq(h_edge < cnt, true);
    const h_vtx: usize = locateTriangle(pts[0..], out[0..cnt], Point2 { x: 0.0, y: 0.0 }) ?? unreachable;
    expect_eq(h_vtx < cnt, true);

    // Exterior point -> null
    expect_eq(locateTriangle(pts[0..], out[0..cnt], Point2 { x: 2.5, y: 1.0 }) == null, true);
    expect_eq(locateTriangle(pts[0..], out[0..cnt], Point2 { x: -0.1, y: 0.5 }) == null, true);
});

test("5. Degenerate sets (< 3 points, coincident, all-collinear) return 0 triangles cleanly", () => {
    let tri_pool: [48]Triangle = std.mem.zeroes([48]Triangle);
    let bad_pool: [48]u8 = std.mem.zeroes([48]u8);
    let edge_pool: [144]Edge2 = std.mem.zeroes([144]Edge2);
    let dup_pool: [144]u8 = std.mem.zeroes([144]u8);
    let ws: DelaunayWorkspace = DelaunayWorkspace {
        tris: tri_pool[0..], bad: bad_pool[0..], edges: edge_pool[0..], edge_dup: dup_pool[0..],
    };
    let out_buf: [16]Triangle = std.mem.zeroes([16]Triangle);
    let out: []Triangle = out_buf[0..];

    const empty_pts: [0]Point2 = [];
    expect_eq(triangulate(empty_pts[0..], *out, *ws) ?? unreachable, 0 as usize);

    const two_pts: [2]Point2 = [Point2 { x: 0.0, y: 0.0 }, Point2 { x: 1.0, y: 1.0 }];
    expect_eq(triangulate(two_pts[0..], *out, *ws) ?? unreachable, 0 as usize);

    const collinear: [5]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 1.0, y: 2.0 },
        Point2 { x: 2.0, y: 4.0 },
        Point2 { x: 3.0, y: 6.0 },
        Point2 { x: 4.0, y: 8.0 },
    ];
    expect_eq(circumcircle(collinear[0], collinear[1], collinear[2]) == null, true);
    expect_eq(triangulate(collinear[0..], *out, *ws) ?? unreachable, 0 as usize);
});

test("6. Insufficient buffers & NaN/Inf coordinates safely rejected with null", () => {
    const sq: [4]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 1.0, y: 0.0 },
        Point2 { x: 1.0, y: 1.0 },
        Point2 { x: 0.0, y: 1.0 },
    ];
    let tri_pool: [32]Triangle = std.mem.zeroes([32]Triangle);
    let bad_pool: [32]u8 = std.mem.zeroes([32]u8);
    let edge_pool: [96]Edge2 = std.mem.zeroes([96]Edge2);
    let dup_pool: [96]u8 = std.mem.zeroes([96]u8);
    let ws: DelaunayWorkspace = DelaunayWorkspace {
        tris: tri_pool[0..], bad: bad_pool[0..], edges: edge_pool[0..], edge_dup: dup_pool[0..],
    };

    // Output buffer too small (needs 2, given 1)
    let tiny_out_buf: [1]Triangle = std.mem.zeroes([1]Triangle);
    let tiny_out: []Triangle = tiny_out_buf[0..];
    expect_eq(triangulate(sq[0..], *tiny_out, *ws) == null, true);

    // Workspace too small
    let tiny_ws: DelaunayWorkspace = DelaunayWorkspace {
        tris: tri_pool[0..2], bad: bad_pool[0..2], edges: edge_pool[0..3], edge_dup: dup_pool[0..3],
    };
    let ok_out_buf: [8]Triangle = std.mem.zeroes([8]Triangle);
    let ok_out: []Triangle = ok_out_buf[0..];
    expect_eq(triangulate(sq[0..], *ok_out, *tiny_ws) == null, true);

    // NaN & Inf coordinates
    const zero: f64 = 0.0;
    const nan_pts: [3]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: zero / zero, y: 0.0 },
        Point2 { x: 0.0, y: 1.0 },
    ];
    const inf_pts: [3]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 1.0 / zero, y: 0.0 },
        Point2 { x: 0.0, y: 1.0 },
    ];
    expect_eq(triangulate(nan_pts[0..], *ok_out, *ws) == null, true);
    expect_eq(triangulate(inf_pts[0..], *ok_out, *ws) == null, true);
});
