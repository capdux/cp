import { test, expect_eq } from "cap:test";
import {
    PI,
    V2,
    Mat2,
    identity,
    zero,
    fromScale,
    fromRotation,
    fromShear,
    add,
    sub,
    scale,
    mul,
    mulVec,
    transpose,
    trace,
    determinant,
    isInvertible,
    inverse,
    frobeniusNormSq,
    approxEq,
} from "../../packages/glam/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) {
        d = 0.0 - d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/mat2 — identity, scale & 90-degree rotation", () => {
        let eps: f64 = 0.000000001;
        let id: Mat2 = identity();
        let v: V2 = V2.new(3.0, -4.0);
        expect_eq(mulVec(id, v).approxEq(v, eps), true);

        let sc: Mat2 = fromScale(2.0, 5.0);
        expect_eq(mulVec(sc, V2.new(3.0, 4.0)).approxEq(V2.new(6.0, 20.0), eps), true);

        let rot90: Mat2 = fromRotation(PI * 0.5);
        expect_eq(mulVec(rot90, V2.new(1.0, 0.0)).approxEq(V2.new(0.0, 1.0), eps), true);
        expect_eq(mulVec(rot90, V2.new(0.0, 1.0)).approxEq(V2.new(-1.0, 0.0), eps), true);
    });

    test("2. @cap/mat2 — matrix multiplication & mulVec composition", () => {
        let eps: f64 = 0.000000001;
        let a: Mat2 = Mat2.new(1.0, 2.0, 3.0, 4.0);
        let b: Mat2 = Mat2.new(5.0, 6.0, 7.0, 8.0);
        let prod: Mat2 = mul(a, b);
        expect_eq(approxEq(prod, Mat2.new(19.0, 22.0, 43.0, 50.0), eps), true);

        let v: V2 = V2.new(2.0, -1.0);
        let lhs: V2 = mulVec(prod, v);
        let rhs: V2 = mulVec(a, mulVec(b, v));
        expect_eq(lhs.approxEq(rhs, eps), true);
    });

    test("3. @cap/mat2 — determinant, trace, transpose & frobeniusNormSq", () => {
        let eps: f64 = 0.000000001;
        let m: Mat2 = Mat2.new(4.0, 7.0, 2.0, 6.0);
        expect_eq(near(determinant(m), 10.0, eps), true);
        expect_eq(near(trace(m), 10.0, eps), true);
        expect_eq(approxEq(transpose(m), Mat2.new(4.0, 2.0, 7.0, 6.0), eps), true);
        expect_eq(near(frobeniusNormSq(m), 105.0, eps), true);
    });

    test("4. @cap/mat2 — invertible matrix M * inv(M) == I", () => {
        let eps: f64 = 0.000000001;
        let m: Mat2 = Mat2.new(4.0, 7.0, 2.0, 6.0);
        expect_eq(isInvertible(m), true);

        let inv_m: Mat2 = inverse(m);
        expect_eq(approxEq(inv_m, Mat2.new(0.6, -0.7, -0.2, 0.4), eps), true);
        expect_eq(approxEq(mul(m, inv_m), identity(), eps), true);
        expect_eq(approxEq(mul(inv_m, m), identity(), eps), true);
    });

    test("5. @cap/mat2 — singular matrix safety", () => {
        let eps: f64 = 0.000000001;
        let sing: Mat2 = Mat2.new(2.0, 4.0, 1.0, 2.0);
        expect_eq(near(determinant(sing), 0.0, eps), true);
        expect_eq(isInvertible(sing), false);
        expect_eq(approxEq(inverse(sing), zero(), eps), true);
        expect_eq(isInvertible(zero()), false);
    });

    test("6. @cap/mat2 — shear, add, sub & scale", () => {
        let eps: f64 = 0.000000001;
        let sh: Mat2 = fromShear(2.0, 0.5);
        expect_eq(mulVec(sh, V2.new(2.0, 4.0)).approxEq(V2.new(10.0, 5.0), eps), true);

        let a: Mat2 = Mat2.new(1.0, 2.0, 3.0, 4.0);
        let b: Mat2 = Mat2.new(10.0, 20.0, 30.0, 40.0);
        expect_eq(approxEq(add(a, b), Mat2.new(11.0, 22.0, 33.0, 44.0), eps), true);
        expect_eq(approxEq(sub(b, a), Mat2.new(9.0, 18.0, 27.0, 36.0), eps), true);
        expect_eq(approxEq(scale(a, 3.0), Mat2.new(3.0, 6.0, 9.0, 12.0), eps), true);
    });
}
