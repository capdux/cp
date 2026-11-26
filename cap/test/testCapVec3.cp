import { test, expect_eq } from "cap:test";
import {
    Vec3,
    add,
    sub,
    mul,
    scale,
    neg,
    dot,
    cross,
    lengthSq,
    length,
    distance,
    normalize,
    lerp,
    reflect,
    project,
    minVec,
    maxVec,
    clampLength,
    approxEq,
} from "../../packages/@cap/vec3/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) {
        d = 0.0 - d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/vec3 — constructors, add, sub, mul, scale & neg", () => {
        let eps: f64 = 0.000000001;
        let a: Vec3 = Vec3.new(1.0, 2.0, 3.0);
        let b: Vec3 = Vec3.new(4.0, 5.0, 6.0);
        let s: Vec3 = Vec3.splat(2.5);
        let z: Vec3 = Vec3.zero();

        expect_eq(approxEq(s, Vec3.new(2.5, 2.5, 2.5), eps), true);
        expect_eq(approxEq(z, Vec3.new(0.0, 0.0, 0.0), eps), true);
        expect_eq(approxEq(add(a, b), Vec3.new(5.0, 7.0, 9.0), eps), true);
        expect_eq(approxEq(sub(b, a), Vec3.new(3.0, 3.0, 3.0), eps), true);
        expect_eq(approxEq(mul(a, b), Vec3.new(4.0, 10.0, 18.0), eps), true);
        expect_eq(approxEq(scale(a, 3.0), Vec3.new(3.0, 6.0, 9.0), eps), true);
        expect_eq(approxEq(neg(a), Vec3.new(-1.0, -2.0, -3.0), eps), true);
    });

    test("2. @cap/vec3 — dot product & right-handed 3D cross product", () => {
        let eps: f64 = 0.000000001;
        let ux: Vec3 = Vec3.unitX();
        let uy: Vec3 = Vec3.unitY();
        let uz: Vec3 = Vec3.unitZ();

        expect_eq(approxEq(cross(ux, uy), uz, eps), true);
        expect_eq(approxEq(cross(uy, uz), ux, eps), true);
        expect_eq(approxEq(cross(uz, ux), uy, eps), true);
        expect_eq(approxEq(cross(uy, ux), uz.neg(), eps), true);

        expect_eq(near(dot(ux, uy), 0.0, eps), true);
        let a: Vec3 = Vec3.new(1.0, 2.0, 3.0);
        let b: Vec3 = Vec3.new(4.0, -5.0, 6.0);
        expect_eq(near(dot(a, b), 12.0, eps), true);

        let c: Vec3 = cross(a, b);
        expect_eq(near(dot(c, a), 0.0, eps), true);
        expect_eq(near(dot(c, b), 0.0, eps), true);
    });

    test("3. @cap/vec3 — lengthSq, length & normalize", () => {
        let eps: f64 = 0.000000001;
        let v: Vec3 = Vec3.new(2.0, 3.0, 6.0);
        expect_eq(near(lengthSq(v), 49.0, eps), true);
        expect_eq(near(length(v), 7.0, eps), true);

        let n: Vec3 = normalize(v);
        expect_eq(near(n.length(), 1.0, eps), true);
        expect_eq(approxEq(n, Vec3.new(2.0 / 7.0, 3.0 / 7.0, 6.0 / 7.0), eps), true);
    });

    test("4. @cap/vec3 — distance & lerp", () => {
        let eps: f64 = 0.000000001;
        let a: Vec3 = Vec3.new(1.0, 2.0, 3.0);
        let b: Vec3 = Vec3.new(4.0, 6.0, 3.0);

        expect_eq(near(distance(a, b), 5.0, eps), true);
        expect_eq(approxEq(lerp(a, b, 0.0), a, eps), true);
        expect_eq(approxEq(lerp(a, b, 1.0), b, eps), true);
        expect_eq(approxEq(lerp(a, b, 0.5), Vec3.new(2.5, 4.0, 3.0), eps), true);
        expect_eq(approxEq(lerp(a, b, 0.25), Vec3.new(1.75, 3.0, 3.0), eps), true);
    });

    test("5. @cap/vec3 — reflect & project", () => {
        let eps: f64 = 0.000000001;
        let incident: Vec3 = Vec3.new(1.0, -1.0, 0.0);
        let normal: Vec3 = Vec3.unitY();
        let refl: Vec3 = reflect(incident, normal);
        expect_eq(approxEq(refl, Vec3.new(1.0, 1.0, 0.0), eps), true);

        let v: Vec3 = Vec3.new(3.0, 4.0, 5.0);
        let onto: Vec3 = Vec3.new(2.0, 0.0, 0.0);
        let proj: Vec3 = project(v, onto);
        expect_eq(approxEq(proj, Vec3.new(3.0, 0.0, 0.0), eps), true);

        let diag: Vec3 = Vec3.new(1.0, 1.0, 0.0);
        let proj2: Vec3 = project(Vec3.new(2.0, 0.0, 0.0), diag);
        expect_eq(approxEq(proj2, Vec3.new(1.0, 1.0, 0.0), eps), true);
    });

    test("6. @cap/vec3 — minVec, maxVec, clampLength & zero-vector safety", () => {
        let eps: f64 = 0.000000001;
        let a: Vec3 = Vec3.new(1.0, 5.0, -2.0);
        let b: Vec3 = Vec3.new(3.0, 2.0, 4.0);

        expect_eq(approxEq(minVec(a, b), Vec3.new(1.0, 2.0, -2.0), eps), true);
        expect_eq(approxEq(maxVec(a, b), Vec3.new(3.0, 5.0, 4.0), eps), true);

        let long_v: Vec3 = Vec3.new(3.0, 4.0, 0.0);
        let clamped: Vec3 = clampLength(long_v, 2.5);
        expect_eq(near(clamped.length(), 2.5, eps), true);
        expect_eq(approxEq(clamped, Vec3.new(1.5, 2.0, 0.0), eps), true);
        expect_eq(approxEq(clampLength(long_v, 10.0), long_v, eps), true);

        let z: Vec3 = Vec3.zero();
        expect_eq(approxEq(normalize(z), z, eps), true);
        expect_eq(approxEq(project(a, z), z, eps), true);
        expect_eq(approxEq(clampLength(z, 5.0), z, eps), true);
        expect_eq(approxEq(clampLength(long_v, 0.0), z, eps), true);
    });
}
