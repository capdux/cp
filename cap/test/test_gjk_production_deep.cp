// Source: cp/cap/test/test_gjk_production_deep.cp (Production deep tests for cap:gjk)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Vec2,
    SatContact,
    GjkResult,
    polygonCentroid,
    translatePolygon,
    polygonSupport,
    minkowskiSupport,
    gjkIntersects,
    satCollide,
    circlePolygonCollide,
} from "cap:gjk";

fn absF(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    return absF(a - b) <= eps;
}

test("1. GJK vs SAT 100% agreement across separated, overlapping, contained & rotated convex polygons", () => {
    const sq_a: [4]Vec2 = [
        Vec2 { x: 0.0, y: 0.0 },
        Vec2 { x: 4.0, y: 0.0 },
        Vec2 { x: 4.0, y: 4.0 },
        Vec2 { x: 0.0, y: 4.0 },
    ];

    // Separated square
    const sq_far: [4]Vec2 = [
        Vec2 { x: 5.0, y: 0.0 },
        Vec2 { x: 8.0, y: 0.0 },
        Vec2 { x: 8.0, y: 3.0 },
        Vec2 { x: 5.0, y: 3.0 },
    ];
    expect_eq(gjkIntersects(sq_a[0..], sq_far[0..]).colliding, false);
    expect_eq(satCollide(sq_a[0..], sq_far[0..]).colliding, false);

    // Overlapping square
    const sq_hit: [4]Vec2 = [
        Vec2 { x: 3.2, y: 1.0 },
        Vec2 { x: 6.2, y: 1.0 },
        Vec2 { x: 6.2, y: 3.0 },
        Vec2 { x: 3.2, y: 3.0 },
    ];
    expect_eq(gjkIntersects(sq_a[0..], sq_hit[0..]).colliding, true);
    const hit_sat = satCollide(sq_a[0..], sq_hit[0..]);
    expect_eq(hit_sat.colliding, true);
    expect_eq(approxEq(hit_sat.depth, 0.8, 1.0e-9), true);
    expect_eq(approxEq(hit_sat.normal.x, 1.0, 1.0e-9), true);
    expect_eq(approxEq(hit_sat.normal.y, 0.0, 1.0e-9), true);

    // Completely contained inner triangle
    const inner_tri: [3]Vec2 = [
        Vec2 { x: 1.0, y: 1.0 },
        Vec2 { x: 3.0, y: 1.0 },
        Vec2 { x: 2.0, y: 3.0 },
    ];
    expect_eq(gjkIntersects(sq_a[0..], inner_tri[0..]).colliding, true);
    expect_eq(satCollide(sq_a[0..], inner_tri[0..]).colliding, true);

    // 45-degree rotated diamond overlapping sq_a corner (4, 4) vs separated diamond
    const diam_hit: [4]Vec2 = [
        Vec2 { x: 4.5, y: 3.0 },
        Vec2 { x: 5.5, y: 4.0 },
        Vec2 { x: 4.5, y: 5.0 },
        Vec2 { x: 3.5, y: 4.0 },
    ];
    expect_eq(gjkIntersects(sq_a[0..], diam_hit[0..]).colliding, true);
    expect_eq(satCollide(sq_a[0..], diam_hit[0..]).colliding, true);

    const diam_miss: [4]Vec2 = [
        Vec2 { x: 5.2, y: 3.0 },
        Vec2 { x: 6.2, y: 4.0 },
        Vec2 { x: 5.2, y: 5.0 },
        Vec2 { x: 4.2, y: 4.0 },
    ];
    expect_eq(gjkIntersects(sq_a[0..], diam_miss[0..]).colliding, false);
    expect_eq(satCollide(sq_a[0..], diam_miss[0..]).colliding, false);
});

test("2. SAT MTV resolution separates colliding convex polygons cleanly (both axis-aligned & rotated)", () => {
    const poly_a: [4]Vec2 = [
        Vec2 { x: 0.0, y: 0.0 },
        Vec2 { x: 4.0, y: 0.0 },
        Vec2 { x: 4.0, y: 4.0 },
        Vec2 { x: 0.0, y: 4.0 },
    ];

    // Rotated triangle penetrating poly_a
    const poly_b: [3]Vec2 = [
        Vec2 { x: 3.3, y: 2.5 },
        Vec2 { x: 5.5, y: 1.2 },
        Vec2 { x: 5.8, y: 3.9 },
    ];

    const c1 = satCollide(poly_a[0..], poly_b[0..]);
    expect_eq(c1.colliding, true);
    expect_eq(c1.depth > 0.0, true);
    expect_eq(approxEq(c1.normal.len(), 1.0, 1.0e-12), true);

    // Push B by mtv() + epsilon along normal -> must strictly stop colliding in BOTH SAT & GJK
    const push = c1.mtv().add(c1.normal.scale(1.0e-6));
    let resolved_b: [3]Vec2 = std.mem.zeroes([3]Vec2);
    expect_eq(translatePolygon(poly_b[0..], push, resolved_b[0..]), true);

    expect_eq(satCollide(poly_a[0..], resolved_b[0..]).colliding, false);
    expect_eq(gjkIntersects(poly_a[0..], resolved_b[0..]).colliding, false);
});

test("3. circlePolygonCollide face contact, vertex contact & separation exact penetration depth", () => {
    const sq: [4]Vec2 = [
        Vec2 { x: 0.6, y: -1.0 },
        Vec2 { x: 2.6, y: -1.0 },
        Vec2 { x: 2.6, y: 1.0 },
        Vec2 { x: 0.6, y: 1.0 },
    ];

    // Face-to-circle contact: center=(0,0), r=1.0 -> overlaps left edge x=0.6 by 0.4
    const face = circlePolygonCollide(Vec2 { x: 0.0, y: 0.0 }, 1.0, sq[0..]);
    expect_eq(face.colliding, true);
    expect_eq(approxEq(face.depth, 0.4, 1.0e-9), true);
    expect_eq(approxEq(face.normal.x, 1.0, 1.0e-9), true);
    expect_eq(approxEq(face.normal.y, 0.0, 1.0e-9), true);

    // Vertex-to-circle contact: square shifted to [0.6, 2.6] x [0.6, 2.6], closest vertex (0.6, 0.6)
    const sq_v: [4]Vec2 = [
        Vec2 { x: 0.6, y: 0.6 },
        Vec2 { x: 2.6, y: 0.6 },
        Vec2 { x: 2.6, y: 2.6 },
        Vec2 { x: 0.6, y: 2.6 },
    ];
    const vert = circlePolygonCollide(Vec2 { x: 0.0, y: 0.0 }, 1.0, sq_v[0..]);
    const expected_depth = 1.0 - std.math.sqrt(0.72);
    expect_eq(vert.colliding, true);
    expect_eq(approxEq(vert.depth, expected_depth, 1.0e-9), true);

    // Separated circle: center=(0,0), r=0.8 < sqrt(0.72) ≈ 0.8485
    const sep = circlePolygonCollide(Vec2 { x: 0.0, y: 0.0 }, 0.8, sq_v[0..]);
    expect_eq(sep.colliding, false);
    expect_eq(approxEq(sep.mtv().len(), 0.0, 1.0e-12), true);
});

test("4. Boundary tangency, shared vertex & identical coincident polygons converge safely", () => {
    const a: [4]Vec2 = [
        Vec2 { x: 0.0, y: 0.0 },
        Vec2 { x: 2.0, y: 0.0 },
        Vec2 { x: 2.0, y: 2.0 },
        Vec2 { x: 0.0, y: 2.0 },
    ];

    // Identical coincident polygons
    const g_same = gjkIntersects(a[0..], a[0..]);
    const s_same = satCollide(a[0..], a[0..]);
    expect_eq(g_same.colliding, true);
    expect_eq(g_same.iterations <= 64, true);
    expect_eq(s_same.colliding, true);
    expect_eq(approxEq(s_same.depth, 2.0, 1.0e-9), true);

    // Shared edge tangency (x = 2.0)
    const tangent: [4]Vec2 = [
        Vec2 { x: 2.0, y: 0.0 },
        Vec2 { x: 4.0, y: 0.0 },
        Vec2 { x: 4.0, y: 2.0 },
        Vec2 { x: 2.0, y: 2.0 },
    ];
    const s_tan = satCollide(a[0..], tangent[0..]);
    expect_eq(s_tan.colliding, true);
    expect_eq(approxEq(s_tan.depth, 0.0, 1.0e-9), true);
    expect_eq(gjkIntersects(a[0..], tangent[0..]).colliding, true);
});

test("5. polygonSupport, minkowskiSupport & polygonCentroid exact geometric verification", () => {
    const tri: [3]Vec2 = [
        Vec2 { x: 0.0, y: 0.0 },
        Vec2 { x: 6.0, y: 0.0 },
        Vec2 { x: 3.0, y: 9.0 },
    ];

    const c = polygonCentroid(tri[0..]) ?? unreachable;
    expect_eq(approxEq(c.x, 3.0, 1.0e-12), true);
    expect_eq(approxEq(c.y, 3.0, 1.0e-12), true);

    const sup_up = polygonSupport(tri[0..], Vec2 { x: 0.0, y: 1.0 }) ?? unreachable;
    expect_eq(approxEq(sup_up.x, 3.0, 1.0e-12), true);
    expect_eq(approxEq(sup_up.y, 9.0, 1.0e-12), true);

    const b: [3]Vec2 = [
        Vec2 { x: 1.0, y: 1.0 },
        Vec2 { x: 2.0, y: 1.0 },
        Vec2 { x: 1.0, y: 2.0 },
    ];
    // minkowskiSupport(tri, b, (1, 0)) = (6, 0) - (1, 1 or 2) => x = 5.0
    const m = minkowskiSupport(tri[0..], b[0..], Vec2 { x: 1.0, y: 0.0 }) ?? unreachable;
    expect_eq(approxEq(m.x, 5.0, 1.0e-12), true);
});

test("6. Empty/degenerate (< 3 vertices), negative radius & small dst buffer safety guards", () => {
    const deg: [2]Vec2 = [Vec2 { x: 0.0, y: 0.0 }, Vec2 { x: 1.0, y: 0.0 }];
    const tri: [3]Vec2 = [
        Vec2 { x: 0.0, y: 0.0 },
        Vec2 { x: 2.0, y: 0.0 },
        Vec2 { x: 1.0, y: 2.0 },
    ];
    let dst: [2]Vec2 = std.mem.zeroes([2]Vec2);

    expect_eq(polygonCentroid(deg[0..0]) == null, true);
    expect_eq(polygonSupport(deg[0..0], Vec2 { x: 1.0, y: 0.0 }) == null, true);
    expect_eq(minkowskiSupport(deg[0..0], tri[0..], Vec2 { x: 1.0, y: 0.0 }) == null, true);

    expect_eq(gjkIntersects(deg[0..], tri[0..]).colliding, false);
    expect_eq(satCollide(deg[0..], tri[0..]).colliding, false);
    expect_eq(circlePolygonCollide(Vec2 { x: 0.0, y: 0.0 }, -1.0, tri[0..]).colliding, false);
    expect_eq(circlePolygonCollide(Vec2 { x: 0.0, y: 0.0 }, 1.0, deg[0..]).colliding, false);

    // translatePolygon buffer too small -> false
    expect_eq(translatePolygon(tri[0..], Vec2 { x: 1.0, y: 1.0 }, dst[0..]), false);
});
