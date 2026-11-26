import { test, expect_eq } from "cap:test";
import {
    V3,
    Ray3,
    HitResult,
    rayAt,
    rayPlane,
    raySphere,
    rayTriangle,
    raySegment2D,
} from "../../packages/@cap/ray-cast/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) {
        d = 0.0 - d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/ray-cast — rayPlane orthogonal hit, behind-origin & parallel miss", () => {
        let eps: f64 = 0.000001;
        let plane_pt: V3 = V3.new(0.0, 0.0, 5.0);
        let normal: V3 = V3.new(0.0, 0.0, -1.0);

        let r_hit: Ray3 = Ray3.new(1.0, 2.0, 0.0, 0.0, 0.0, 1.0);
        let h1: HitResult = rayPlane(r_hit, plane_pt, normal);
        expect_eq(h1.hit, true);
        expect_eq(near(h1.t, 5.0, eps), true);

        // Ray pointing away (t < 0)
        let r_back: Ray3 = Ray3.new(0.0, 0.0, 0.0, 0.0, 0.0, -1.0);
        expect_eq(rayPlane(r_back, plane_pt, normal).hit, false);

        // Parallel ray
        let r_par: Ray3 = Ray3.new(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        expect_eq(rayPlane(r_par, plane_pt, normal).hit, false);
    });

    test("2. @cap/ray-cast — raySphere outside hit, tangent, inside origin & miss", () => {
        let eps: f64 = 0.000001;
        let center: V3 = V3.new(0.0, 0.0, 10.0);

        // Outside piercing hit at z = 8
        let r1: Ray3 = Ray3.new(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
        let h1: HitResult = raySphere(r1, center, 2.0);
        expect_eq(h1.hit, true);
        expect_eq(near(h1.t, 8.0, eps), true);

        // Tangent hit at y = 2, z = 10
        let r_tan: Ray3 = Ray3.new(0.0, 2.0, 0.0, 0.0, 0.0, 1.0);
        let h_tan: HitResult = raySphere(r_tan, center, 2.0);
        expect_eq(h_tan.hit, true);
        expect_eq(near(h_tan.t, 10.0, eps), true);

        // Ray originating inside sphere exits at t = 2
        let r_in: Ray3 = Ray3.new(0.0, 0.0, 10.0, 0.0, 0.0, 1.0);
        let h_in: HitResult = raySphere(r_in, center, 2.0);
        expect_eq(h_in.hit, true);
        expect_eq(near(h_in.t, 2.0, eps), true);

        // Complete miss
        let r_miss: Ray3 = Ray3.new(0.0, 5.0, 0.0, 0.0, 0.0, 1.0);
        expect_eq(raySphere(r_miss, center, 2.0).hit, false);
    });

    test("3. @cap/ray-cast — rayTriangle Moller-Trumbore barycentric hit & outside miss", () => {
        let eps: f64 = 0.000001;
        let v0: V3 = V3.new(0.0, 0.0, 4.0);
        let v1: V3 = V3.new(4.0, 0.0, 4.0);
        let v2: V3 = V3.new(0.0, 4.0, 4.0);

        // Hit at (1, 1, 4) -> u = 0.25, v = 0.25, t = 4.0
        let r_in: Ray3 = Ray3.new(1.0, 1.0, 0.0, 0.0, 0.0, 1.0);
        let h: HitResult = rayTriangle(r_in, v0, v1, v2);
        expect_eq(h.hit, true);
        expect_eq(near(h.t, 4.0, eps), true);
        expect_eq(near(h.u, 0.25, eps), true);
        expect_eq(near(h.v, 0.25, eps), true);

        // Outside triangle (u + v > 1)
        let r_out: Ray3 = Ray3.new(3.0, 3.0, 0.0, 0.0, 0.0, 1.0);
        expect_eq(rayTriangle(r_out, v0, v1, v2).hit, false);
    });

    test("4. @cap/ray-cast — rayAt computes exact intersection point", () => {
        let eps: f64 = 0.000001;
        let r: Ray3 = Ray3.new(1.0, 2.0, 0.0, 0.0, 0.0, 2.0);
        let h: HitResult = rayPlane(r, V3.new(0.0, 0.0, 10.0), V3.new(0.0, 0.0, 1.0));
        expect_eq(h.hit, true);
        let pt: V3 = rayAt(r, h.t);
        expect_eq(near(pt.x, 1.0, eps), true);
        expect_eq(near(pt.y, 2.0, eps), true);
        expect_eq(near(pt.z, 10.0, eps), true);
    });

    test("5. @cap/ray-cast — raySegment2D segment hit & extension line rejection", () => {
        let eps: f64 = 0.000001;
        // Ray from (0,0) along +X hits vertical segment (5, -2) -> (5, 2) at t=5, u=0.5
        let h1: HitResult = raySegment2D(0.0, 0.0, 1.0, 0.0, 5.0, -2.0, 5.0, 2.0);
        expect_eq(h1.hit, true);
        expect_eq(near(h1.t, 5.0, eps), true);
        expect_eq(near(h1.u, 0.5, eps), true);

        // Ray passes above segment (on infinite line extension, u > 1) -> miss
        let h2: HitResult = raySegment2D(0.0, 3.0, 1.0, 0.0, 5.0, -2.0, 5.0, 2.0);
        expect_eq(h2.hit, false);

        // Ray pointing away -> miss
        let h3: HitResult = raySegment2D(0.0, 0.0, -1.0, 0.0, 5.0, -2.0, 5.0, 2.0);
        expect_eq(h3.hit, false);
    });

    test("6. @cap/ray-cast — zero direction vector safety across all intersections", () => {
        let z_ray: Ray3 = Ray3.new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        expect_eq(rayPlane(z_ray, V3.new(0.0, 0.0, 1.0), V3.new(0.0, 0.0, 1.0)).hit, false);
        expect_eq(raySphere(z_ray, V3.new(0.0, 0.0, 5.0), 1.0).hit, false);
        expect_eq(
            rayTriangle(z_ray, V3.new(0.0, 0.0, 1.0), V3.new(1.0, 0.0, 1.0), V3.new(0.0, 1.0, 1.0)).hit,
            false
        );
        expect_eq(raySegment2D(0.0, 0.0, 0.0, 0.0, 1.0, -1.0, 1.0, 1.0).hit, false);
    });
}
