// cap/test/testCapNumComplex.cp
// Comprehensive unit tests for @cap/num-complex (Rust num-complex parity)

import { test, expect_eq } from "cap:test";
import {
    Complex64,
    formatComplex,
} from "../../packages/@cap/num-complex/src/index.cp";

fn approxF64(a: f64, b: f64, eps: f64) -> bool {
    const d = if (a > b) { a - b } else { b - a };
    return d <= eps;
}

fn main() {
    test("1. @cap/num-complex — basic arithmetic (add, sub, mul, div, neg, scale, inv)", () => {
        const a = Complex64.new(3.0, 4.0);
        const b = Complex64.new(1.0, -2.0);

        expect_eq(a.add(b).approxEq(Complex64.new(4.0, 2.0), 0.000001), true);
        expect_eq(a.sub(b).approxEq(Complex64.new(2.0, 6.0), 0.000001), true);
        // (3+4i)(1-2i) = 11 - 2i
        expect_eq(a.mul(b).approxEq(Complex64.new(11.0, -2.0), 0.000001), true);
        // (11-2i) / (1-2i) = 3+4i
        expect_eq(a.mul(b).div(b).approxEq(a, 0.000001), true);
        expect_eq(a.neg().approxEq(Complex64.new(-3.0, -4.0), 0.000001), true);
        expect_eq(a.scale(2.0).approxEq(Complex64.new(6.0, 8.0), 0.000001), true);
        expect_eq(a.mul(a.inv()).approxEq(Complex64.one(), 0.000001), true);
    });

    test("2. @cap/num-complex — conjugate, normSqr, norm & arg", () => {
        const z = Complex64.new(3.0, 4.0);
        expect_eq(z.conj().approxEq(Complex64.new(3.0, -4.0), 0.000001), true);
        expect_eq(approxF64(z.normSqr(), 25.0, 0.000001), true);
        expect_eq(approxF64(z.norm(), 5.0, 0.000001), true);
        // z * conj(z) = |z|^2
        expect_eq(z.mul(z.conj()).approxEq(Complex64.new(25.0, 0.0), 0.000001), true);
        // arg(i) = pi/2
        expect_eq(approxF64(Complex64.i().arg(), 1.5707963267948966, 0.000001), true);
    });

    test("3. @cap/num-complex — i^2 = -1 & integer powers powi (positive, zero, negative)", () => {
        const imag = Complex64.i();
        expect_eq(imag.mul(imag).approxEq(Complex64.new(-1.0, 0.0), 0.000001), true);
        expect_eq(imag.powi(0).approxEq(Complex64.one(), 0.000001), true);
        expect_eq(imag.powi(2).approxEq(Complex64.new(-1.0, 0.0), 0.000001), true);
        expect_eq(imag.powi(3).approxEq(Complex64.new(0.0, -1.0), 0.000001), true);
        expect_eq(imag.powi(4).approxEq(Complex64.one(), 0.000001), true);
        // (1+i)^-2 = 1/(2i) = -0.5i
        const z = Complex64.new(1.0, 1.0);
        expect_eq(z.powi(-2).approxEq(Complex64.new(0.0, -0.5), 0.000001), true);
    });

    test("4. @cap/num-complex — polar coordinate round-trip", () => {
        const pi: f64 = 3.14159265358979323846;
        const z = Complex64.fromPolar(2.0, pi / 3.0); // 2*(0.5 + i*sqrt(3)/2) = 1 + 1.7320508i
        expect_eq(z.approxEq(Complex64.new(1.0, 1.7320508075688772), 0.000001), true);
        expect_eq(approxF64(z.norm(), 2.0, 0.000001), true);
        expect_eq(approxF64(z.arg(), pi / 3.0, 0.000001), true);
    });

    test("5. @cap/num-complex — Euler's identity e^(i*pi) + 1 = 0 & principal sqrt", () => {
        const pi: f64 = 3.14159265358979323846;
        const euler: Complex64 = Complex64.new(0.0, pi).exp().add(Complex64.one());
        expect_eq(euler.approxEq(Complex64.zero(), 0.000001), true);

        // sqrt(-1) = i
        const s_neg1: Complex64 = Complex64.new(-1.0, 0.0).sqrt();
        expect_eq(s_neg1.approxEq(Complex64.i(), 0.000001), true);

        // sqrt(3 + 4i) = 2 + i
        const s_34: Complex64 = Complex64.new(3.0, 4.0).sqrt();
        expect_eq(s_34.approxEq(Complex64.new(2.0, 1.0), 0.000001), true);
        expect_eq(s_34.mul(s_34).approxEq(Complex64.new(3.0, 4.0), 0.000001), true);
    });

    test("6. @cap/num-complex — formatComplex zero-allocation output", () => {
        let buf: [64]u8 = undefined;
        expect_eq(formatComplex(Complex64.new(3.0, 4.0), buf[0..64]), "3+4i");
        expect_eq(formatComplex(Complex64.new(3.0, -4.0), buf[0..64]), "3-4i");
        expect_eq(formatComplex(Complex64.new(-1.5, 2.25), buf[0..64]), "-1.5+2.25i");
        expect_eq(formatComplex(Complex64.zero(), buf[0..64]), "0+0i");
    });
}
