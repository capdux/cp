import { test, expect_eq } from "cap:test";
import {
    Pt2,
    QuadBezier,
    CubicBezier,
    CubicSplit,
    splitCubic,
    cubicBezierEasing,
} from "../../packages/@cap/bezier-curve/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) {
        d = 0.0 - d;
    }
    return d <= eps;
}

fn nearPt(a: Pt2, b: Pt2, eps: f64): bool {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps);
}

fn main() {
    test("1. @cap/bezier-curve — QuadBezier endpoints & midpoint eval", () => {
        let eps: f64 = 0.000000001;
        let q: QuadBezier = QuadBezier.new(
            Pt2.new(0.0, 0.0),
            Pt2.new(1.0, 2.0),
            Pt2.new(2.0, 0.0)
        );
        expect_eq(nearPt(q.eval(0.0), Pt2.new(0.0, 0.0), eps), true);
        expect_eq(nearPt(q.eval(1.0), Pt2.new(2.0, 0.0), eps), true);
        expect_eq(nearPt(q.eval(0.5), Pt2.new(1.0, 1.0), eps), true);
    });

    test("2. @cap/bezier-curve — CubicBezier endpoints & midpoint eval", () => {
        let eps: f64 = 0.000000001;
        let c: CubicBezier = CubicBezier.new(
            Pt2.new(0.0, 0.0),
            Pt2.new(0.0, 4.0),
            Pt2.new(4.0, 4.0),
            Pt2.new(4.0, 0.0)
        );
        expect_eq(nearPt(c.eval(0.0), Pt2.new(0.0, 0.0), eps), true);
        expect_eq(nearPt(c.eval(1.0), Pt2.new(4.0, 0.0), eps), true);
        expect_eq(nearPt(c.eval(0.5), Pt2.new(2.0, 3.0), eps), true);
    });

    test("3. @cap/bezier-curve — QuadBezier & CubicBezier derivative tangents", () => {
        let eps: f64 = 0.000000001;
        let q: QuadBezier = QuadBezier.new(
            Pt2.new(0.0, 0.0),
            Pt2.new(1.0, 2.0),
            Pt2.new(2.0, 0.0)
        );
        expect_eq(nearPt(q.deriv(0.0), Pt2.new(2.0, 4.0), eps), true);
        expect_eq(nearPt(q.deriv(0.5), Pt2.new(2.0, 0.0), eps), true);
        expect_eq(nearPt(q.deriv(1.0), Pt2.new(2.0, -4.0), eps), true);

        let c: CubicBezier = CubicBezier.new(
            Pt2.new(0.0, 0.0),
            Pt2.new(0.0, 4.0),
            Pt2.new(4.0, 4.0),
            Pt2.new(4.0, 0.0)
        );
        expect_eq(nearPt(c.deriv(0.0), Pt2.new(0.0, 12.0), eps), true);
        expect_eq(nearPt(c.deriv(0.5), Pt2.new(6.0, 0.0), eps), true);
        expect_eq(nearPt(c.deriv(1.0), Pt2.new(0.0, -12.0), eps), true);
    });

    test("4. @cap/bezier-curve — de Casteljau splitCubic preserves curve geometry", () => {
        let eps: f64 = 0.000000001;
        let c: CubicBezier = CubicBezier.new(
            Pt2.new(0.0, 0.0),
            Pt2.new(1.0, 3.0),
            Pt2.new(3.0, 3.0),
            Pt2.new(5.0, 1.0)
        );
        let t_split: f64 = 0.4;
        let sp: CubicSplit = splitCubic(c, t_split);

        expect_eq(nearPt(sp.left.p3, c.eval(t_split), eps), true);
        expect_eq(nearPt(sp.right.p0, c.eval(t_split), eps), true);
        expect_eq(nearPt(sp.left.eval(0.5), c.eval(0.2), eps), true);
        expect_eq(nearPt(sp.right.eval(0.5), c.eval(0.7), eps), true);
        expect_eq(nearPt(sp.left.eval(0.25), c.eval(0.1), eps), true);
        expect_eq(nearPt(sp.right.eval(0.75), c.eval(0.85), eps), true);
    });

    test("5. @cap/bezier-curve — straight-line arcLength exactness", () => {
        let eps: f64 = 0.000000001;
        let q_line: QuadBezier = QuadBezier.new(
            Pt2.new(0.0, 0.0),
            Pt2.new(1.5, 2.0),
            Pt2.new(3.0, 4.0)
        );
        expect_eq(near(q_line.arcLength(16), 5.0, eps), true);
        expect_eq(near(q_line.arcLength(0), 0.0, eps), true);

        let c_line: CubicBezier = CubicBezier.new(
            Pt2.new(0.0, 0.0),
            Pt2.new(2.0, 0.0),
            Pt2.new(4.0, 0.0),
            Pt2.new(6.0, 0.0)
        );
        expect_eq(near(c_line.arcLength(32), 6.0, eps), true);
    });

    test("6. @cap/bezier-curve — CSS cubicBezierEasing linear & ease-in-out", () => {
        let eps: f64 = 0.000001;
        // Linear: cubic-bezier(0, 0, 1, 1)
        expect_eq(near(cubicBezierEasing(0.0, 0.0, 1.0, 1.0, 0.0), 0.0, eps), true);
        expect_eq(near(cubicBezierEasing(0.0, 0.0, 1.0, 1.0, 0.37), 0.37, eps), true);
        expect_eq(near(cubicBezierEasing(0.0, 0.0, 1.0, 1.0, 1.0), 1.0, eps), true);

        // CSS ease-in-out: cubic-bezier(0.42, 0, 0.58, 1)
        expect_eq(near(cubicBezierEasing(0.42, 0.0, 0.58, 1.0, 0.0), 0.0, eps), true);
        expect_eq(near(cubicBezierEasing(0.42, 0.0, 0.58, 1.0, 0.5), 0.5, eps), true);
        expect_eq(near(cubicBezierEasing(0.42, 0.0, 0.58, 1.0, 1.0), 1.0, eps), true);

        // Symmetry of ease-in-out: f(0.25) + f(0.75) == 1.0, and f(0.25) < 0.25
        let y25: f64 = cubicBezierEasing(0.42, 0.0, 0.58, 1.0, 0.25);
        let y75: f64 = cubicBezierEasing(0.42, 0.0, 0.58, 1.0, 0.75);
        expect_eq(y25 < 0.25, true);
        expect_eq(y75 > 0.75, true);
        expect_eq(near(y25 + y75, 1.0, eps), true);
    });
}
