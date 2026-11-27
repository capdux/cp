import { test, expect_eq } from "cap:test";
import { F64_EPSILON, F32_EPSILON } from "../../packages/approx/src/index.cp";
import {
    F64Margin,
    F32Margin,
    approxEqF64,
    approxNeF64,
    approxEqF32,
    approxNeF32,
    approxEqRatioF64,
    approxEqRatioF32,
    approxOrdF64,
    approxOrdF32,
    ulpsDiffF64,
    ulpsDiffF32,
    nextUp,
    nextDown,
    nextUpF64,
    nextDownF64,
    nextUpF32,
    nextDownF32
} from "../../packages/float-cmp/src/index.cp";

fn main() {
    test("1. @cap/float-cmp — F64Margin & F32Margin builders", () => {
        let d64: F64Margin = F64Margin.defaultMargin();
        expect_eq(d64.epsilon, F64_EPSILON);
        expect_eq(d64.ulps, 4);

        let z64: F64Margin = F64Margin.zero();
        expect_eq(z64.epsilon, 0.0);
        expect_eq(z64.ulps, 0);

        let c64: F64Margin = z64.withEpsilon(1.0e-9).withUlps(8);
        expect_eq(c64.epsilon, 1.0e-9);
        expect_eq(c64.ulps, 8);

        let d32: F32Margin = F32Margin.defaultMargin();
        expect_eq(d32.epsilon, F32_EPSILON);
        expect_eq(d32.ulps, 4);

        let c32: F32Margin = F32Margin.zero().withEpsilon(1.0e-5).withUlps(2);
        expect_eq(c32.epsilon, 1.0e-5);
        expect_eq(c32.ulps, 2);
    });

    test("2. @cap/float-cmp — approxEqF64 dual epsilon & ULPs semantics", () => {
        let x: f64 = 0.1;
        let y: f64 = 0.2;
        let sum: f64 = x + y;
        expect_eq(approxEqF64(sum, 0.3, F64Margin.defaultMargin()), true);
        expect_eq(approxEqF64(sum, 0.3, F64Margin.zero()), false);
        expect_eq(approxEqF64(sum, 0.3, F64Margin.zero().withEpsilon(1.0e-12)), true);

        let a: f64 = 1.0;
        let b: f64 = 1.0 + F64_EPSILON;
        expect_eq(approxEqF64(a, b, F64Margin.zero().withUlps(1)), true);
        expect_eq(approxEqF64(a, b, F64Margin.zero()), false);
        expect_eq(approxEqF64(1.0, 1.05, F64Margin.defaultMargin()), false);
    });

    test("3. @cap/float-cmp — strict NaN, Inf & signed-zero handling", () => {
        let zero: f64 = 0.0;
        let neg_zero: f64 = -0.0;
        let nan: f64 = zero / zero;
        let pos_inf: f64 = 1.0 / zero;
        let neg_inf: f64 = -1.0 / zero;
        let m: F64Margin = F64Margin.defaultMargin();

        expect_eq(approxEqF64(zero, neg_zero, F64Margin.zero()), true);
        expect_eq(approxEqF64(nan, nan, m), false);
        expect_eq(approxEqF64(nan, 0.0, m), false);
        expect_eq(approxEqF64(pos_inf, pos_inf, m), true);
        expect_eq(approxEqF64(neg_inf, neg_inf, m), true);
        expect_eq(approxEqF64(pos_inf, neg_inf, m.withEpsilon(1.0e9)), false);
        expect_eq(approxEqF64(pos_inf, 1.0e300, m), false);
    });

    test("4. @cap/float-cmp — approxEqF32 epsilon, ULPs & NaN/Inf", () => {
        let a: f32 = 1.0;
        let b: f32 = 1.0 + F32_EPSILON;
        expect_eq(approxEqF32(a, b, F32Margin.zero().withUlps(1)), true);
        expect_eq(approxEqF32(a, b, F32Margin.zero()), false);
        expect_eq(approxEqF32(1.0, 1.00001, F32Margin.zero().withEpsilon(1.0e-4)), true);
        expect_eq(approxEqF32(1.0, 1.1, F32Margin.defaultMargin()), false);

        let z32: f32 = 0.0;
        let nan32: f32 = z32 / z32;
        let inf32: f32 = 1.0 / z32;
        expect_eq(approxEqF32(nan32, nan32, F32Margin.defaultMargin()), false);
        expect_eq(approxEqF32(inf32, inf32, F32Margin.defaultMargin()), true);
    });

    test("5. @cap/float-cmp — approxEqRatioF64 relative scaling & edge cases", () => {
        expect_eq(approxEqRatioF64(1000000.0, 1000000.5, 1.0e-6), true);
        expect_eq(approxEqRatioF64(1000000.0, 1000010.0, 1.0e-6), false);
        expect_eq(approxEqRatioF64(0.0, -0.0, 0.0), true);
        expect_eq(approxEqRatioF64(1.0, 1.0, -0.1), false);

        let zero: f64 = 0.0;
        let nan: f64 = zero / zero;
        let pos_inf: f64 = 1.0 / zero;
        expect_eq(approxEqRatioF64(nan, nan, 1.0), false);
        expect_eq(approxEqRatioF64(pos_inf, pos_inf, 1.0e-6), true);
    });

    test("6. @cap/float-cmp — approxOrdF64 approximate total ordering", () => {
        let m: F64Margin = F64Margin.zero().withEpsilon(1.0e-6);
        expect_eq(approxOrdF64(1.0, 1.0000005, m), 0);
        expect_eq(approxOrdF64(1.0, 1.001, m), -1);
        expect_eq(approxOrdF64(1.001, 1.0, m), 1);

        let zero: f64 = 0.0;
        let nan: f64 = zero / zero;
        let pos_inf: f64 = 1.0 / zero;
        expect_eq(approxOrdF64(nan, nan, m), 0);
        expect_eq(approxOrdF64(nan, pos_inf, m), 1);
        expect_eq(approxOrdF64(pos_inf, nan, m), -1);
    });

    test("7. @cap/float-cmp — ulpsDiffF64/F32 & nextUp/nextDown", () => {
        let a64: f64 = 1.0;
        let up64: f64 = nextUp(a64);
        let dn64: f64 = nextDown(a64);
        expect_eq(up64 > a64, true);
        expect_eq(dn64 < a64, true);
        expect_eq(nextDownF64(up64), a64);
        expect_eq(nextUpF64(dn64), a64);
        expect_eq(ulpsDiffF64(up64, a64) ?? 0, 1);
        expect_eq(ulpsDiffF64(dn64, a64) ?? 0, -1);
        // Cross-zero ULP diff: nextDown(0.0) to nextUp(0.0) is 2 ULPs
        expect_eq(ulpsDiffF64(nextUp(0.0), nextDown(0.0)) ?? 0, 2);

        let a32: f32 = 1.0;
        let up32: f32 = nextUpF32(a32);
        let dn32: f32 = nextDownF32(a32);
        expect_eq(up32 > a32, true);
        expect_eq(dn32 < a32, true);
        expect_eq(nextDownF32(up32), a32);
        expect_eq(ulpsDiffF32(up32, a32) ?? 0, 1);
        expect_eq(ulpsDiffF32(dn32, a32) ?? 0, -1);

        let z: f64 = 0.0;
        let nan: f64 = z / z;
        expect_eq(ulpsDiffF64(nan, 1.0) == null, true);
    });

    test("8. @cap/float-cmp — approxNe, approxEqRatioF32, approxOrdF32 & fromTuple", () => {
        let m64 = F64Margin.fromTuple(1.0e-9, 2);
        expect_eq(approxNeF64(1.0, 2.0, m64), true);
        expect_eq(approxNeF64(1.0, 1.0, m64), false);

        let m32 = F32Margin.fromTuple(1.0e-5, 2);
        expect_eq(approxNeF32(1.0, 2.0, m32), true);
        expect_eq(approxEqRatioF32(1000.0, 1000.005, 1.0e-4), true);
        expect_eq(approxOrdF32(1.0, 1.000001, m32), 0);
        expect_eq(approxOrdF32(1.0, 2.0, m32), -1);
        expect_eq(approxOrdF32(2.0, 1.0, m32), 1);
    });
}
