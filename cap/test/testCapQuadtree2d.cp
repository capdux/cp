// cap/test/testCapQuadtree2d.cp
// Comprehensive unit tests for @cap/quadtree-2d

import { test, expect_eq } from "cap:test";
import {
    Rect2,
    QuadItem,
    QuadNode,
    QuadTree,
} from "../../packages/quadtree_rs/src/index.cp";

fn containsId(ids: []const u32, target: u32): bool {
    let i: usize = 0;
    while (i < ids.len) {
        if (ids[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/quadtree-2d — Insert points & full-bounds range query", () => {
        let nodes: [32]QuadNode = undefined;
        let items: [32]QuadItem = undefined;
        let qt = QuadTree.empty();
        const world = Rect2.new(0.0 as f64, 0.0 as f64, 100.0 as f64, 100.0 as f64);
        expect_eq(qt.init(nodes[0..32], world, 4 as u32, 6 as u32), true);

        expect_eq(qt.insert(nodes[0..32], items[0..32], 10.0 as f64, 10.0 as f64, 101 as u32), true);
        expect_eq(qt.insert(nodes[0..32], items[0..32], 80.0 as f64, 20.0 as f64, 102 as u32), true);
        expect_eq(qt.insert(nodes[0..32], items[0..32], 50.0 as f64, 90.0 as f64, 103 as u32), true);

        let out: [8]u32 = undefined;
        const n = qt.queryRect(nodes[0..32], items[0..32], world, out[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(containsId(out[0..n], 101 as u32), true);
        expect_eq(containsId(out[0..n], 102 as u32), true);
        expect_eq(containsId(out[0..n], 103 as u32), true);
    });

    test("2. @cap/quadtree-2d — Automatic 4-quadrant subdivision & sub-rect query", () => {
        let nodes: [64]QuadNode = undefined;
        let items: [32]QuadItem = undefined;
        let qt = QuadTree.empty();
        const world = Rect2.new(0.0 as f64, 0.0 as f64, 100.0 as f64, 100.0 as f64);
        expect_eq(qt.init(nodes[0..64], world, 2 as u32, 5 as u32), true);

        // Insert 8 points across all 4 quadrants to force subdivision.
        * = qt.insert(nodes[0..64], items[0..32], 10.0 as f64, 10.0 as f64, 1 as u32);
        * = qt.insert(nodes[0..64], items[0..32], 20.0 as f64, 20.0 as f64, 2 as u32);
        * = qt.insert(nodes[0..64], items[0..32], 80.0 as f64, 10.0 as f64, 3 as u32);
        * = qt.insert(nodes[0..64], items[0..32], 10.0 as f64, 80.0 as f64, 4 as u32);
        * = qt.insert(nodes[0..64], items[0..32], 85.0 as f64, 85.0 as f64, 5 as u32);
        * = qt.insert(nodes[0..64], items[0..32], 90.0 as f64, 90.0 as f64, 6 as u32);

        expect_eq(qt.node_count > (1 as usize), true);

        // Query only top-right quadrant [70..100] x [70..100] -> should return IDs 5 and 6.
        let out: [8]u32 = undefined;
        const n = qt.queryRect(nodes[0..64], items[0..32], Rect2.new(70.0 as f64, 70.0 as f64, 100.0 as f64, 100.0 as f64), out[0..8]);
        expect_eq(n, 2 as usize);
        expect_eq(containsId(out[0..n], 5 as u32), true);
        expect_eq(containsId(out[0..n], 6 as u32), true);
    });

    test("3. @cap/quadtree-2d — Circular queryRadius filters corners outside circle", () => {
        let nodes: [32]QuadNode = undefined;
        let items: [16]QuadItem = undefined;
        let qt = QuadTree.empty();
        * = qt.init(nodes[0..32], Rect2.new(0.0 as f64, 0.0 as f64, 100.0 as f64, 100.0 as f64), 2 as u32, 4 as u32);

        // Center (50, 50), radius 10:
        // (50, 55) dist=5 -> IN
        // (58, 56) dist=10 -> IN
        // (59, 59) dist=sqrt(162)≈12.73 (inside bbox [40..60]^2 but outside circle!) -> OUT
        * = qt.insert(nodes[0..32], items[0..16], 50.0 as f64, 55.0 as f64, 10 as u32);
        * = qt.insert(nodes[0..32], items[0..16], 58.0 as f64, 56.0 as f64, 11 as u32);
        * = qt.insert(nodes[0..32], items[0..16], 59.0 as f64, 59.0 as f64, 12 as u32);

        let out: [8]u32 = undefined;
        const n = qt.queryRadius(nodes[0..32], items[0..16], 50.0 as f64, 50.0 as f64, 10.0 as f64, out[0..8]);
        expect_eq(n, 2 as usize);
        expect_eq(containsId(out[0..n], 10 as u32), true);
        expect_eq(containsId(out[0..n], 11 as u32), true);
        expect_eq(containsId(out[0..n], 12 as u32), false);
    });

    test("4. @cap/quadtree-2d — Out-of-bounds point insertion rejected", () => {
        let nodes: [16]QuadNode = undefined;
        let items: [16]QuadItem = undefined;
        let qt = QuadTree.empty();
        * = qt.init(nodes[0..16], Rect2.new(0.0 as f64, 0.0 as f64, 10.0 as f64, 10.0 as f64), 4 as u32, 4 as u32);

        expect_eq(qt.insert(nodes[0..16], items[0..16], -1.0 as f64, 5.0 as f64, 1 as u32), false);
        expect_eq(qt.insert(nodes[0..16], items[0..16], 11.0 as f64, 5.0 as f64, 2 as u32), false);
        expect_eq(qt.item_count, 0 as usize);
    });

    test("5. @cap/quadtree-2d — Duplicate coordinates stop safely at max_depth", () => {
        let nodes: [64]QuadNode = undefined;
        let items: [16]QuadItem = undefined;
        let qt = QuadTree.empty();
        * = qt.init(nodes[0..64], Rect2.new(0.0 as f64, 0.0 as f64, 100.0 as f64, 100.0 as f64), 1 as u32, 4 as u32);

        // Insert 6 identical coordinates: must not infinite-loop!
        let i: u32 = 0 as u32;
        while (i < (6 as u32)) {
            expect_eq(qt.insert(nodes[0..64], items[0..16], 25.0 as f64, 25.0 as f64, i + (1 as u32)), true);
            i += 1 as u32;
        }
        let out: [8]u32 = undefined;
        const n = qt.queryRect(nodes[0..64], items[0..16], Rect2.new(24.0 as f64, 24.0 as f64, 26.0 as f64, 26.0 as f64), out[0..8]);
        expect_eq(n, 6 as usize);
    });

    test("6. @cap/quadtree-2d — Empty tree & full item pool boundary safety", () => {
        let nodes: [8]QuadNode = undefined;
        let items: [2]QuadItem = undefined;
        let emptyQt = QuadTree.empty();
        let out: [4]u32 = undefined;
        expect_eq(emptyQt.queryRect(nodes[0..8], items[0..2], Rect2.new(0.0 as f64, 0.0 as f64, 10.0 as f64, 10.0 as f64), out[0..4]), 0 as usize);

        expect_eq(emptyQt.init(nodes[0..8], Rect2.new(0.0 as f64, 0.0 as f64, 10.0 as f64, 10.0 as f64), 4 as u32, 2 as u32), true);
        expect_eq(emptyQt.insert(nodes[0..8], items[0..2], 1.0 as f64, 1.0 as f64, 1 as u32), true);
        expect_eq(emptyQt.insert(nodes[0..8], items[0..2], 2.0 as f64, 2.0 as f64, 2 as u32), true);
        // 3rd insert exceeds 2-item pool capacity -> returns false safely.
        expect_eq(emptyQt.insert(nodes[0..8], items[0..2], 3.0 as f64, 3.0 as f64, 3 as u32), false);
    });
}
