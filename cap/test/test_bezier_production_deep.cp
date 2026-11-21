// Source: cp/cap/test/test_bezier_production_deep.cp (Production deep tests for cap:bezier)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Vec2,
    BBox2,
    QuadPair,
    CubicPair,
    QuadBezier,
    CubicBezier,
    tAtArcLength,
} from "cap:bezier";

fn absF(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    return absF(a - b) <= eps;
}

test("1. Quadratic exact degree elevation to Cubic (< 1.0e-12 error) & tangent/normal orthogonality", () => {
    const q = QuadBezier {
        p0: Vec2 { x: 0.0, y: 0.0 },
        p1: Vec2 { x: 2.5, y: 6.0 },
        p2: Vec2 { x: 8.0, y: 1.0 },
    };
    const c = q.elevateToCubic();

    let i: usize = 0;
    while (i <= 100) {
        const t = (i as f64) / 100.0;
        const pq = q.eval(t);
        const pc = c.eval(t);
        expect_eq(pq.dist(pc) < 1.0e-12, true);

        const tan = q.tangent(t);
        const nrm = q.normal(t);
        expect_eq(approxEq(tan.len(), 1.0, 1.0e-12), true);
        expect_eq(approxEq(nrm.len(), 1.0, 1.0e-12), true);
        expect_eq(approxEq(tan.dot(nrm), 0.0, 1.0e-12), true);
        i += 1;
    }

    // Arc lengths and tight bounding boxes must also match to machine precision
    expect_eq(approxEq(q.arcLength(), c.arcLength(), 1.0e-12), true);
    const bq = q.bbox();
    const bc = c.bbox();
    expect_eq(approxEq(bq.min_x, bc.min_x, 1.0e-12), true);
    expect_eq(approxEq(bq.min_y, bc.min_y, 1.0e-12), true);
    expect_eq(approxEq(bq.max_x, bc.max_x, 1.0e-12), true);
    expect_eq(approxEq(bq.max_y, bc.max_y, 1.0e-12), true);
});

test("2. De Casteljau split(t) exact continuity, reparameterization & arc length additivity", () => {
    const c = CubicBezier {
        p0: Vec2 { x: 0.0, y: 0.0 },
        p1: Vec2 { x: 1.0, y: 4.0 },
        p2: Vec2 { x: 5.0, y: -3.0 },
        p3: Vec2 { x: 7.0, y: 2.0 },
    };

    const ts: f64 = 0.37;
    const pair = c.split(ts);

    // Junction point matches original eval(ts)
    const mid = c.eval(ts);
    expect_eq(pair.left.p3.dist(mid) < 1.0e-12, true);
    expect_eq(pair.right.p0.dist(mid) < 1.0e-12, true);

    // Tangent direction continuous across split
    const t_left = pair.left.tangent(1.0);
    const t_right = pair.right.tangent(0.0);
    const t_orig = c.tangent(ts);
    expect_eq(t_left.dist(t_orig) < 1.0e-12, true);
    expect_eq(t_right.dist(t_orig) < 1.0e-12, true);

    // Sample points along left and right sub-curves match original curve exactly
    let i: usize = 0;
    while (i <= 20) {
        const u = (i as f64) / 20.0;
        expect_eq(pair.left.eval(u).dist(c.eval(u * ts)) < 1.0e-12, true);
        expect_eq(pair.right.eval(u).dist(c.eval(ts + u * (1.0 - ts))) < 1.0e-12, true);
        i += 1;
    }

    // Arc length additivity: len(left) + len(right) == len(original)
    const sum_len = pair.left.arcLength() + pair.right.arcLength();
    expect_eq(approxEq(sum_len, c.arcLength(), 1.0e-9), true);

    // Quadratic split check
    const q = QuadBezier {
        p0: Vec2 { x: 0.0, y: 0.0 },
        p1: Vec2 { x: 3.0, y: 5.0 },
        p2: Vec2 { x: 6.0, y: 0.0 },
    };
    const qpair = q.split(0.5);
    expect_eq(qpair.left.p2.dist(q.eval(0.5)) < 1.0e-12, true);
    expect_eq(approxEq(qpair.left.arcLength() + qpair.right.arcLength(), q.arcLength(), 1.0e-9), true);
});

test("3. Quarter-circle cubic Bezier approximation: arcLength ≈ pi/2 (< 1.0e-4) & curvature ≈ 1.0", () => {
    const k: f64 = 4.0 * (std.math.sqrt(2.0) - 1.0) / 3.0; // ≈ 0.5522847498307936
    const quarter = CubicBezier {
        p0: Vec2 { x: 1.0, y: 0.0 },
        p1: Vec2 { x: 1.0, y: k },
        p2: Vec2 { x: k, y: 1.0 },
        p3: Vec2 { x: 0.0, y: 1.0 },
    };

    const half_pi: f64 = 1.5707963267948966;
    const len = quarter.arcLength();
    expect_eq(absF(len - half_pi) < 1.0e-4, true);

    // Curvature along the classic kappa quarter-circle stays very close to +1.0
    expect_eq(absF(quarter.curvature(0.5) - 1.0) < 0.005, true);
    expect_eq(absF(quarter.curvature(0.25) - 1.0) < 0.005, true);
    expect_eq(absF(quarter.curvature(0.75) - 1.0) < 0.005, true);

    // Midpoint normal points radially inward toward origin (-sqrt(2)/2, -sqrt(2)/2)
    const n_mid = quarter.normal(0.5);
    const inv_sqrt2: f64 = 0.7071067811865475;
    expect_eq(approxEq(n_mid.x, -inv_sqrt2, 1.0e-6), true);
    expect_eq(approxEq(n_mid.y, -inv_sqrt2, 1.0e-6), true);
});

test("4. S-shaped cubic Bezier exact analytical tight bbox vs loose control-point hull", () => {
    // Control points span y in [-10, +10], but the actual curve extrema are much tighter!
    const s_curve = CubicBezier {
        p0: Vec2 { x: 0.0, y: 0.0 },
        p1: Vec2 { x: 2.0, y: 10.0 },
        p2: Vec2 { x: 4.0, y: -10.0 },
        p3: Vec2 { x: 6.0, y: 0.0 },
    };

    const box = s_curve.bbox();
    // Strictly tighter than control-point hull [-10, 10]
    expect_eq(box.max_y < 3.0, true);
    expect_eq(box.min_y > -3.0, true);
    // Exact analytical extremum of y(t) = 30*(1-t)^2*t - 30*(1-t)*t^2 = 30*t*(1-t)*(1-2t):
    // max/min at t = (3 +- sqrt(3)) / 6 => |y_ext| = 5 / sqrt(3) ≈ 2.8867513459
    const exact_y_ext: f64 = 5.0 / std.math.sqrt(3.0);
    expect_eq(approxEq(box.max_y, exact_y_ext, 1.0e-12), true);
    expect_eq(approxEq(box.min_y, -exact_y_ext, 1.0e-12), true);
    expect_eq(approxEq(box.width(), 6.0, 1.0e-12), true);

    // Every sampled point on the curve must lie inside the analytical tight bbox
    let i: usize = 0;
    while (i <= 500) {
        const p = s_curve.eval((i as f64) / 500.0);
        expect_eq(box.contains(p), true);
        i += 1;
    }
});

test("5. tAtArcLength bisection inversion on symmetric & asymmetric non-constant-speed curves", () => {
    // Symmetric quarter circle: half arc length must invert to t == 0.5
    const k: f64 = 0.5522847498307936;
    const quarter = CubicBezier {
        p0: Vec2 { x: 1.0, y: 0.0 },
        p1: Vec2 { x: 1.0, y: k },
        p2: Vec2 { x: k, y: 1.0 },
        p3: Vec2 { x: 0.0, y: 1.0 },
    };
    const q_len = quarter.arcLength();
    expect_eq(approxEq(tAtArcLength(quarter, q_len * 0.5), 0.5, 1.0e-9), true);
    expect_eq(approxEq(tAtArcLength(quarter, 0.0), 0.0, 1.0e-12), true);
    expect_eq(approxEq(tAtArcLength(quarter, -1.0), 0.0, 1.0e-12), true);
    expect_eq(approxEq(tAtArcLength(quarter, q_len * 2.0), 1.0, 1.0e-12), true);

    // Strongly asymmetric speed curve (control points clustered near p0)
    const asym = CubicBezier {
        p0: Vec2 { x: 0.0, y: 0.0 },
        p1: Vec2 { x: 0.1, y: 0.2 },
        p2: Vec2 { x: 0.5, y: 3.0 },
        p3: Vec2 { x: 10.0, y: 0.0 },
    };
    const total = asym.arcLength();
    const target_25 = total * 0.25;
    const target_75 = total * 0.75;
    const t25 = tAtArcLength(asym, target_25);
    const t75 = tAtArcLength(asym, target_75);
    expect_eq(t25 > 0.25, true); // slow start means 25% length occurs after t=0.25
    expect_eq(approxEq(asym.arcLengthTo(t25), target_25, 1.0e-9), true);
    expect_eq(approxEq(asym.arcLengthTo(t75), target_75, 1.0e-9), true);
});

test("6. sampleUniform zero-alloc sampling & Vec2/BBox2 edge cases", () => {
    const c = CubicBezier {
        p0: Vec2 { x: 1.0, y: 2.0 },
        p1: Vec2 { x: 3.0, y: 6.0 },
        p2: Vec2 { x: 5.0, y: -2.0 },
        p3: Vec2 { x: 9.0, y: 4.0 },
    };

    let pts: [5]Vec2 = std.mem.zeroes([5]Vec2);

    // Empty output slice rejected
    expect_eq(c.sampleUniform(pts[0..0]) == null, true);

    // Single sample returns p0
    expect_eq(c.sampleUniform(pts[0..1]) ?? unreachable, 1 as usize);
    expect_eq(pts[0].dist(c.p0) < 1.0e-12, true);

    // 5 uniform samples hit p0, eval(0.25), eval(0.5), eval(0.75), p3
    expect_eq(c.sampleUniform(pts[0..5]) ?? unreachable, 5 as usize);
    expect_eq(pts[0].dist(c.p0) < 1.0e-12, true);
    expect_eq(pts[2].dist(c.eval(0.5)) < 1.0e-12, true);
    expect_eq(pts[4].dist(c.p3) < 1.0e-12, true);

    // Degenerate zero-vector normalize & zero-length curve curvature return 0 safely
    const zero_v = Vec2 { x: 0.0, y: 0.0 };
    const n0 = zero_v.normalize();
    expect_eq(approxEq(n0.x, 0.0, 1.0e-12), true);
    expect_eq(approxEq(n0.y, 0.0, 1.0e-12), true);
});
