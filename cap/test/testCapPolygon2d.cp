import { test, expect_eq } from "cap:test";
import {
    Pt2,
    absF64,
    signedArea,
    area,
    isCounterClockwise,
    perimeter,
    centroid,
    containsPoint,
    isConvex,
} from "../../packages/@cap/polygon2d/src/index.cp";

const EPS: f64 = 0.000001;

fn approxF64(a: f64, b: f64): bool {
    return absF64(a - b) <= EPS;
}

fn approxPt2(a: Pt2, b: Pt2): bool {
    return approxF64(a.x, b.x) && approxF64(a.y, b.y);
}

fn main() {
    test("1. @cap/polygon2d — Square & right triangle area, perimeter & centroid", () => {
        const sq: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 4.0, y: 0.0 },
            Pt2 { x: 4.0, y: 4.0 },
            Pt2 { x: 0.0, y: 4.0 },
        ];
        expect_eq(approxF64(area(&sq), 16.0), true);
        expect_eq(approxF64(perimeter(&sq), 16.0), true);
        expect_eq(approxPt2(centroid(&sq), Pt2 { x: 2.0, y: 2.0 }), true);

        const tri: [3]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 3.0, y: 0.0 },
            Pt2 { x: 0.0, y: 4.0 },
        ];
        expect_eq(approxF64(area(&tri), 6.0), true);
        expect_eq(approxF64(perimeter(&tri), 12.0), true);
        expect_eq(approxPt2(centroid(&tri), Pt2 { x: 1.0, y: 4.0 / 3.0 }), true);
    });

    test("2. @cap/polygon2d — CCW vs CW winding direction", () => {
        const ccw: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 2.0, y: 0.0 },
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 0.0, y: 2.0 },
        ];
        const cw: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 0.0, y: 2.0 },
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 2.0, y: 0.0 },
        ];
        expect_eq(isCounterClockwise(&ccw), true);
        expect_eq(approxF64(signedArea(&ccw), 4.0), true);
        expect_eq(isCounterClockwise(&cw), false);
        expect_eq(approxF64(signedArea(&cw), -4.0), true);
        expect_eq(approxF64(area(&cw), 4.0), true);
    });

    test("3. @cap/polygon2d — Convex polygon interior, boundary & exterior points", () => {
        const sq: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 10.0, y: 0.0 },
            Pt2 { x: 10.0, y: 10.0 },
            Pt2 { x: 0.0, y: 10.0 },
        ];
        expect_eq(containsPoint(&sq, Pt2 { x: 5.0, y: 5.0 }), true);
        expect_eq(containsPoint(&sq, Pt2 { x: 0.0, y: 5.0 }), true);
        expect_eq(containsPoint(&sq, Pt2 { x: 10.0, y: 10.0 }), true);
        expect_eq(containsPoint(&sq, Pt2 { x: -1.0, y: 5.0 }), false);
        expect_eq(containsPoint(&sq, Pt2 { x: 11.0, y: 5.0 }), false);
        expect_eq(containsPoint(&sq, Pt2 { x: 5.0, y: 12.0 }), false);
    });

    test("4. @cap/polygon2d — L-shaped concave polygon inside, outside & notch", () => {
        const lshape: [6]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 4.0, y: 0.0 },
            Pt2 { x: 4.0, y: 2.0 },
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 2.0, y: 4.0 },
            Pt2 { x: 0.0, y: 4.0 },
        ];
        // Area = 4*4 - 2*2 = 12
        expect_eq(approxF64(area(&lshape), 12.0), true);
        // Inside lower-right arm & upper-left arm
        expect_eq(containsPoint(&lshape, Pt2 { x: 3.0, y: 1.0 }), true);
        expect_eq(containsPoint(&lshape, Pt2 { x: 1.0, y: 3.0 }), true);
        expect_eq(containsPoint(&lshape, Pt2 { x: 1.0, y: 1.0 }), true);
        // Inside the concave notch (must be OUTSIDE polygon)
        expect_eq(containsPoint(&lshape, Pt2 { x: 3.0, y: 3.0 }), false);
        // Far outside
        expect_eq(containsPoint(&lshape, Pt2 { x: -2.0, y: 2.0 }), false);
    });

    test("5. @cap/polygon2d — isConvex distinguishes convex vs concave polygons", () => {
        const pentagon: [5]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 4.0, y: 0.0 },
            Pt2 { x: 5.0, y: 3.0 },
            Pt2 { x: 2.0, y: 5.0 },
            Pt2 { x: -1.0, y: 3.0 },
        ];
        const lshape: [6]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 4.0, y: 0.0 },
            Pt2 { x: 4.0, y: 2.0 },
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 2.0, y: 4.0 },
            Pt2 { x: 0.0, y: 4.0 },
        ];
        const collinear: [3]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 4.0, y: 4.0 },
        ];
        expect_eq(isConvex(&pentagon), true);
        expect_eq(isConvex(&lshape), false);
        expect_eq(isConvex(&collinear), false);
    });

    test("6. @cap/polygon2d — Degenerate polygons with fewer than 3 vertices", () => {
        const one: [1]Pt2 = [Pt2 { x: 3.0, y: 7.0 }];
        const empty: []const Pt2 = one[0..0];
        const two: [2]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 3.0, y: 4.0 },
        ];

        expect_eq(signedArea(empty), 0.0);
        expect_eq(area(empty), 0.0);
        expect_eq(perimeter(empty), 0.0);
        expect_eq(approxPt2(centroid(empty), Pt2 { x: 0.0, y: 0.0 }), true);
        expect_eq(containsPoint(empty, Pt2 { x: 0.0, y: 0.0 }), false);
        expect_eq(isConvex(empty), false);

        expect_eq(approxPt2(centroid(&one), Pt2 { x: 3.0, y: 7.0 }), true);
        expect_eq(perimeter(&one), 0.0);
        expect_eq(isConvex(&one), false);

        expect_eq(area(&two), 0.0);
        expect_eq(approxF64(perimeter(&two), 5.0), true);
        expect_eq(approxPt2(centroid(&two), Pt2 { x: 1.5, y: 2.0 }), true);
        expect_eq(containsPoint(&two, Pt2 { x: 1.5, y: 2.0 }), false);
        expect_eq(isConvex(&two), false);
    });
}
