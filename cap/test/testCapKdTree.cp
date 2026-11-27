import { test, expect_eq } from "cap:test";
import { MAX_POINTS, KdPoint2D, KdTree2D } from "../../packages/kiddo/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/kd-tree — Empty tree & single-point queries", () => {
        expect_eq(MAX_POINTS, 256 as usize);
        let dummy: [1]KdPoint2D = [KdPoint2D { x: 3.0, y: 4.0, id: 77 }];
        const empty_tree = KdTree2D.build(dummy[0..0]);
        expect_eq(empty_tree.isEmpty(), true);
        expect_eq(empty_tree.len(), 0 as usize);
        const r0 = empty_tree.nearest(0.0, 0.0);
        expect_eq(r0.found, false);

        let out: [4]u32 = undefined;
        expect_eq(empty_tree.withinRadius(0.0, 0.0, 10.0, out[0..4]), 0 as usize);

        const single = KdTree2D.build(dummy[0..1]);
        expect_eq(single.isEmpty(), false);
        expect_eq(single.len(), 1 as usize);
        const r1 = single.nearest(0.0, 0.0);
        expect_eq(r1.found, true);
        expect_eq(r1.id, 77 as u32);
        expect_eq(approxEq(r1.dist_sq, 25.0, 0.0001), true);
    });

    test("2. @cap/kd-tree — Regular 4x4 grid exact nearest hits", () => {
        let pts: [16]KdPoint2D = undefined;
        let idx: usize = 0;
        let gx: usize = 0;
        while (gx < 4) {
            let gy: usize = 0;
            while (gy < 4) {
                pts[idx] = KdPoint2D {
                    x: (gx as f64) * 10.0,
                    y: (gy as f64) * 10.0,
                    id: (gx * 10 + gy) as u32,
                };
                idx += 1;
                gy += 1;
            }
            gx += 1;
        }

        const tree = KdTree2D.build(pts[0..16]);
        expect_eq(tree.len(), 16 as usize);

        const hit1 = tree.nearest(21.0, 29.0); // closest to (20, 30) -> id 23
        expect_eq(hit1.found, true);
        expect_eq(hit1.id, 23 as u32);
        expect_eq(approxEq(hit1.dist_sq, 2.0, 0.0001), true);

        const hit2 = tree.nearest(0.2, 0.3); // closest to (0, 0) -> id 0
        expect_eq(hit2.id, 0 as u32);
    });

    test("3. @cap/kd-tree — 100 pseudo-random points cross-checked vs brute-force over 30 queries", () => {
        let pts: [100]KdPoint2D = undefined;
        let state: u32 = 0xCAFEBABE;
        let i: usize = 0;
        while (i < 100) {
            state = state *% 1664525 +% 1013904223;
            const rx = ((state >> 16) as f64) / 65.536 - 500.0;
            state = state *% 1664525 +% 1013904223;
            const ry = ((state >> 16) as f64) / 65.536 - 500.0;
            pts[i] = KdPoint2D { x: rx, y: ry, id: i as u32 };
            i += 1;
        }

        const tree = KdTree2D.build(pts[0..100]);

        let q: usize = 0;
        while (q < 30) {
            state = state *% 1664525 +% 1013904223;
            const qx = ((state >> 16) as f64) / 65.536 - 500.0;
            state = state *% 1664525 +% 1013904223;
            const qy = ((state >> 16) as f64) / 65.536 - 500.0;

            // Brute-force reference minimum dist_sq
            let best_d2: f64 = 1000000000000.0;
            let j: usize = 0;
            while (j < 100) {
                const dx = qx - pts[j].x;
                const dy = qy - pts[j].y;
                const d2 = dx * dx + dy * dy;
                if (d2 < best_d2) {
                    best_d2 = d2;
                }
                j += 1;
            }

            const res = tree.nearest(qx, qy);
            expect_eq(res.found, true);
            expect_eq(approxEq(res.dist_sq, best_d2, 0.000001), true);
            q += 1;
        }
    });

    test("4. @cap/kd-tree — withinRadius exact boundary inclusion & exclusion", () => {
        const pts: [5]KdPoint2D = [
            KdPoint2D { x: 0.0, y: 0.0, id: 1 },
            KdPoint2D { x: 3.0, y: 4.0, id: 2 },   // dist = 5.0 (exact boundary)
            KdPoint2D { x: -5.0, y: 0.0, id: 3 },  // dist = 5.0 (exact boundary)
            KdPoint2D { x: 3.0, y: 4.01, id: 4 },  // dist > 5.0 (outside)
            KdPoint2D { x: 10.0, y: 10.0, id: 5 }, // outside
        ];
        const tree = KdTree2D.build(pts[0..5]);

        let out: [5]u32 = undefined;
        const count = tree.withinRadius(0.0, 0.0, 5.0, out[0..5]);
        expect_eq(count, 3 as usize);

        // Truncated output slice still returns full count
        let small_out: [2]u32 = undefined;
        expect_eq(tree.withinRadius(0.0, 0.0, 5.0, small_out[0..2]), 3 as usize);
        // Negative radius returns 0
        expect_eq(tree.withinRadius(0.0, 0.0, -1.0, out[0..5]), 0 as usize);
    });

    test("5. @cap/kd-tree — Duplicate coordinates handling", () => {
        const pts: [4]KdPoint2D = [
            KdPoint2D { x: 5.0, y: 5.0, id: 10 },
            KdPoint2D { x: 5.0, y: 5.0, id: 20 },
            KdPoint2D { x: 5.0, y: 5.0, id: 30 },
            KdPoint2D { x: 50.0, y: 50.0, id: 40 },
        ];
        const tree = KdTree2D.build(pts[0..4]);
        const res = tree.nearest(5.0, 5.0);
        expect_eq(res.found, true);
        expect_eq(approxEq(res.dist_sq, 0.0, 0.0001), true);

        let out: [4]u32 = undefined;
        const cnt = tree.withinRadius(5.0, 5.0, 0.0, out[0..4]);
        expect_eq(cnt, 3 as usize);
    });

    test("6. @cap/kd-tree — Negative & wide-span coordinate pruning correctness", () => {
        const pts: [6]KdPoint2D = [
            KdPoint2D { x: -100000.0, y: -100000.0, id: 1 },
            KdPoint2D { x: -100000.0, y: 100000.0, id: 2 },
            KdPoint2D { x: 100000.0, y: -100000.0, id: 3 },
            KdPoint2D { x: 100000.0, y: 100000.0, id: 4 },
            KdPoint2D { x: -0.5, y: 0.5, id: 5 },
            KdPoint2D { x: 0.25, y: -0.25, id: 6 },
        ];
        const tree = KdTree2D.build(pts[0..6]);

        const r1 = tree.nearest(-99999.0, 99998.0);
        expect_eq(r1.id, 2 as u32);
        expect_eq(approxEq(r1.dist_sq, 5.0, 0.0001), true);

        const r2 = tree.nearest(0.0, 0.0);
        expect_eq(r2.id, 6 as u32);
        expect_eq(approxEq(r2.dist_sq, 0.125, 0.0001), true);
    });
}
