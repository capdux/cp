import { test, expect_eq } from "cap:test";
import { Pt2, pointToSegmentDistSq, simplifyPolyline } from "../../packages/@cap/ramer-douglas-peucker/src/index.cp";

fn approx(a: f64, b: f64): bool {
    let d = a - b;
    if (d < 0.0) { d = -d; }
    return d < 0.000001;
}

fn main() {
    test("pointToSegmentDistSq perpendicular & endpoints", () => {
        let a = Pt2 { x: 0.0, y: 0.0 };
        let b = Pt2 { x: 10.0, y: 0.0 };
        expect_eq(approx(pointToSegmentDistSq(Pt2 { x: 5.0, y: 3.0 }, a, b), 9.0), true);
        expect_eq(approx(pointToSegmentDistSq(Pt2 { x: -4.0, y: 3.0 }, a, b), 25.0), true);
        expect_eq(approx(pointToSegmentDistSq(Pt2 { x: 3.0, y: 4.0 }, a, a), 25.0), true);
    });

    test("collinear points collapse to 2 endpoints", () => {
        let pts: [5]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 1.0 },
            Pt2 { x: 2.0, y: 2.0 },
            Pt2 { x: 3.0, y: 3.0 },
            Pt2 { x: 4.0, y: 4.0 },
        ];
        let mask: [5]u8 = [0; 5];
        let out: [5]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 5];
        let n = simplifyPolyline(pts[0..5], 0.01, mask[0..5], out[0..5]);
        expect_eq(n, 2 as usize);
        expect_eq(approx(out[0].x, 0.0), true);
        expect_eq(approx(out[1].x, 4.0), true);
    });

    test("sharp peak preserved above epsilon", () => {
        let pts: [5]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 0.05 },
            Pt2 { x: 2.0, y: 5.0 },
            Pt2 { x: 3.0, y: 0.05 },
            Pt2 { x: 4.0, y: 0.0 },
        ];
        let mask: [5]u8 = [0; 5];
        let out: [5]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 5];
        let n = simplifyPolyline(pts[0..5], 1.5, mask[0..5], out[0..5]);
        expect_eq(n, 3 as usize);
        expect_eq(approx(out[1].y, 5.0), true);
    });

    test("zero epsilon keeps all non-collinear vertices", () => {
        let pts: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 0.2 },
            Pt2 { x: 2.0, y: -0.2 },
            Pt2 { x: 3.0, y: 0.0 },
        ];
        let mask: [4]u8 = [0; 4];
        let out: [4]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 4];
        let n = simplifyPolyline(pts[0..4], 0.0, mask[0..4], out[0..4]);
        expect_eq(n, 4 as usize);
    });

    test("huge epsilon reduces to endpoints only", () => {
        let pts: [4]Pt2 = [
            Pt2 { x: 0.0, y: 0.0 },
            Pt2 { x: 1.0, y: 10.0 },
            Pt2 { x: 2.0, y: -10.0 },
            Pt2 { x: 3.0, y: 0.0 },
        ];
        let mask: [4]u8 = [0; 4];
        let out: [4]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 4];
        let n = simplifyPolyline(pts[0..4], 100.0, mask[0..4], out[0..4]);
        expect_eq(n, 2 as usize);
    });

    test("empty & short polyline boundary safety", () => {
        let pts: [2]Pt2 = [Pt2 { x: 1.0, y: 2.0 }, Pt2 { x: 3.0, y: 4.0 }];
        let mask: [2]u8 = [0; 2];
        let out: [2]Pt2 = [Pt2 { x: 0.0, y: 0.0 }; 2];
        expect_eq(simplifyPolyline(pts[0..0], 1.0, mask[0..2], out[0..2]), 0 as usize);
        expect_eq(simplifyPolyline(pts[0..1], 1.0, mask[0..2], out[0..2]), 1 as usize);
        expect_eq(simplifyPolyline(pts[0..2], 1.0, mask[0..2], out[0..2]), 2 as usize);
    });
}
