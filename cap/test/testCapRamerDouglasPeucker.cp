import { test, expect_eq } from "cap:test";
import {
    Pt2,
    Point2D,
    Point3D,
    getSqDist,
    pointToSegmentDistSq,
    getSqSegDist,
    getSqDist3D,
    getSqSegDist3D,
    simplifyRadialDist,
    simplifyPolyline,
    simplifyDouglasPeucker,
    simplify,
    simplifyRadialDist3D,
    simplifyDouglasPeucker3D,
    simplify3D,
} from "../../packages/simplify-js/src/index.cp";

fn approx(a: f64, b: f64): bool {
    let d = a - b;
    if (d < 0.0) { d = -d; }
    return d < 0.000001;
}

fn main() {
    test("1. simplify-js — pointToSegmentDistSq & getSqSegDist perpendicular & endpoints", () => {
        const a = Pt2 { x: 0.0, y: 0.0 };
        const b = Pt2 { x: 10.0, y: 0.0 };
        expect_eq(approx(pointToSegmentDistSq(Pt2 { x: 5.0, y: 3.0 }, a, b), 9.0), true);
        expect_eq(approx(getSqSegDist(Pt2 { x: -4.0, y: 3.0 }, a, b), 25.0), true);
        expect_eq(approx(pointToSegmentDistSq(Pt2 { x: 3.0, y: 4.0 }, a, a), 25.0), true);
        expect_eq(approx(getSqDist(a, Pt2 { x: 3.0, y: 4.0 }), 25.0), true);
    });

    test("2. simplify-js — collinear points collapse to 2 endpoints", () => {
        let pts: [5]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 1.0 },
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 3.0, y: 3.0 },
            Pt2 { x: 4.0, y: 4.0 },
        ];
        let mask: [5]u8 = [0; 5];
        let out: [5]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 5];
        const n = simplifyPolyline(pts[0..5], 0.01, mask[0..5], out[0..5]);
        expect_eq(n, 2 as usize);
        expect_eq(approx(out[0].x, 0.0), true);
        expect_eq(approx(out[1].x, 4.0), true);
    });

    test("3. simplify-js — sharp peak preserved above epsilon", () => {
        let pts: [5]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 0.05 },
            Pt2 { x: 2.0, y: 5.0 },
            Pt2 { x: 3.0, y: 0.05 },
            Pt2 { x: 4.0, y: 0.0 },
        ];
        let mask: [5]u8 = [0; 5];
        let out: [5]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 5];
        const n = simplifyPolyline(pts[0..5], 1.5, mask[0..5], out[0..5]);
        expect_eq(n, 3 as usize);
        expect_eq(approx(out[1].y, 5.0), true);
    });

    test("4. simplify-js — zero epsilon keeps all non-collinear vertices", () => {
        let pts: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 0.2 },
            Pt2 { x: 2.0, y: -0.2 },
            Pt2 { x: 3.0, y: 0.0 },
        ];
        let mask: [4]u8 = [0; 4];
        let out: [4]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 4];
        const n = simplifyPolyline(pts[0..4], 0.0, mask[0..4], out[0..4]);
        expect_eq(n, 4 as usize);
    });

    test("5. simplify-js — huge epsilon reduces to endpoints only", () => {
        let pts: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 10.0 },
            Pt2 { x: 2.0, y: -10.0 },
            Pt2 { x: 3.0, y: 0.0 },
        ];
        let mask: [4]u8 = [0; 4];
        let out: [4]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 4];
        const n = simplifyPolyline(pts[0..4], 100.0, mask[0..4], out[0..4]);
        expect_eq(n, 2 as usize);
    });

    test("6. simplify-js — empty & short polyline boundary safety", () => {
        let pts: [2]Pt2 = [Pt2 { x: 1.0, y: 2.0 }, Pt2 { x: 3.0, y: 4.0 }];
        let mask: [2]u8 = [0; 2];
        let out: [2]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 2];
        expect_eq(simplifyPolyline(pts[0..0], 1.0, mask[0..2], out[0..2]), 0 as usize);
        expect_eq(simplifyPolyline(pts[0..1], 1.0, mask[0..2], out[0..2]), 1 as usize);
        expect_eq(simplifyPolyline(pts[0..2], 1.0, mask[0..2], out[0..2]), 2 as usize);
    });

    test("7. simplify-js — simplifyRadialDist drops dense clusters while keeping endpoints", () => {
        const pts: [6]Point2D = [
            Point2D { x: 0.0, y: 0.0 },
            Point2D { x: 0.1, y: 0.0 },
            Point2D { x: 0.2, y: 0.0 },
            Point2D { x: 5.0, y: 0.0 },
            Point2D { x: 5.1, y: 0.0 },
            Point2D { x: 10.0, y: 0.0 },
        ];
        let out: [6]Point2D = undefined;
        const n = simplifyRadialDist(&pts, 1.0, out[0..6]);
        expect_eq(n, 3 as usize);
        expect_eq(approx(out[0].x, 0.0), true);
        expect_eq(approx(out[1].x, 5.0), true);
        expect_eq(approx(out[2].x, 10.0), true);
    });

    test("8. simplify-js — simplify() fast mode vs highQuality mode & simplifyDouglasPeucker", () => {
        const pts: [5]Point2D = [
            Point2D { x: 0.0, y: 0.0 },
            Point2D { x: 0.5, y: 0.1 },
            Point2D { x: 2.0, y: 4.0 },
            Point2D { x: 3.5, y: 0.1 },
            Point2D { x: 4.0, y: 0.0 },
        ];
        let out_fast: [5]Point2D = undefined;
        let out_hq: [5]Point2D = undefined;
        let out_dp: [5]Point2D = undefined;

        const nf = simplify(&pts, 1.0, false, out_fast[0..5]);
        const nh = simplify(&pts, 1.0, true, out_hq[0..5]);
        const nd = simplifyDouglasPeucker(&pts, 1.0, out_dp[0..5]);
        expect_eq(nf, 3 as usize);
        expect_eq(nh, 3 as usize);
        expect_eq(nd, 3 as usize);
        expect_eq(approx(out_hq[1].y, 4.0), true);
    });

    test("9. simplify-js — 3D Point3D distance, radial pass, DP and simplify3D", () => {
        const a = Point3D { x: 0.0, y: 0.0, z: 0.0 };
        const b = Point3D { x: 0.0, y: 0.0, z: 10.0 };
        expect_eq(approx(getSqDist3D(a, Point3D { x: 1.0, y: 2.0, z: 2.0 }), 9.0), true);
        expect_eq(approx(getSqSegDist3D(Point3D { x: 3.0, y: 4.0, z: 5.0 }, a, b), 25.0), true);

        const pts3: [5]Point3D = [
            Point3D { x: 0.0, y: 0.0, z: 0.0 },
            Point3D { x: 1.0, y: 1.0, z: 0.05 },
            Point3D { x: 2.0, y: 2.0, z: 6.0 },
            Point3D { x: 3.0, y: 3.0, z: 0.05 },
            Point3D { x: 4.0, y: 4.0, z: 0.0 },
        ];
        let out3: [5]Point3D = undefined;
        const n_hq = simplify3D(&pts3, 2.0, true, out3[0..5]);
        expect_eq(n_hq, 3 as usize);
        expect_eq(approx(out3[1].z, 6.0), true);

        const n_fast = simplify3D(&pts3, 2.0, false, out3[0..5]);
        expect_eq(n_fast, 3 as usize);

        const n_rad = simplifyRadialDist3D(&pts3, 0.01, out3[0..5]);
        expect_eq(n_rad, 5 as usize);

        const n_dp = simplifyDouglasPeucker3D(&pts3, 4.0, out3[0..5]);
        expect_eq(n_dp, 3 as usize);
    });
}
