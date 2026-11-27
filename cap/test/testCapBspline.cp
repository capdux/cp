import { test, expect_eq } from "cap:test";
import {
    Pt2,
    hermite1D,
    hermite2D,
    catmullRom1D,
    catmullRom2D,
    uniformCubicBSpline1D,
    uniformCubicBSpline2D,
    evalCatmullRomChain,
} from "../../packages/b-spline/src/index.cp";

const EPS: f64 = 0.000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    const d: f64 = if (a > b) a - b else b - a;
    return d <= eps;
}

fn approxPt2(a: Pt2, b: Pt2, eps: f64): bool {
    return approxF64(a.x, b.x, eps) && approxF64(a.y, b.y, eps);
}

fn main() {
    test("1. @cap/bspline — Cubic Hermite endpoints & finite-difference derivatives", () => {
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

    test("2. @cap/bspline — Catmull-Rom interpolates interior control points p1 & p2", () => {
        expect_eq(approxF64(catmullRom1D(-3.0, 1.5, 4.25, 9.0, 0.0), 1.5, EPS), true);
        expect_eq(approxF64(catmullRom1D(-3.0, 1.5, 4.25, 9.0, 1.0), 4.25, EPS), true);

        // Collinear uniform points should reproduce linear segment exactly
        expect_eq(approxF64(catmullRom1D(0.0, 10.0, 20.0, 30.0, 0.25), 12.5, EPS), true);
        expect_eq(approxF64(catmullRom1D(0.0, 10.0, 20.0, 30.0, 0.5), 15.0, EPS), true);

        const p0 = Pt2 { x: 0.0, y: 0.0 };
        const p1 = Pt2 { x: 1.0, y: 3.0 };
        const p2 = Pt2 { x: 4.0, y: -2.0 };
        const p3 = Pt2 { x: 7.0, y: 5.0 };
        expect_eq(approxPt2(catmullRom2D(p0, p1, p2, p3, 0.0), p1, EPS), true);
        expect_eq(approxPt2(catmullRom2D(p0, p1, p2, p3, 1.0), p2, EPS), true);
    });

    test("3. @cap/bspline — Uniform cubic B-Spline convex hull & collinearity preservation", () => {
        // Partition of unity: constant control points return the exact constant
        expect_eq(approxF64(uniformCubicBSpline1D(5.0, 5.0, 5.0, 5.0, 0.37), 5.0, EPS), true);

        // Collinear uniform points reproduce exact line y = 2x + 1
        const c0 = Pt2 { x: 0.0, y: 1.0 };
        const c1 = Pt2 { x: 1.0, y: 3.0 };
        const c2 = Pt2 { x: 2.0, y: 5.0 };
        const c3 = Pt2 { x: 3.0, y: 7.0 };
        const mid = uniformCubicBSpline2D(c0, c1, c2, c3, 0.5);
        expect_eq(approxPt2(mid, Pt2 { x: 1.5, y: 4.0 }, EPS), true);

        // Convex hull property: curve stays inside bounding box of control points
        const a0 = Pt2 { x: 0.0, y: 0.0 };
        const a1 = Pt2 { x: 2.0, y: 10.0 };
        const a2 = Pt2 { x: 8.0, y: -4.0 };
        const a3 = Pt2 { x: 10.0, y: 6.0 };
        let i: u32 = 0;
        while (i <= 20) {
            const t: f64 = (i as f64) / 20.0;
            const pt = uniformCubicBSpline2D(a0, a1, a2, a3, t);
            expect_eq(pt.x >= 0.0 && pt.x <= 10.0, true);
            expect_eq(pt.y >= -4.0 && pt.y <= 10.0, true);
            i += 1;
        }
    });

    test("4. @cap/bspline — Multi-segment evalCatmullRomChain start, mid & end", () => {
        const pts: [5]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 2.0 },
            Pt2 { x: 3.0, y: 4.0 },
            Pt2 { x: 6.0, y: 1.0 },
            Pt2 { x: 9.0, y: 3.0 },
        ];
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 0.0), pts[0], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 0.25), pts[1], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 0.5), pts[2], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 0.75), pts[3], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 1.0), pts[4], EPS), true);
        // Out-of-range u clamping
        expect_eq(approxPt2(evalCatmullRomChain(&pts, -0.5), pts[0], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&pts, 1.5), pts[4], EPS), true);
    });

    test("5. @cap/bspline — 2D reversal symmetry of Catmull-Rom & B-Spline", () => {
        const p0 = Pt2 { x: -2.0, y: 1.0 };
        const p1 = Pt2 { x: 1.0, y: 5.0 };
        const p2 = Pt2 { x: 4.0, y: -3.0 };
        const p3 = Pt2 { x: 9.0, y: 2.0 };
        const t: f64 = 0.3;

        const cr_fwd = catmullRom2D(p0, p1, p2, p3, t);
        const cr_rev = catmullRom2D(p3, p2, p1, p0, 1.0 - t);
        expect_eq(approxPt2(cr_fwd, cr_rev, EPS), true);

        const bs_fwd = uniformCubicBSpline2D(p0, p1, p2, p3, t);
        const bs_rev = uniformCubicBSpline2D(p3, p2, p1, p0, 1.0 - t);
        expect_eq(approxPt2(bs_fwd, bs_rev, EPS), true);
    });

    test("6. @cap/bspline — Degenerate chains with fewer than 4 points", () => {
        const dummy: [1]Pt2 = [Pt2 { x: 7.0, y: -3.0 }];
        const empty: []const Pt2 = dummy[0..0];
        expect_eq(approxPt2(evalCatmullRomChain(empty, 0.5), Pt2 { x: 0.0, y: 0.0 }, EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&dummy, 0.5), Pt2 { x: 7.0, y: -3.0 }, EPS), true);

        const two: [2]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 10.0, y: 20.0 },
        ];
        expect_eq(approxPt2(evalCatmullRomChain(&two, 0.0), two[0], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&two, 0.5), Pt2 { x: 5.0, y: 10.0 }, EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&two, 1.0), two[1], EPS), true);

        const three: [3]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 2.0, y: 4.0 },
            Pt2 { x: 4.0, y: 0.0 },
        ];
        expect_eq(approxPt2(evalCatmullRomChain(&three, 0.0), three[0], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&three, 0.5), three[1], EPS), true);
        expect_eq(approxPt2(evalCatmullRomChain(&three, 1.0), three[2], EPS), true);
    });
}
