// cap/test/testCapRayCast.cp
// Comprehensive unit tests for @cap/parry3d (3D AABB ray intersection & spatial queries)

import { test, expect_eq } from "cap:test";
import { Aabb3 } from "../../packages/parry3d/src/index.cp";

fn rcNear(a: f64, b: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) { d = -d; }
    return d <= 0.000001;
}

fn main() {
    test("1. @cap/parry3d — Aabb3 rayIntersects hit, miss & opposite direction", () => {
        const box = Aabb3.new(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
        expect_eq(box.isValid(), true);
        // Ray from (-5, 0, 0) pointing +X hits box
        expect_eq(box.rayIntersects(-5.0, 0.0, 0.0, 1.0, 0.0, 0.0), true);
        // Ray pointing away (-X) misses
        expect_eq(box.rayIntersects(-5.0, 0.0, 0.0, -1.0, 0.0, 0.0), false);
        // Parallel ray outside Y range misses
        expect_eq(box.rayIntersects(-5.0, 2.5, 0.0, 1.0, 0.0, 0.0), false);
    });

    test("2. @cap/parry3d — Aabb3 intersects, expandBy & distanceSqToPoint", () => {
        const a = Aabb3.new(0.0, 0.0, 0.0, 2.0, 2.0, 2.0);
        const b = Aabb3.new(1.0, 1.0, 1.0, 3.0, 3.0, 3.0);
        expect_eq(a.intersects(b), true);

        const exp = a.expandBy(1.0);
        expect_eq(exp.containsPoint(-0.5, 2.5, 1.0), true);
        expect_eq(rcNear(a.distanceSqToPoint(5.0, 2.0, 2.0), 9.0), true);
    });
}
