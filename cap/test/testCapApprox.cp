import { test, expect_eq } from "cap:test";
import {
    F64_EPSILON, F32_EPSILON,
    absDiffEq, absDiffNe, absDiffEqF64, absDiffNeF64, absDiffEqF32, absDiffNeF32,
    relativeEq, relativeNe, relativeEqF64, relativeNeF64, relativeEqF32, relativeNeF32,
    ulpsDistanceF64, ulpsEq, ulpsNe, ulpsEqF64, ulpsNeF64, ulpsDistanceF32, ulpsEqF32, ulpsNeF32,
    absDiffEqDefaultF64, absDiffEqDefaultF32, relativeEqDefaultF64, relativeEqDefaultF32,
    ulpsEqDefaultF64, ulpsEqDefaultF32,
    absDiffEqSliceF64, absDiffEqSliceF32, relativeEqSliceF64, relativeEqSliceF32,
    ulpsEqSliceF64, ulpsEqSliceF32
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

    test("7. @cap/approx — absDiffNe, relativeNe, ulpsNe & default aliases for f64/f32", () => {
        expect_eq(absDiffEq(0.1 + 0.2, 0.3, 1.0e-12), true);
        expect_eq(absDiffNe(1.0, 1.1, 0.01), true);
        expect_eq(absDiffNeF64(1.0, 1.0, 1.0e-9), false);
        expect_eq(absDiffNeF32(1.0, 1.5, 0.01), true);
        expect_eq(relativeEq(1000.0, 1000.001, F64_EPSILON, 1.0e-5), true);
        expect_eq(relativeNe(1000.0, 1010.0, F64_EPSILON, 1.0e-5), true);
        expect_eq(relativeNeF64(1.0, 1.0, F64_EPSILON, F64_EPSILON), false);
        expect_eq(relativeNeF32(1.0, 2.0, F32_EPSILON, 1.0e-4), true);
        expect_eq(ulpsEq(1.0, 1.0 + F64_EPSILON, 0.0, 1), true);
        expect_eq(ulpsNe(1.0, 1.0 + F64_EPSILON, 0.0, 0), true);
        expect_eq(ulpsNeF64(1.0, 1.0, 0.0, 0), false);
        expect_eq(ulpsNeF32(1.0, 1.0 + F32_EPSILON, 0.0, 0), true);

        expect_eq(absDiffEqDefaultF64(1.0, 1.0 + F64_EPSILON), true);
        expect_eq(absDiffEqDefaultF32(1.0, 1.0 + F32_EPSILON), true);
        expect_eq(relativeEqDefaultF64(1.0, 1.0 + F64_EPSILON), true);
        expect_eq(relativeEqDefaultF32(1.0, 1.0 + F32_EPSILON), true);
        expect_eq(ulpsEqDefaultF64(1.0, 1.0 + F64_EPSILON), true);
        expect_eq(ulpsEqDefaultF32(1.0, 1.0 + F32_EPSILON), true);
    });

    test("8. @cap/approx — slice comparisons for f64 and f32", () => {
        const a64: [3]f64 = [1.0, 2.0, 3.0];
        const b64: [3]f64 = [1.0 + F64_EPSILON, 2.0, 3.0];
        const c64: [2]f64 = [1.0, 2.0];
        expect_eq(absDiffEqSliceF64(a64[0..3], b64[0..3], 1.0e-12), true);
        expect_eq(absDiffEqSliceF64(a64[0..3], c64[0..2], 1.0), false);
        expect_eq(relativeEqSliceF64(a64[0..3], b64[0..3], F64_EPSILON, 1.0e-12), true);
        expect_eq(ulpsEqSliceF64(a64[0..3], b64[0..3], 0.0, 2), true);

        const a32: [2]f32 = [1.0, 2.0];
        const b32: [2]f32 = [1.0 + F32_EPSILON, 2.0];
        expect_eq(absDiffEqSliceF32(a32[0..2], b32[0..2], 1.0e-6), true);
        expect_eq(relativeEqSliceF32(a32[0..2], b32[0..2], F32_EPSILON, 1.0e-6), true);
        expect_eq(ulpsEqSliceF32(a32[0..2], b32[0..2], 0.0, 2), true);
    });
}
