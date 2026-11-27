import { test, expect_eq } from "cap:test";
import {
    F64_EPSILON, F32_EPSILON,
    absDiffEqF64, absDiffEqF32, relativeEqF64, relativeEqF32,
    ulpsDistanceF64, ulpsEqF64, ulpsDistanceF32, ulpsEqF32
} from "../../packages/approx/src/index.cp";

fn main() {
    test("1. @cap/approx — absDiffEqF64 handles 0.1 + 0.2 ~= 0.3 & rejects NaN", () => {
        let sum: f64 = 0.1 + 0.2;
        expect_eq(absDiffEqF64(sum, 0.3, 1.0e-12), true);
        expect_eq(absDiffEqF64(1.0, 1.05, 0.01), false);
        let zero: f64 = 0.0;
        let nan: f64 = zero / zero;
        expect_eq(absDiffEqF64(nan, nan, 1.0), false);
    });

    test("2. @cap/approx — relativeEqF64 scales tolerance with magnitude", () => {
        expect_eq(relativeEqF64(1000000.0, 1000000.5, F64_EPSILON, 1.0e-6), true);
        expect_eq(relativeEqF64(1000000.0, 1000010.0, F64_EPSILON, 1.0e-6), false);
        expect_eq(relativeEqF64(0.0, -0.0, F64_EPSILON, F64_EPSILON), true);
    });

    test("3. @cap/approx — ulpsDistanceF64 & ulpsEqF64 measure exact IEEE-754 bit steps", () => {
        let a: f64 = 1.0;
        let b: f64 = 1.0 + F64_EPSILON;
        expect_eq(ulpsDistanceF64(a, b) ?? 99, 1);
        expect_eq(ulpsEqF64(a, b, 0.0, 1), true);
        expect_eq(ulpsEqF64(a, b, 0.0, 0), false);
        expect_eq(ulpsDistanceF64(-1.0, 1.0) == null, true);
    });

    test("4. @cap/approx — Inf handling (+Inf == +Inf, +Inf != -Inf)", () => {
        let zero: f64 = 0.0;
        let pos_inf: f64 = 1.0 / zero;
        let neg_inf: f64 = -1.0 / zero;
        expect_eq(absDiffEqF64(pos_inf, pos_inf, 1.0e-9), true);
        expect_eq(absDiffEqF64(pos_inf, neg_inf, 1.0e9), false);
        expect_eq(relativeEqF64(pos_inf, pos_inf, 1.0e-9, 1.0e-9), true);
        expect_eq(ulpsEqF64(pos_inf, pos_inf, 0.0, 0), true);
    });

    test("5. @cap/approx — f32 absDiffEqF32 & relativeEqF32", () => {
        let a: f32 = 1.0;
        let b: f32 = 1.000001;
        expect_eq(absDiffEqF32(a, b, 1.0e-5), true);
        expect_eq(relativeEqF32(a, b, F32_EPSILON, 1.0e-5), true);
        expect_eq(absDiffEqF32(a, 1.1, 1.0e-5), false);
    });

    test("6. @cap/approx — f32 ulpsDistanceF32 & ulpsEqF32", () => {
        let a: f32 = 1.0;
        let b: f32 = 1.0 + F32_EPSILON;
        expect_eq(ulpsDistanceF32(a, b) ?? 99, 1);
        expect_eq(ulpsEqF32(a, b, 0.0, 1), true);
        expect_eq(ulpsEqF32(a, b, 0.0, 0), false);
    });
}
