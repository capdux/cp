import { test, expect_eq } from "cap:test";
import {
    Pt2,
    Aabb2,
    fromCenterSize,
    fromPoints,
} from "../../packages/parry2d/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) {
        d = 0.0 - d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/aabb2 — constructors, width, height, area, perimeter & center", () => {
        let eps: f64 = 0.000001;
        let b: Aabb2 = fromCenterSize(5.0, 10.0, 4.0, 6.0);
        expect_eq(b.isValid(), true);
        expect_eq(near(b.min_x, 3.0, eps), true);
        expect_eq(near(b.max_x, 7.0, eps), true);
        expect_eq(near(b.min_y, 7.0, eps), true);
        expect_eq(near(b.max_y, 13.0, eps), true);
        expect_eq(near(b.width(), 4.0, eps), true);
        expect_eq(near(b.height(), 6.0, eps), true);
        expect_eq(near(b.area(), 24.0, eps), true);
        expect_eq(near(b.perimeter(), 20.0, eps), true);

        let c: Pt2 = b.center();
        expect_eq(near(c.x, 5.0, eps), true);
        expect_eq(near(c.y, 10.0, eps), true);
    });

    test("2. @cap/aabb2 — containsPoint & containsBox", () => {
        let outer: Aabb2 = Aabb2.new(0.0, 0.0, 10.0, 10.0);
        let inner: Aabb2 = Aabb2.new(2.0, 3.0, 8.0, 9.0);
        let partial: Aabb2 = Aabb2.new(5.0, 5.0, 12.0, 8.0);

        expect_eq(outer.containsPoint(5.0, 5.0), true);
        expect_eq(outer.containsPoint(0.0, 10.0), true);
        expect_eq(outer.containsPoint(-0.1, 5.0), false);
        expect_eq(outer.containsPoint(5.0, 10.1), false);

        expect_eq(outer.containsBox(inner), true);
        expect_eq(outer.containsBox(outer), true);
        expect_eq(outer.containsBox(partial), false);
        expect_eq(inner.containsBox(outer), false);
    });

    test("3. @cap/aabb2 — intersects, intersection & unionBox", () => {
        let eps: f64 = 0.000001;
        let a: Aabb2 = Aabb2.new(0.0, 0.0, 6.0, 6.0);
        let b: Aabb2 = Aabb2.new(4.0, 2.0, 10.0, 8.0);
        let far: Aabb2 = Aabb2.new(20.0, 20.0, 25.0, 25.0);

        expect_eq(a.intersects(b), true);
        expect_eq(a.intersects(far), false);

        let inter: Aabb2 = a.intersection(b);
        expect_eq(inter.isValid(), true);
        expect_eq(near(inter.min_x, 4.0, eps), true);
        expect_eq(near(inter.min_y, 2.0, eps), true);
        expect_eq(near(inter.max_x, 6.0, eps), true);
        expect_eq(near(inter.max_y, 6.0, eps), true);
        expect_eq(near(inter.area(), 8.0, eps), true);

        let no_inter: Aabb2 = a.intersection(far);
        expect_eq(no_inter.isValid(), false);

        let uni: Aabb2 = a.unionBox(b);
        expect_eq(near(uni.min_x, 0.0, eps), true);
        expect_eq(near(uni.min_y, 0.0, eps), true);
        expect_eq(near(uni.max_x, 10.0, eps), true);
        expect_eq(near(uni.max_y, 8.0, eps), true);
    });

    test("4. @cap/aabb2 — fromPoints, expandBy & expandToPoint", () => {
        let eps: f64 = 0.000001;
        let pts: [4]Pt2 = [
            Pt2.new(1.0, 4.0),
            Pt2.new(-2.0, 0.5),
            Pt2.new(3.5, -1.0),
            Pt2.new(0.0, 2.0),
        ];
        let b: Aabb2 = fromPoints(pts[0..4]);
        expect_eq(b.isValid(), true);
        expect_eq(near(b.min_x, -2.0, eps), true);
        expect_eq(near(b.min_y, -1.0, eps), true);
        expect_eq(near(b.max_x, 3.5, eps), true);
        expect_eq(near(b.max_y, 4.0, eps), true);

        let exp_m: Aabb2 = b.expandBy(1.5);
        expect_eq(near(exp_m.min_x, -3.5, eps), true);
        expect_eq(near(exp_m.max_y, 5.5, eps), true);

        let exp_p: Aabb2 = b.expandToPoint(10.0, -5.0);
        expect_eq(near(exp_p.max_x, 10.0, eps), true);
        expect_eq(near(exp_p.min_y, -5.0, eps), true);
    });

    test("5. @cap/aabb2 — distanceSqToPoint inside, edge & corner", () => {
        let eps: f64 = 0.000001;
        let b: Aabb2 = Aabb2.new(0.0, 0.0, 4.0, 4.0);

        expect_eq(near(b.distanceSqToPoint(2.0, 2.0), 0.0, eps), true);
        expect_eq(near(b.distanceSqToPoint(0.0, 4.0), 0.0, eps), true);
        expect_eq(near(b.distanceSqToPoint(7.0, 2.0), 9.0, eps), true);
        expect_eq(near(b.distanceSqToPoint(7.0, 8.0), 25.0, eps), true);
        expect_eq(near(b.distanceSqToPoint(-3.0, -4.0), 25.0, eps), true);
    });

    test("6. @cap/aabb2 — 2D rayIntersects slab hits & misses", () => {
        let b: Aabb2 = Aabb2.new(2.0, 2.0, 6.0, 6.0);

        // Diagonal hit from origin
        expect_eq(b.rayIntersects(0.0, 0.0, 1.0, 1.0), true);
        // Horizontal parallel ray hitting box
        expect_eq(b.rayIntersects(0.0, 4.0, 1.0, 0.0), true);
        // Horizontal parallel ray missing box (y outside)
        expect_eq(b.rayIntersects(0.0, 7.0, 1.0, 0.0), false);
        // Ray pointing away from box (behind origin)
        expect_eq(b.rayIntersects(0.0, 0.0, -1.0, -1.0), false);
        // Ray originating inside box
        expect_eq(b.rayIntersects(4.0, 4.0, -1.0, 0.5), true);
        // Diagonal ray grazing miss
        expect_eq(b.rayIntersects(0.0, 5.0, 1.0, 1.0), false);
    });
}
