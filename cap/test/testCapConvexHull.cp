import { test, expect_eq } from "cap:test";
import {
    Pt2,
    crossOAB,
    sortPointsXY,
    convexHull,
    hullArea,
    hullPerimeter,
} from "../../packages/@cap/convex-hull/src/index.cp";

const EPS: f64 = 0.000001;

fn approxF64(a: f64, b: f64): bool {
    const d: f64 = if (a > b) a - b else b - a;
    return d <= EPS;
}

fn approxPt2(a: Pt2, b: Pt2): bool {
    return approxF64(a.x, b.x) && approxF64(a.y, b.y);
}

fn main() {
    test("1. @cap/convex-hull — Square with interior & edge points keeps only 4 corners", () => {
        let pts: [9]Pt2 = [
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 4.0, y: 4.0 },
            Pt2 { x: 1.0, y: 3.0 },
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 2.0, y: 0.0 }, // collinear bottom edge midpoint
            Pt2 { x: 4.0, y: 0.0 },
            Pt2 { x: 3.0, y: 1.0 },
            Pt2 { x: 0.0, y: 4.0 },
            Pt2 { x: 4.0, y: 2.0 }, // collinear right edge midpoint
        ];
        let hull: [9]Pt2 = undefined;
        const n = convexHull(&pts, &hull);
        expect_eq(n, 4);
        // CCW starting from bottom-left (0,0)
        expect_eq(approxPt2(hull[0], Pt2 { x: 0.0, y: 0.0 }), true);
        expect_eq(approxPt2(hull[1], Pt2 { x: 4.0, y: 0.0 }), true);
        expect_eq(approxPt2(hull[2], Pt2 { x: 4.0, y: 4.0 }), true);
        expect_eq(approxPt2(hull[3], Pt2 { x: 0.0, y: 4.0 }), true);
    });

    test("2. @cap/convex-hull — Triangle preserves all 3 vertices in CCW order", () => {
        let pts: [3]Pt2 = [
            Pt2 { x: 1.0, y: 3.0 },
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 4.0, y: 0.0 },
        ];
        let hull: [3]Pt2 = undefined;
        const n = convexHull(&pts, &hull);
        expect_eq(n, 3);
        expect_eq(crossOAB(hull[0], hull[1], hull[2]) > 0.0, true);
    });

    test("3. @cap/convex-hull — Collinear points reduce to 2 extreme endpoints", () => {
        let pts: [5]Pt2 = [
            Pt2 { x: 2.0, y: 4.0 },
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 4.0, y: 8.0 },
            Pt2 { x: 1.0, y: 2.0 },
            Pt2 { x: 3.0, y: 6.0 },
        ];
        let hull: [5]Pt2 = undefined;
        const n = convexHull(&pts, &hull);
        expect_eq(n, 2);
        expect_eq(approxPt2(hull[0], Pt2 { x: 0.0, y: 0.0 }), true);
        expect_eq(approxPt2(hull[1], Pt2 { x: 4.0, y: 8.0 }), true);
    });

    test("4. @cap/convex-hull — Duplicate points are deduplicated cleanly", () => {
        let pts: [7]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 3.0, y: 0.0 },
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 0.0, y: 4.0 },
            Pt2 { x: 3.0, y: 0.0 },
            Pt2 { x: 0.0, y: 4.0 },
            Pt2 { x: 1.0, y: 1.0 },
        ];
        let hull: [7]Pt2 = undefined;
        const n = convexHull(&pts, &hull);
        expect_eq(n, 3);

        let all_same: [4]Pt2 = [
            Pt2 { x: 5.0, y: 5.0 },
            Pt2 { x: 5.0, y: 5.0 },
            Pt2 { x: 5.0, y: 5.0 },
            Pt2 { x: 5.0, y: 5.0 },
        ];
        let hull_same: [4]Pt2 = undefined;
        expect_eq(convexHull(&all_same, &hull_same), 1);
        expect_eq(approxPt2(hull_same[0], Pt2 { x: 5.0, y: 5.0 }), true);
    });

    test("5. @cap/convex-hull — hullArea, hullPerimeter & sortPointsXY accuracy", () => {
        let pts: [4]Pt2 = [
            Pt2 { x: 3.0, y: 0.0 },
            Pt2 { x: 0.0, y: 4.0 },
            Pt2 { x: 1.0, y: 1.0 },
            Pt2 { x: 0.0, y: 0.0 },
        ];
        sortPointsXY(&pts);
        expect_eq(approxPt2(pts[0], Pt2 { x: 0.0, y: 0.0 }), true);
        expect_eq(approxPt2(pts[1], Pt2 { x: 0.0, y: 4.0 }), true);

        let hull: [4]Pt2 = undefined;
        const n = convexHull(&pts, &hull);
        expect_eq(n, 3);
        expect_eq(approxF64(hullArea(hull[0..n]), 6.0), true);
        expect_eq(approxF64(hullPerimeter(hull[0..n]), 12.0), true);
    });

    test("6. @cap/convex-hull — Degenerate 0, 1 & 2 point boundaries", () => {
        let one: [1]Pt2 = [Pt2 { x: 2.0, y: 3.0 }];
        let hull: [2]Pt2 = undefined;
        const empty_in: []Pt2 = one[0..0];
        expect_eq(convexHull(empty_in, &hull), 0);
        expect_eq(hullArea(hull[0..0]), 0.0);
        expect_eq(hullPerimeter(hull[0..0]), 0.0);

        expect_eq(convexHull(&one, &hull), 1);
        expect_eq(approxPt2(hull[0], Pt2 { x: 2.0, y: 3.0 }), true);
        expect_eq(hullArea(hull[0..1]), 0.0);
        expect_eq(hullPerimeter(hull[0..1]), 0.0);

        let two: [2]Pt2 = [
            Pt2 { x: 3.0, y: 4.0 },
            Pt2 { x: 0.0, y: 0.0 },
        ];
        expect_eq(convexHull(&two, &hull), 2);
        expect_eq(approxPt2(hull[0], Pt2 { x: 0.0, y: 0.0 }), true);
        expect_eq(approxPt2(hull[1], Pt2 { x: 3.0, y: 4.0 }), true);
        expect_eq(hullArea(hull[0..2]), 0.0);
        expect_eq(approxF64(hullPerimeter(hull[0..2]), 5.0), true);
    });
}
