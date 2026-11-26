import { test, expect_eq } from "cap:test";
import {
    PI,
    Complex,
    zero,
    one,
    iUnit,
    fromPolar,
    add,
    sub,
    mul,
    div,
    conj,
    neg,
    scale,
    normSq,
    norm,
    arg,
    exp,
    powInt,
    sqrt,
    approxEq,
} from "../../packages/@cap/complex-num/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) {
        d = 0.0 - d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/complex-num — add, sub, mul, div & i*i == -1", () => {
        let eps: f64 = 0.000001;
        let a: Complex = Complex.new(3.0, 2.0);
        let b: Complex = Complex.new(1.0, -4.0);

        expect_eq(approxEq(add(a, b), Complex.new(4.0, -2.0), eps), true);
        expect_eq(approxEq(sub(a, b), Complex.new(2.0, 6.0), eps), true);
        expect_eq(approxEq(mul(a, b), Complex.new(11.0, -10.0), eps), true);
        expect_eq(approxEq(div(mul(a, b), b), a, eps), true);

        let ii: Complex = mul(iUnit(), iUnit());
        expect_eq(approxEq(ii, Complex.new(-1.0, 0.0), eps), true);
        expect_eq(approxEq(neg(a), Complex.new(-3.0, -2.0), eps), true);
        expect_eq(approxEq(scale(a, 2.0), Complex.new(6.0, 4.0), eps), true);
    });

    test("2. @cap/complex-num — conj, normSq & norm (3+4i -> 5)", () => {
        let eps: f64 = 0.000001;
        let z: Complex = Complex.new(3.0, 4.0);
        expect_eq(approxEq(conj(z), Complex.new(3.0, -4.0), eps), true);
        expect_eq(near(normSq(z), 25.0, eps), true);
        expect_eq(near(norm(z), 5.0, eps), true);
        expect_eq(approxEq(mul(z, conj(z)), Complex.new(25.0, 0.0), eps), true);
    });

    test("3. @cap/complex-num — polar round-trip (fromPolar, norm & arg)", () => {
        let eps: f64 = 0.000001;
        let z1: Complex = fromPolar(2.0, PI / 3.0);
        expect_eq(near(norm(z1), 2.0, eps), true);
        expect_eq(near(arg(z1), PI / 3.0, eps), true);
        expect_eq(approxEq(fromPolar(norm(z1), arg(z1)), z1, eps), true);

        let z2: Complex = Complex.new(-1.0, -1.0);
        let rt: Complex = fromPolar(norm(z2), arg(z2));
        expect_eq(approxEq(rt, z2, eps), true);
    });

    test("4. @cap/complex-num — Euler's identity exp(i * PI) == -1", () => {
        let eps: f64 = 0.000001;
        let e_ipi: Complex = exp(Complex.new(0.0, PI));
        expect_eq(approxEq(e_ipi, Complex.new(-1.0, 0.0), eps), true);

        let e_half: Complex = exp(Complex.new(0.0, PI * 0.5));
        expect_eq(approxEq(e_half, iUnit(), eps), true);

        let e_zero: Complex = exp(zero());
        expect_eq(approxEq(e_zero, one(), eps), true);
    });

    test("5. @cap/complex-num — powInt positive, zero & negative exponents", () => {
        let eps: f64 = 0.000001;
        let z: Complex = Complex.new(1.0, 1.0);
        expect_eq(approxEq(powInt(z, 0), one(), eps), true);
        expect_eq(approxEq(powInt(z, 2), Complex.new(0.0, 2.0), eps), true);
        expect_eq(approxEq(powInt(z, 4), Complex.new(-4.0, 0.0), eps), true);
        expect_eq(approxEq(powInt(z, -2), Complex.new(0.0, -0.5), eps), true);
        expect_eq(approxEq(mul(powInt(z, 3), powInt(z, -3)), one(), eps), true);
    });

    test("6. @cap/complex-num — sqrt(-1) == i, sqrt(3+4i) == 2+i & div-by-zero safety", () => {
        let eps: f64 = 0.000001;
        let s1: Complex = sqrt(Complex.new(-1.0, 0.0));
        expect_eq(approxEq(s1, iUnit(), eps), true);

        let s2: Complex = sqrt(Complex.new(3.0, 4.0));
        expect_eq(approxEq(s2, Complex.new(2.0, 1.0), eps), true);
        expect_eq(approxEq(mul(s2, s2), Complex.new(3.0, 4.0), eps), true);

        let s0: Complex = sqrt(zero());
        expect_eq(approxEq(s0, zero(), eps), true);

        let safe_div: Complex = div(Complex.new(5.0, 7.0), zero());
        expect_eq(approxEq(safe_div, zero(), eps), true);
    });
}
