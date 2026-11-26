import { test, expect_eq } from "cap:test";
import { Fixed16, ONE_RAW } from "../../packages/@cap/fixed-q16/src/index.cp";

fn approx(a: f64, b: f64): bool {
    let d = a - b;
    if (d < 0.0) { d = -d; }
    return d < 0.001;
}

fn main() {
    test("fromInt, fromF64 & toF64 conversion", () => {
        let a = Fixed16.fromInt(3);
        expect_eq(a.raw, 3 * ONE_RAW);
        expect_eq(approx(a.toF64(), 3.0), true);
        let b = Fixed16.fromF64(-2.25);
        expect_eq(approx(b.toF64(), -2.25), true);
    });

    test("add, sub, neg & abs", () => {
        let a = Fixed16.fromF64(1.5);
        let b = Fixed16.fromF64(2.25);
        expect_eq(approx(a.add(b).toF64(), 3.75), true);
        expect_eq(approx(a.sub(b).toF64(), -0.75), true);
        expect_eq(approx(a.sub(b).abs().toF64(), 0.75), true);
        expect_eq(approx(a.neg().toF64(), -1.5), true);
    });

    test("mul & div deterministic precision", () => {
        let a = Fixed16.fromF64(3.5);
        let b = Fixed16.fromF64(-2.0);
        expect_eq(approx(a.mul(b).toF64(), -7.0), true);
        expect_eq(approx(a.div(b).toF64(), -1.75), true);
        expect_eq(a.div(Fixed16.zero()).raw, 0);
    });

    test("floorInt, ceilInt & roundInt", () => {
        let a = Fixed16.fromF64(2.75);
        expect_eq(a.floorInt(), 2);
        expect_eq(a.ceilInt(), 3);
        expect_eq(a.roundInt(), 3);
        let b = Fixed16.fromInt(4);
        expect_eq(b.ceilInt(), 4);
        let c = Fixed16.fromF64(-1.25);
        expect_eq(c.floorInt(), -2);
    });

    test("clamp & lerp", () => {
        let lo = Fixed16.fromInt(0);
        let hi = Fixed16.fromInt(10);
        expect_eq(Fixed16.fromInt(-5).clamp(lo, hi).raw, 0);
        expect_eq(Fixed16.fromInt(15).clamp(lo, hi).raw, 10 * ONE_RAW);
        let mid = lo.lerp(hi, Fixed16.fromF64(0.25));
        expect_eq(approx(mid.toF64(), 2.5), true);
    });

    test("zero & one identities", () => {
        let x = Fixed16.fromF64(7.125);
        expect_eq(x.mul(Fixed16.one()).raw, x.raw);
        expect_eq(x.add(Fixed16.zero()).raw, x.raw);
    });
}
