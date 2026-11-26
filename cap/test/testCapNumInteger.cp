import { test, expect_eq } from "cap:test";
import {
    ExtGcdResult,
    gcdU64,
    lcmU64,
    extendedGcd,
    divFloor,
    modFloor,
    divCeil,
    isqrtU64,
    icbrtU64,
    binomialU64,
    isEven,
    isOdd,
    nextMultipleOfU64,
    prevMultipleOfU64,
} from "../../packages/@cap/num-integer/src/index.cp";

fn main() {
    test("1. @cap/num-integer — gcdU64 & overflow-safe lcmU64", () => {
        expect_eq(gcdU64(0, 0), 0 as u64);
        expect_eq(gcdU64(0, 42), 42 as u64);
        expect_eq(gcdU64(48, 18), 6 as u64);
        expect_eq(gcdU64(1071, 462), 21 as u64);
        expect_eq(gcdU64(17, 31), 1 as u64);

        expect_eq(lcmU64(0, 12), 0 as u64);
        expect_eq(lcmU64(12, 18), 36 as u64);
        // Large inputs that would overflow if multiplied before dividing by gcd.
        const big_a: u64 = 1000000000000;
        const big_b: u64 = 1500000000000;
        expect_eq(lcmU64(big_a, big_b), 3000000000000 as u64);
    });

    test("2. @cap/num-integer — extendedGcd Bezout identity a*x + b*y == gcd", () => {
        const r1: ExtGcdResult = extendedGcd(240, 46);
        expect_eq(r1.gcd, 2 as i64);
        expect_eq(240 * r1.x + 46 * r1.y, r1.gcd);

        const r2: ExtGcdResult = extendedGcd(-48, 18);
        expect_eq(r2.gcd, 6 as i64);
        expect_eq((-48) * r2.x + 18 * r2.y, r2.gcd);

        const r3: ExtGcdResult = extendedGcd(-35, -15);
        expect_eq(r3.gcd, 5 as i64);
        expect_eq((-35) * r3.x + (-15) * r3.y, r3.gcd);

        const r4: ExtGcdResult = extendedGcd(17, 0);
        expect_eq(r4.gcd, 17 as i64);
        expect_eq(17 * r4.x + 0 * r4.y, r4.gcd);
    });

    test("3. @cap/num-integer — divFloor, modFloor & divCeil across all sign combinations", () => {
        // Positive / positive
        expect_eq(divFloor(7, 3), 2 as i64);
        expect_eq(modFloor(7, 3), 1 as i64);
        expect_eq(divCeil(7, 3), 3 as i64);
        expect_eq(divCeil(6, 3), 2 as i64);

        // Negative / positive
        expect_eq(divFloor(-7, 3), -3 as i64);
        expect_eq(modFloor(-7, 3), 2 as i64);
        expect_eq(divFloor(-7, 3) * 3 + modFloor(-7, 3), -7 as i64);
        expect_eq(divCeil(-7, 3), -2 as i64);

        // Positive / negative
        expect_eq(divFloor(7, -3), -3 as i64);
        expect_eq(modFloor(7, -3), -2 as i64);
        expect_eq(divFloor(7, -3) * (-3) + modFloor(7, -3), 7 as i64);
        expect_eq(divCeil(7, -3), -2 as i64);

        // Negative / negative
        expect_eq(divFloor(-7, -3), 2 as i64);
        expect_eq(modFloor(-7, -3), -1 as i64);
        expect_eq(divFloor(-7, -3) * (-3) + modFloor(-7, -3), -7 as i64);
        expect_eq(divCeil(-7, -3), 3 as i64);
    });

    test("4. @cap/num-integer — Exact isqrtU64 & icbrtU64 up to u64::MAX", () => {
        expect_eq(isqrtU64(0), 0 as u64);
        expect_eq(isqrtU64(1), 1 as u64);
        expect_eq(isqrtU64(15), 3 as u64);
        expect_eq(isqrtU64(16), 4 as u64);
        expect_eq(isqrtU64(1000000000000), 1000000 as u64);
        expect_eq(isqrtU64(~(0 as u64)), 4294967295 as u64);

        expect_eq(icbrtU64(0), 0 as u64);
        expect_eq(icbrtU64(1), 1 as u64);
        expect_eq(icbrtU64(26), 2 as u64);
        expect_eq(icbrtU64(27), 3 as u64);
        expect_eq(icbrtU64(1000000000), 1000 as u64);
        expect_eq(icbrtU64(~(0 as u64)), 2642245 as u64);
    });

    test("5. @cap/num-integer — Overflow-safe binomialU64 C(n, k)", () => {
        expect_eq(binomialU64(5, 0), 1 as u64);
        expect_eq(binomialU64(5, 5), 1 as u64);
        expect_eq(binomialU64(5, 6), 0 as u64);
        expect_eq(binomialU64(10, 3), 120 as u64);
        expect_eq(binomialU64(20, 10), 184756 as u64);
        expect_eq(binomialU64(60, 30), 118264581564861424 as u64);
    });

    test("6. @cap/num-integer — isEven, isOdd, nextMultipleOfU64 & prevMultipleOfU64", () => {
        expect_eq(isEven(0), true);
        expect_eq(isEven(-4), true);
        expect_eq(isEven(7), false);
        expect_eq(isOdd(-3), true);
        expect_eq(isOdd(9), true);
        expect_eq(isOdd(8), false);

        expect_eq(nextMultipleOfU64(0, 8), 0 as u64);
        expect_eq(nextMultipleOfU64(1, 8), 8 as u64);
        expect_eq(nextMultipleOfU64(16, 8), 16 as u64);
        expect_eq(nextMultipleOfU64(17, 8), 24 as u64);

        expect_eq(prevMultipleOfU64(0, 8), 0 as u64);
        expect_eq(prevMultipleOfU64(7, 8), 0 as u64);
        expect_eq(prevMultipleOfU64(16, 8), 16 as u64);
        expect_eq(prevMultipleOfU64(23, 8), 16 as u64);
    });
}
