// Reference-hardened test suite against donbright/earcutr (ISC) & dimforge/parry (Apache-2.0)
// Covers convex/concave/holed/degenerate/NaN Earcut triangulation & area conservation, GJK
// disjoint/overlapping/contained/symmetry/degenerate guards, 4-lane @Vector(4, f64) SIMD 0-diff
// parity, and 1,600 deterministic PRNG fuzz cases (800 Earcut + 800 GJK).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { Vec2 } from "../../lib/gfx/bezier.cp";
import {
    Point2D,
    TriangleIdx,
    simdPolyBounds2d,
    scalarPolyBounds2d,
    polygonArea,
    triangulate,
    triangulateWithHoles,
    triangulationTotalArea,
} from "../../lib/gfx/earcut.cp";
import {
    simdGjkSupportIndex,
    scalarGjkSupportIndex,
    polygonSupport,
    minkowskiSupport,
    gjkIntersects,
    satCollide,
    circlePolygonCollide,
} from "../../lib/gfx/gjk.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextRangeF64(*_, lo: f64, hi: f64): f64 {
        const u = ((_.next() >> 11) as f64) / 9007199254740992.0;
        return lo + u * (hi - lo);
    }
}

fn checkEarcutSuite(): u32 {
    let tris: [64]TriangleIdx = [TriangleIdx{ a: 0, b: 0, c: 0 }; 64];
    let scratch: [64]usize = [0; 64];

    // 1. Convex square (CCW & CW) area conservation
    const sq: [4]Point2D = [
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: 10.0, y: 0.0 },
        Point2D{ x: 10.0, y: 10.0 },
        Point2D{ x: 0.0, y: 10.0 },
    ];
    const n_sq = triangulate(sq[0..], tris[0..], scratch[0..]) ?? return 1;
    if (n_sq != 2 || @abs(triangulationTotalArea(sq[0..], tris[0..n_sq]) - 100.0) > 1.0e-9) return 2;

    // 2. Concave L-shape polygon area conservation (10x10 minus 5x5 = 75)
    const lshape: [6]Point2D = [
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: 10.0, y: 0.0 },
        Point2D{ x: 10.0, y: 5.0 },
        Point2D{ x: 5.0, y: 5.0 },
        Point2D{ x: 5.0, y: 10.0 },
        Point2D{ x: 0.0, y: 10.0 },
    ];
    const n_l = triangulate(lshape[0..], tris[0..], scratch[0..]) ?? return 3;
    if (n_l != 4 || @abs(triangulationTotalArea(lshape[0..], tris[0..n_l]) - 75.0) > 1.0e-9) return 4;
    let i: usize = 0;
    while (i < n_l) : (i += 1) {
        if (tris[i].a >= lshape.len || tris[i].b >= lshape.len || tris[i].c >= lshape.len) return 5;
    }

    // 3. Polygon with a square hole (outer 10x10 = 100, hole 4x4 = 16 -> area = 84)
    const holed: [8]Point2D = [
        // Outer CCW
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: 10.0, y: 0.0 },
        Point2D{ x: 10.0, y: 10.0 },
        Point2D{ x: 0.0, y: 10.0 },
        // Hole CW (3..7)
        Point2D{ x: 3.0, y: 3.0 },
        Point2D{ x: 3.0, y: 7.0 },
        Point2D{ x: 7.0, y: 7.0 },
        Point2D{ x: 7.0, y: 3.0 },
    ];
    const holes: [1]usize = [4];
    const n_h = triangulateWithHoles(holed[0..], holes[0..], tris[0..], scratch[0..]) ?? return 6;
    const h_area = triangulationTotalArea(holed[0..], tris[0..n_h]);
    if (@abs(h_area - 84.0) > 1.0e-7) return 7;
    i = 0;
    while (i < n_h) : (i += 1) {
        if (tris[i].a >= holed.len || tris[i].b >= holed.len || tris[i].c >= holed.len) return 8;
    }

    // 4. Reject bad hole_indices (out-of-bounds, unsorted, < 3 outer/hole verts)
    const bad_h1: [1]usize = [2];
    const bad_h2: [2]usize = [6, 4];
    const bad_h3: [1]usize = [7];
    if (triangulateWithHoles(holed[0..], bad_h1[0..], tris[0..], scratch[0..]) != null) return 9;
    if (triangulateWithHoles(holed[0..], bad_h2[0..], tris[0..], scratch[0..]) != null) return 10;
    if (triangulateWithHoles(holed[0..], bad_h3[0..], tris[0..], scratch[0..]) != null) return 11;

    // 5. Reject NaN, Inf, < 3 verts, collinear degenerate polygon
    const nan_poly: [3]Point2D = [
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: std.math.nan(f64), y: 1.0 },
        Point2D{ x: 1.0, y: 0.0 },
    ];
    const inf_poly: [3]Point2D = [
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: std.math.inf(f64), y: 1.0 },
        Point2D{ x: 1.0, y: 0.0 },
    ];
    const col_poly: [4]Point2D = [
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: 1.0, y: 1.0 },
        Point2D{ x: 2.0, y: 2.0 },
        Point2D{ x: 3.0, y: 3.0 },
    ];
    if (triangulate(nan_poly[0..], tris[0..], scratch[0..]) != null) return 12;
    if (triangulate(inf_poly[0..], tris[0..], scratch[0..]) != null) return 13;
    if (triangulate(col_poly[0..], tris[0..], scratch[0..]) != null) return 14;
    if (triangulate(sq[0..2], tris[0..], scratch[0..]) != null) return 15;

    // 6. Duplicate consecutive vertices stripped cleanly without infinite loop
    const dup_sq: [6]Point2D = [
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: 0.0, y: 0.0 },
        Point2D{ x: 4.0, y: 0.0 },
        Point2D{ x: 4.0, y: 4.0 },
        Point2D{ x: 4.0, y: 4.0 },
        Point2D{ x: 0.0, y: 4.0 },
    ];
    const n_dup = triangulate(dup_sq[0..], tris[0..], scratch[0..]) ?? return 16;
    if (@abs(triangulationTotalArea(dup_sq[0..], tris[0..n_dup]) - 16.0) > 1.0e-9) return 17;

    return 0;
}

fn checkGjkSuite(): u32 {
    const box_a: [4]Vec2 = [
        Vec2{ x: 0.0, y: 0.0 },
        Vec2{ x: 4.0, y: 0.0 },
        Vec2{ x: 4.0, y: 4.0 },
        Vec2{ x: 0.0, y: 4.0 },
    ];
    const box_overlap: [4]Vec2 = [
        Vec2{ x: 2.0, y: 2.0 },
        Vec2{ x: 6.0, y: 2.0 },
        Vec2{ x: 6.0, y: 6.0 },
        Vec2{ x: 2.0, y: 6.0 },
    ];
    const box_inside: [4]Vec2 = [
        Vec2{ x: 1.0, y: 1.0 },
        Vec2{ x: 3.0, y: 1.0 },
        Vec2{ x: 3.0, y: 3.0 },
        Vec2{ x: 1.0, y: 3.0 },
    ];
    const box_disjoint: [4]Vec2 = [
        Vec2{ x: 10.0, y: 10.0 },
        Vec2{ x: 12.0, y: 10.0 },
        Vec2{ x: 12.0, y: 12.0 },
        Vec2{ x: 10.0, y: 12.0 },
    ];

    // 1. Overlapping, contained, disjoint & strict symmetry
    if (!gjkIntersects(box_a[0..], box_overlap[0..]).colliding) return 1;
    if (!gjkIntersects(box_overlap[0..], box_a[0..]).colliding) return 2;
    if (!gjkIntersects(box_a[0..], box_inside[0..]).colliding) return 3;
    if (!gjkIntersects(box_inside[0..], box_a[0..]).colliding) return 4;
    if (gjkIntersects(box_a[0..], box_disjoint[0..]).colliding) return 5;
    if (gjkIntersects(box_disjoint[0..], box_a[0..]).colliding) return 6;

    // 2. Guard NaN, Inf, empty, zero direction, degenerate point-cloud simplex
    const nan_box: [3]Vec2 = [
        Vec2{ x: std.math.nan(f64), y: 0.0 },
        Vec2{ x: 1.0, y: 0.0 },
        Vec2{ x: 0.0, y: 1.0 },
    ];
    const inf_box: [3]Vec2 = [
        Vec2{ x: std.math.inf(f64), y: 0.0 },
        Vec2{ x: 1.0, y: 0.0 },
        Vec2{ x: 0.0, y: 1.0 },
    ];
    const degen_pt: [4]Vec2 = [
        Vec2{ x: 2.0, y: 2.0 },
        Vec2{ x: 2.0, y: 2.0 },
        Vec2{ x: 2.0, y: 2.0 },
        Vec2{ x: 2.0, y: 2.0 },
    ];
    if (gjkIntersects(box_a[0..], nan_box[0..]).colliding) return 7;
    if (gjkIntersects(box_a[0..], inf_box[0..]).colliding) return 8;
    if (gjkIntersects(box_a[0..0], box_a[0..]).colliding) return 9;
    if (minkowskiSupport(box_a[0..], box_overlap[0..], Vec2{ x: 0.0, y: 0.0 }) != null) return 10;
    if (polygonSupport(nan_box[0..], Vec2{ x: 1.0, y: 0.0 }) != null) return 11;
    if (satCollide(box_a[0..], nan_box[0..]).colliding) return 12;
    if (circlePolygonCollide(Vec2{ x: std.math.nan(f64), y: 0.0 }, 1.0, box_a[0..]).colliding) return 13;

    // Degenerate point inside box must terminate safely without infinite loop
    * = gjkIntersects(box_a[0..], degen_pt[0..]);

    return 0;
}

fn checkSimdDifferential(): u32 {
    let prng = XorShift64.init(0x4745_4f4d_5349_4d44);
    let pts: [132]Point2D = [Point2D{ x: 0.0, y: 0.0 }; 132];
    let vecs: [132]Vec2 = [Vec2{ x: 0.0, y: 0.0 }; 132];

    let len: usize = 0;
    while (len <= 131) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            const x = prng.nextRangeF64(-500.0, 500.0);
            const y = prng.nextRangeF64(-500.0, 500.0);
            pts[i] = Point2D{ x: x, y: y };
            vecs[i] = Vec2{ x: x, y: y };
        }
        const b1 = simdPolyBounds2d(pts[0..len]);
        const b2 = scalarPolyBounds2d(pts[0..len]);
        if (b1.valid != b2.valid) return 1;
        if (b1.valid && (b1.min_x != b2.min_x || b1.max_x != b2.max_x || b1.min_y != b2.min_y || b1.max_y != b2.max_y)) {
            return 2;
        }

        const dir = Vec2{ x: prng.nextRangeF64(-10.0, 10.0), y: prng.nextRangeF64(-10.0, 10.0) };
        if (simdGjkSupportIndex(vecs[0..len], dir) != scalarGjkSupportIndex(vecs[0..len], dir)) return 3;
    }
    return 0;
}

fn checkDeterministicFuzz(): u32 {
    let prng = XorShift64.init(0x2026_1124_4745_4f4d);
    let pts: [16]Point2D = [Point2D{ x: 0.0, y: 0.0 }; 16];
    let tris: [32]TriangleIdx = [TriangleIdx{ a: 0, b: 0, c: 0 }; 32];
    let scratch: [32]usize = [0; 32];

    // 1. 800 Earcut fuzz cases (random star-shaped simple polygons + degenerate/random polygons)
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const n = @as(usize, @intCast(prng.next() % 12)) + 3; // 3..14 vertices
        if (iter % 2 == 0) {
            // Valid star-shaped simple polygon -> must triangulate & conserve area!
            let k: usize = 0;
            while (k < n) : (k += 1) {
                const angle = (2.0 * std.math.pi * (k as f64)) / (n as f64);
                const r = prng.nextRangeF64(5.0, 50.0);
                pts[k] = Point2D{ x: @cos(angle) * r, y: @sin(angle) * r };
            }
            const t_count = triangulate(pts[0..n], tris[0..], scratch[0..]) ?? return 1;
            const ref_a = polygonArea(pts[0..n]);
            const tri_a = triangulationTotalArea(pts[0..n], tris[0..t_count]);
            if (@abs(ref_a - tri_a) > 1.0e-5 * ref_a + 1.0e-6) return 2;
        } else {
            // Arbitrary/degenerate/NaN-injected polygons: 0 panic / 0 infinite loop / 0 OOB
            let k: usize = 0;
            while (k < n) : (k += 1) {
                if ((prng.next() % 32) == 0) {
                    pts[k] = Point2D{ x: std.math.nan(f64), y: 0.0 };
                } else {
                    pts[k] = Point2D{ x: prng.nextRangeF64(-100.0, 100.0), y: prng.nextRangeF64(-100.0, 100.0) };
                }
            }
            * = triangulate(pts[0..n], tris[0..], scratch[0..]);
        }
    }

    // 2. 800 GJK fuzz cases (random convex regular/perturbed polygons: symmetry + 0 panic / 0 hang)
    let pa: [8]Vec2 = [Vec2{ x: 0.0, y: 0.0 }; 8];
    let pb: [8]Vec2 = [Vec2{ x: 0.0, y: 0.0 }; 8];
    iter = 0;
    while (iter < 800) : (iter += 1) {
        const na = @as(usize, @intCast(prng.next() % 5)) + 3;
        const nb = @as(usize, @intCast(prng.next() % 5)) + 3;
        const cax = prng.nextRangeF64(-20.0, 20.0);
        const cay = prng.nextRangeF64(-20.0, 20.0);
        const cbx = prng.nextRangeF64(-20.0, 20.0);
        const cby = prng.nextRangeF64(-20.0, 20.0);
        const ra = prng.nextRangeF64(1.0, 10.0);
        const rb = prng.nextRangeF64(1.0, 10.0);

        let k: usize = 0;
        while (k < na) : (k += 1) {
            const ang = (2.0 * std.math.pi * (k as f64)) / (na as f64);
            pa[k] = Vec2{ x: cax + @cos(ang) * ra, y: cay + @sin(ang) * ra };
        }
        k = 0;
        while (k < nb) : (k += 1) {
            const ang = (2.0 * std.math.pi * (k as f64)) / (nb as f64);
            pb[k] = Vec2{ x: cbx + @cos(ang) * rb, y: cby + @sin(ang) * rb };
        }

        const r_ab = gjkIntersects(pa[0..na], pb[0..nb]);
        const r_ba = gjkIntersects(pb[0..nb], pa[0..na]);
        if (r_ab.colliding != r_ba.colliding) return 3;
    }

    return 0;
}

fn main() {
    test("1. Earcut convex/concave/holed area conservation & NaN/Inf/degenerate/hole guards", () => {
        expect_eq(checkEarcutSuite(), 0 as u32);
    });

    test("2. GJK disjoint/overlap/contained/symmetry & NaN/Inf/degenerate simplex guards", () => {
        expect_eq(checkGjkSuite(), 0 as u32);
    });

    test("3. 4-lane @Vector(4, f64) SIMD AABB & GJK support index vs scalar 0-diff parity", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG fuzzing (800 Earcut + 800 GJK symmetry & stability cases)", () => {
        expect_eq(checkDeterministicFuzz(), 0 as u32);
    });
}
