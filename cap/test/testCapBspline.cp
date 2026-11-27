import { test, expect_eq } from "cap:test";
import {
    Pt2,
    Pt3,
    hermite1D,
    hermite2D,
    catmullRom1D,
    catmullRom2D,
    uniformCubicBSpline1D,
    uniformCubicBSpline2D,
    evalCatmullRomChain,
    generateClampedKnots,
    generateUniformKnots,
    bspline1D,
    bspline2D,
    bspline3D,
    bspline1DUniform,
    bspline2DUniform,
    bspline3DUniform,
    sampleBSpline2D,
} from "../../packages/b-spline/src/index.cp";

const EPS: f64 = 0.000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    const d: f64 = if (a > b) a - b else b - a;
    return d <= eps;
}

fn approxPt2(a: Pt2, b: Pt2, eps: f64): bool {
    return approxF64(a.x, b.x, eps) && approxF64(a.y, b.y, eps);
}

fn approxPt3(a: Pt3, b: Pt3, eps: f64): bool {
    return approxF64(a.x, b.x, eps) && approxF64(a.y, b.y, eps) && approxF64(a.z, b.z, eps);
}

fn main() {
    test("1. b-spline — Cubic Hermite endpoints & finite-difference derivatives", () => {
        const p0: f64 = 2.0;
        const m0: f64 = 3.5;
        const p1: f64 = -1.0;
        const m1: f64 = -2.0;

        expect_eq(approxF64(hermite1D(p0, m0, p1, m1, 0.0), p0, EPS), true);
        expect_eq(approxF64(hermite1D(p0, m0, p1, m1, 1.0), p1, EPS), true);

        const h: f64 = 0.000001;
        const d0: f64 = (hermite1D(p0, m0, p1, m1, h) - hermite1D(p0, m0, p1, m1, 0.0)) / h;
        const d1: f64 = (hermite1D(p0, m0, p1, m1, 1.0) - hermite1D(p0, m0, p1, m1, 1.0 - h)) / h;
        expect_eq(approxF64(d0, m0, 0.0001), true);
        expect_eq(approxF64(d1, m1, 0.0001), true);

        const q0 = Pt2 { x: 1.0, y: 2.0 };
        const t0 = Pt2 { x: 0.5, y: -1.0 };
        const q1 = Pt2 { x: 4.0, y: 5.0 };
        const t1 = Pt2 { x: 1.0, y: 0.5 };
        expect_eq(approxPt2(hermite2D(q0, t0, q1, t1, 0.0), q0, EPS), true);
        expect_eq(approxPt2(hermite2D(q0, t0, q1, t1, 1.0), q1, EPS), true);
    });

    test("2. b-spline — Catmull-Rom interpolates interior control points p1 & p2", () => {
        expect_eq(approxF64(catmullRom1D(-3.0, 1.5, 4.25, 9.0, 0.0), 1.5, EPS), true);
        expect_eq(approxF64(catmullRom1D(-3.0, 1.5, 4.25, 9.0, 1.0), 4.25, EPS), true);
        expect_eq(approxF64(catmullRom1D(0.0, 10.0, 20.0, 30.0, 0.25), 12.5, EPS), true);
        expect_eq(approxF64(catmullRom1D(0.0, 10.0, 20.0, 30.0, 0.5), 15.0, EPS), true);

        const p0 = Pt2 { x: 0.0, y: 0.0 };
        const p1 = Pt2 { x: 1.0, y: 3.0 };
        const p2 = Pt2 { x: 4.0, y: -2.0 };
        const p3 = Pt2 { x: 7.0, y: 5.0 };
        expect_eq(approxPt2(catmullRom2D(p0, p1, p2, p3, 0.0), p1, EPS), true);
        expect_eq(approxPt2(catmullRom2D(p0, p1, p2, p3, 1.0), p2, EPS), true);
    });

    test("3. b-spline — Uniform cubic B-Spline convex hull & collinearity preservation", () => {
        expect_eq(approxF64(uniformCubicBSpline1D(5.0, 5.0, 5.0, 5.0, 0.37), 5.0, EPS), true);

        const c0 = Pt2 { x: 0.0, y: 1.0 };
        const c1 = Pt2 { x: 1.0, y: 3.0 };
        const c2 = Pt2 { x: 2.0, y: 5.0 };
        const c3 = Pt2 { x: 3.0, y: 7.0 };
        const mid = uniformCubicBSpline2D(c0, c1, c2, c3, 0.5);
        expect_eq(approxPt2(mid, Pt2 { x: 1.5, y: 4.0 }, EPS), true);
    });

    test("4. b-spline — Multi-segment evalCatmullRomChain start, mid & end", () => {
        const pts: [5]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 2.0 },
            Pt2 { x: 3.0, y: 4.0 },
            Pt2 { x: 6.0, y: 1.0 },
            Pt2 { x: 9.0, y: 3.0 },
        ];
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 0.0), pts[0], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 0.5), pts[2], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 1.0), pts[4], EPS), true);
    });

    test("5. b-spline — de Boor 1D clamped & exact endpoint convergence", () => {
        const pts: [4]f64 = [0.0, 10.0, 20.0, 30.0];
        expect_eq(approxF64(bspline1DUniform(0.0, 3, &pts) ?? -1.0, 0.0, EPS), true);
        expect_eq(approxF64(bspline1DUniform(0.5, 3, &pts) ?? -1.0, 15.0, EPS), true);
        expect_eq(approxF64(bspline1DUniform(1.0, 3, &pts) ?? -1.0, 30.0, EPS), true);

        // Degree 1 (linear) interpolation
        const lin: [3]f64 = [0.0, 10.0, 20.0];
        expect_eq(approxF64(bspline1DUniform(0.25, 1, &lin) ?? -1.0, 5.0, EPS), true);
        expect_eq(approxF64(bspline1DUniform(0.75, 1, &lin) ?? -1.0, 15.0, EPS), true);
    });

    test("6. b-spline — de Boor 2D clamped Bezier equivalence & sampling", () => {
        // Degree 2 clamped B-Spline with 3 control points is an exact quadratic Bezier
        const pts: [3]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 2.0 },
            Pt2 { x: 2.0, y: 0.0 },
        ];
        expect_eq(approxPt2(bspline2DUniform(0.0, 2, &pts).?, pts[0], EPS), true);
        expect_eq(approxPt2(bspline2DUniform(0.5, 2, &pts).?, Pt2 { x: 1.0, y: 1.0 }, EPS), true);
        expect_eq(approxPt2(bspline2DUniform(1.0, 2, &pts).?, pts[2], EPS), true);

        const empty: [1]f64 = [0.0];
        let sampled: [5]Pt2 = undefined;
        const cnt = sampleBSpline2D(2, &pts, empty[0..0], empty[0..0], sampled[0..5]);
        expect_eq(cnt, 5 as usize);
        expect_eq(approxPt2(sampled[0], pts[0], EPS), true);
        expect_eq(approxPt2(sampled[2], Pt2 { x: 1.0, y: 1.0 }, EPS), true);
        expect_eq(approxPt2(sampled[4], pts[2], EPS), true);
    });

    test("7. b-spline — de Boor 3D curve evaluation", () => {
        const pts: [4]Pt3 = [
            Pt3 { x: 0.0, y: 0.0, z: 0.0 },
            Pt3 { x: 1.0, y: 2.0, z: 3.0 },
            Pt3 { x: 2.0, y: 4.0, z: 6.0 },
            Pt3 { x: 3.0, y: 6.0, z: 9.0 },
        ];
        expect_eq(approxPt3(bspline3DUniform(0.0, 3, &pts).?, pts[0], EPS), true);
        expect_eq(approxPt3(bspline3DUniform(0.5, 3, &pts).?, Pt3 { x: 1.5, y: 3.0, z: 4.5 }, EPS), true);
        expect_eq(approxPt3(bspline3DUniform(1.0, 3, &pts).?, pts[3], EPS), true);
    });

    test("8. b-spline — Rational NURBS weights represent exact circular arc", () => {
        // Quarter unit circle in 2D: P0=(1,0), P1=(1,1), P2=(0,1) with weights [1, sqrt(2)/2, 1]
        const pts: [3]Pt2 = [
            Pt2 { x: 1.0, y: 0.0 },
            Pt2 { x: 1.0, y: 1.0 },
            Pt2 { x: 0.0, y: 1.0 },
        ];
        const w_mid: f64 = 0.7071067811865475; // sqrt(2)/2
        const weights: [3]f64 = [1.0, w_mid, 1.0];
        const empty: [1]f64 = [0.0];

        const mid = bspline2D(0.5, 2, &pts, empty[0..0], &weights).?;
        const r2 = mid.x * mid.x + mid.y * mid.y;
        expect_eq(approxF64(r2, 1.0, 0.000001), true);
        expect_eq(approxF64(mid.x, w_mid, 0.000001), true);
        expect_eq(approxF64(mid.y, w_mid, 0.000001), true);
    });

    test("9. b-spline — Custom knots, knot generators & invalid argument safety", () => {
        let clamped: [8]f64 = undefined;
        expect_eq(generateClampedKnots(4, 3, clamped[0..8]), 8 as usize);
        expect_eq(approxF64(clamped[0], 0.0, EPS), true);
        expect_eq(approxF64(clamped[3], 0.0, EPS), true);
        expect_eq(approxF64(clamped[4], 1.0, EPS), true);
        expect_eq(approxF64(clamped[7], 1.0, EPS), true);

        let unif: [8]f64 = undefined;
        expect_eq(generateUniformKnots(4, 3, unif[0..8]), 8 as usize);
        expect_eq(approxF64(unif[7], 7.0, EPS), true);

        // Custom knots matching uniformCubicBSpline1D
        const pts: [4]f64 = [0.0, 2.0, 4.0, 6.0];
        const empty: [1]f64 = [0.0];
        const v_deboor = bspline1D(0.5, 3, &pts, unif[0..8], empty[0..0]).?;
        const v_unif = uniformCubicBSpline1D(0.0, 2.0, 4.0, 6.0, 0.5);
        expect_eq(approxF64(v_deboor, v_unif, EPS), true);

        // Invalid degree / mismatched weights return null safely
        expect_eq(bspline1D(0.5, 0, &pts, empty[0..0], empty[0..0]) == null, true);
        expect_eq(bspline1D(0.5, 4, &pts, empty[0..0], empty[0..0]) == null, true);
        const bad_w: [2]f64 = [1.0, 1.0];
        expect_eq(bspline1D(0.5, 2, &pts, empty[0..0], &bad_w) == null, true);
    });
}
