// cap/test/testCapAabb3.cp
// Comprehensive unit tests for @cap/aabb3

import { test, expect_eq } from "cap:test";
import {
    Aabb3,
} from "../../packages/@cap/aabb3/src/index.cp";

fn near(a: f64, b: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= (0.000001 as f64);
}

fn main() {
    test("1. @cap/aabb3 — Volume, surfaceArea & center fromCenterSize", () => {
        const b = Aabb3.fromCenterSize(1.0 as f64, 2.0 as f64, 3.0 as f64, 4.0 as f64, 6.0 as f64, 8.0 as f64);
        expect_eq(b.isValid(), true);
        expect_eq(near(b.volume(), 192.0 as f64), true);
        // 2 * (4*6 + 6*8 + 8*4) = 2 * (24 + 48 + 32) = 208
        expect_eq(near(b.surfaceArea(), 208.0 as f64), true);

        const c = b.center();
        expect_eq(near(c.x, 1.0 as f64), true);
        expect_eq(near(c.y, 2.0 as f64), true);
        expect_eq(near(c.z, 3.0 as f64), true);
    });

    test("2. @cap/aabb3 — containsPoint inside, on boundary & outside", () => {
        const b = Aabb3.new(0.0 as f64, 0.0 as f64, 0.0 as f64, 10.0 as f64, 10.0 as f64, 10.0 as f64);
        expect_eq(b.containsPoint(5.0 as f64, 5.0 as f64, 5.0 as f64), true);
        expect_eq(b.containsPoint(0.0 as f64, 10.0 as f64, 5.0 as f64), true);
        expect_eq(b.containsPoint(-0.1 as f64, 5.0 as f64, 5.0 as f64), false);
        expect_eq(b.containsPoint(5.0 as f64, 10.1 as f64, 5.0 as f64), false);
    });

    test("3. @cap/aabb3 — intersects, intersection & unionBox", () => {
        const a = Aabb3.new(0.0 as f64, 0.0 as f64, 0.0 as f64, 4.0 as f64, 4.0 as f64, 4.0 as f64);
        const b = Aabb3.new(2.0 as f64, 2.0 as f64, 2.0 as f64, 6.0 as f64, 6.0 as f64, 6.0 as f64);
        const far = Aabb3.new(10.0 as f64, 10.0 as f64, 10.0 as f64, 12.0 as f64, 12.0 as f64, 12.0 as f64);

        expect_eq(a.intersects(b), true);
        expect_eq(a.intersects(far), false);

        const inter = a.intersection(b);
        expect_eq(inter.isValid(), true);
        expect_eq(near(inter.volume(), 8.0 as f64), true);

        const emptyInter = a.intersection(far);
        expect_eq(emptyInter.isValid(), false);
        expect_eq(near(emptyInter.volume(), 0.0 as f64), true);

        const uni = a.unionBox(b);
        expect_eq(near(uni.volume(), 216.0 as f64), true);
    });

    test("4. @cap/aabb3 — expandBy positive margin & negative shrink", () => {
        const b = Aabb3.new(1.0 as f64, 1.0 as f64, 1.0 as f64, 3.0 as f64, 3.0 as f64, 3.0 as f64);
        const grown = b.expandBy(1.0 as f64);
        expect_eq(near(grown.min_x, 0.0 as f64), true);
        expect_eq(near(grown.max_x, 4.0 as f64), true);
        expect_eq(near(grown.volume(), 64.0 as f64), true);

        const shrunk = b.expandBy(-0.5 as f64);
        expect_eq(near(shrunk.volume(), 1.0 as f64), true);
    });

    test("5. @cap/aabb3 — distanceSqToPoint for interior, face & corner exterior points", () => {
        const b = Aabb3.new(0.0 as f64, 0.0 as f64, 0.0 as f64, 2.0 as f64, 2.0 as f64, 2.0 as f64);
        expect_eq(near(b.distanceSqToPoint(1.0 as f64, 1.0 as f64, 1.0 as f64), 0.0 as f64), true);
        // 3 units past max_x -> dist^2 = 9
        expect_eq(near(b.distanceSqToPoint(5.0 as f64, 1.0 as f64, 1.0 as f64), 9.0 as f64), true);
        // Corner offset (3, 4, 0) -> dist^2 = 25
        expect_eq(near(b.distanceSqToPoint(5.0 as f64, 6.0 as f64, 2.0 as f64), 25.0 as f64), true);
    });

    test("6. @cap/aabb3 — 3D Kay-Kajiya Slab rayIntersects hit, miss & parallel-axis boundaries", () => {
        const b = Aabb3.new(-1.0 as f64, -1.0 as f64, -1.0 as f64, 1.0 as f64, 1.0 as f64, 1.0 as f64);

        // Parallel to Y and Z axes, shooting along +X through center -> hit
        expect_eq(b.rayIntersects(-5.0 as f64, 0.0 as f64, 0.0 as f64, 1.0 as f64, 0.0 as f64, 0.0 as f64), true);
        // Shooting away (-X) from x=-5 -> miss (t < 0)
        expect_eq(b.rayIntersects(-5.0 as f64, 0.0 as f64, 0.0 as f64, -1.0 as f64, 0.0 as f64, 0.0 as f64), false);
        // Parallel ray outside Y slab -> miss
        expect_eq(b.rayIntersects(-5.0 as f64, 2.0 as f64, 0.0 as f64, 1.0 as f64, 0.0 as f64, 0.0 as f64), false);
        // Diagonal ray hitting box
        expect_eq(b.rayIntersects(-3.0 as f64, -3.0 as f64, -3.0 as f64, 1.0 as f64, 1.0 as f64, 1.0 as f64), true);
        // Diagonal ray missing box
        expect_eq(b.rayIntersects(-3.0 as f64, 5.0 as f64, -3.0 as f64, 1.0 as f64, 0.0 as f64, 1.0 as f64), false);
    });
}
