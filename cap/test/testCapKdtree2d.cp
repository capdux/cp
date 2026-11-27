// cap/test/testCapKdtree2d.cp
// Comprehensive unit tests for @cap/kdtree-2d

import { test, expect_eq } from "cap:test";
import {
    KdPt,
    KdNode,
    NearestResult,
    buildKdTree,
    nearestNeighbor,
    rangeQueryRect,
    radiusQuery
} from "../../packages/kdbush/src/index.cp";

fn absF64(v: f64): f64 {
    if (v < 0.0) {
        return -v;
    }
    return v;
}

fn hasId(ids: []u32, n: usize, target: u32): bool {
    let i: usize = 0;
    while (i < n) {
        if (ids[i] == target) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn main() {
    test("1. @cap/kdtree-2d — 16-point 4x4 grid balanced build & exact nearest neighbor", () => {
        let pts: [16]KdPt = undefined;
        let nodes: [16]KdNode = undefined;
        let idx: usize = 0;
        let r: usize = 0;
        while (r < 4) {
            let c: usize = 0;
            while (c < 4) {
                pts[idx] = KdPt {
                    x: (c * 10) as f64,
                    y: (r * 10) as f64,
                    id: (idx + 1) as u32,
                };
                idx = idx + 1;
                c = c + 1;
            }
            r = r + 1;
        }

        let root = buildKdTree(pts[0..16], nodes[0..16]);
        expect_eq(root >= 0, true);

        // Query near (20, 10) which is r=1, c=2 -> id = 1*4 + 2 + 1 = 7
        let res = nearestNeighbor(nodes[0..16], root, 21.2, 9.4);
        expect_eq(res.found, true);
        expect_eq(res.id, 7 as u32);
        let expected_d2: f64 = 1.2 * 1.2 + 0.6 * 0.6;
        expect_eq(absF64(res.dist_sq - expected_d2) <= 0.000001, true);
    });

    test("2. @cap/kdtree-2d — Exact point coincidence returns dist_sq == 0", () => {
        let pts: [5]KdPt = [
            KdPt { x: 0.0, y: 0.0, id: 10 },
            KdPt { x: 5.5, y: -3.25, id: 20 },
            KdPt { x: 10.0, y: 10.0, id: 30 },
            KdPt { x: -4.0, y: 8.0, id: 40 },
            KdPt { x: 2.0, y: 2.0, id: 50 }
        ];
        let nodes: [5]KdNode = undefined;
        let root = buildKdTree(pts[0..5], nodes[0..5]);
        expect_eq(root >= 0, true);

        let res = nearestNeighbor(nodes[0..5], root, 5.5, -3.25);
        expect_eq(res.found, true);
        expect_eq(res.id, 20 as u32);
        expect_eq(absF64(res.dist_sq) <= 0.000001, true);
    });

    test("3. @cap/kdtree-2d — Cross-hyperplane backtracking finds true nearest neighbor", () => {
        // Root splits on X near x=5. Query at (4.9, 5.0) lies on the left side of x=5,
        // while the true nearest neighbor is at (5.1, 5.0) on the right side (dist = 0.2)!
        let pts: [5]KdPt = [
            KdPt { x: 0.0, y: 0.0, id: 1 },
            KdPt { x: 1.0, y: 10.0, id: 2 },
            KdPt { x: 5.0, y: 0.0, id: 3 },
            KdPt { x: 5.1, y: 5.0, id: 4 },
            KdPt { x: 10.0, y: 10.0, id: 5 }
        ];
        let nodes: [5]KdNode = undefined;
        let root = buildKdTree(pts[0..5], nodes[0..5]);
        expect_eq(root >= 0, true);

        let res = nearestNeighbor(nodes[0..5], root, 4.9, 5.0);
        expect_eq(res.found, true);
        expect_eq(res.id, 4 as u32);
        expect_eq(absF64(res.dist_sq - 0.04) <= 0.000001, true);
    });

    test("4. @cap/kdtree-2d — rangeQueryRect exact bounding box hits", () => {
        let pts: [6]KdPt = [
            KdPt { x: 0.0, y: 0.0, id: 1 },
            KdPt { x: 2.0, y: 3.0, id: 2 },
            KdPt { x: 5.0, y: 5.0, id: 3 },
            KdPt { x: 8.0, y: 2.0, id: 4 },
            KdPt { x: 10.0, y: 10.0, id: 5 },
            KdPt { x: 4.0, y: 1.0, id: 6 }
        ];
        let nodes: [6]KdNode = undefined;
        let root = buildKdTree(pts[0..6], nodes[0..6]);

        let out_ids: [8]u32 = undefined;
        let n = rangeQueryRect(nodes[0..6], root, 1.5, 0.5, 5.5, 5.5, out_ids[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(hasId(out_ids[0..8], n, 2), true);
        expect_eq(hasId(out_ids[0..8], n, 3), true);
        expect_eq(hasId(out_ids[0..8], n, 6), true);
        expect_eq(hasId(out_ids[0..8], n, 1), false);
        expect_eq(hasId(out_ids[0..8], n, 4), false);
    });

    test("5. @cap/kdtree-2d — radiusQuery circle excludes square corners", () => {
        // Center at (0, 0), radius = 1.0:
        // Inside/on circle: (0,0), (1,0), (0,-1), (0.6, 0.6) [dist^2=0.72 <= 1]
        // Inside bounding square [-1,1]^2 but OUTSIDE circle: (0.9, 0.9), (-0.8, 0.8), (1, 1), (-1, -1)
        let pts: [8]KdPt = [
            KdPt { x: 0.0, y: 0.0, id: 1 },
            KdPt { x: 1.0, y: 0.0, id: 2 },
            KdPt { x: 0.0, y: -1.0, id: 3 },
            KdPt { x: 0.6, y: 0.6, id: 4 },
            KdPt { x: 0.9, y: 0.9, id: 5 },
            KdPt { x: -0.8, y: 0.8, id: 6 },
            KdPt { x: 1.0, y: 1.0, id: 7 },
            KdPt { x: -1.0, y: -1.0, id: 8 }
        ];
        let nodes: [8]KdNode = undefined;
        let root = buildKdTree(pts[0..8], nodes[0..8]);

        let out_ids: [8]u32 = undefined;
        let n = radiusQuery(nodes[0..8], root, 0.0, 0.0, 1.0, out_ids[0..8]);
        expect_eq(n, 4 as usize);
        expect_eq(hasId(out_ids[0..8], n, 1), true);
        expect_eq(hasId(out_ids[0..8], n, 2), true);
        expect_eq(hasId(out_ids[0..8], n, 3), true);
        expect_eq(hasId(out_ids[0..8], n, 4), true);
        expect_eq(hasId(out_ids[0..8], n, 5), false);
        expect_eq(hasId(out_ids[0..8], n, 6), false);
        expect_eq(hasId(out_ids[0..8], n, 7), false);
        expect_eq(hasId(out_ids[0..8], n, 8), false);
    });

    test("6. @cap/kdtree-2d — Empty & single-point boundary safety", () => {
        let pts: [1]KdPt = [KdPt { x: 3.0, y: 4.0, id: 99 }];
        let nodes: [1]KdNode = undefined;

        // Empty build returns -1 and queries return empty/not found
        expect_eq(buildKdTree(pts[0..0], nodes[0..1]), -1 as i32);
        let empty_nn = nearestNeighbor(nodes[0..1], -1, 0.0, 0.0);
        expect_eq(empty_nn.found, false);

        let out_ids: [4]u32 = undefined;
        expect_eq(rangeQueryRect(nodes[0..1], -1, 0.0, 0.0, 10.0, 10.0, out_ids[0..4]), 0 as usize);
        expect_eq(radiusQuery(nodes[0..1], -1, 0.0, 0.0, 10.0, out_ids[0..4]), 0 as usize);

        // Single-point tree
        let root = buildKdTree(pts[0..1], nodes[0..1]);
        expect_eq(root, 0 as i32);
        let nn = nearestNeighbor(nodes[0..1], root, 0.0, 0.0);
        expect_eq(nn.found, true);
        expect_eq(nn.id, 99 as u32);
        expect_eq(absF64(nn.dist_sq - 25.0) <= 0.000001, true);
    });
}
