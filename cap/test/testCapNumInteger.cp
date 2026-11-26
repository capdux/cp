// cap/test/testCapNumInteger.cp
// Comprehensive unit tests for @cap/num-integer (Rust num-integer parity)

import { test, expect_eq } from "cap:test";
import {
    divFloor,
    modFloor,
    divCeil,
    divEuclid,
    remEuclid,
    gcdU64,
    lcmU64,
    gcdI64,
    lcmI64,
    extendedGcd,
    modInverse,
    isqrtU64,
    icbrtU64,
    binomialU64,
    isPrimeU64,
    averageFloor,
} from "../../packages/@cap/num-integer/src/index.cp";

fn main() {
    test("1. @cap/num-integer — floor, ceil & Euclidean division across all sign combinations", () => {
        expect_eq(divFloor(7, 3), 2 as i64);
        expect_eq(modFloor(7, 3), 1 as i64);
        expect_eq(divFloor(-7, 3), -3 as i64);
        expect_eq(modFloor(-7, 3), 2 as i64);
        expect_eq(divFloor(7, -3), -3 as i64);
        expect_eq(modFloor(7, -3), -2 as i64);
        expect_eq(divFloor(-7, -3), 2 as i64);
        expect_eq(modFloor(-7, -3), -1 as i64);

        expect_eq(divCeil(7, 3), 3 as i64);
        expect_eq(divCeil(-7, 3), -2 as i64);
        expect_eq(divCeil(6, 3), 2 as i64);
        expect_eq(divCeil(-7, -3), 3 as i64);

        expect_eq(divEuclid(-7, 3), -3 as i64);
        expect_eq(remEuclid(-7, 3), 2 as i64);
        expect_eq(divEuclid(-7, -3), 3 as i64);
        expect_eq(remEuclid(-7, -3), 2 as i64);
    });

    test("2. @cap/num-integer — GCD, LCM, Extended GCD (Bezout) & modInverse", () => {
        expect_eq(gcdU64(48 as u64, 18 as u64), 6 as u64);
        expect_eq(gcdU64(0 as u64, 15 as u64), 15 as u64);
        expect_eq(lcmU64(12 as u64, 18 as u64), 36 as u64);
        expect_eq(lcmU64(0 as u64, 18 as u64), 0 as u64);

        expect_eq(gcdI64(-48, 18), 6 as i64);
        expect_eq(lcmI64(-12, 18), 36 as i64);

        const eg = extendedGcd(240, 46);
        expect_eq(eg.gcd, 2 as i64);
        expect_eq(240 * eg.x + 46 * eg.y, eg.gcd);

        expect_eq(modInverse(3, 11).?, 4 as i64);
        expect_eq(modInverse(10, 17).?, 12 as i64);
        expect_eq(modInverse(6, 9) == null, true);
        expect_eq(modInverse(5, 0) == null, true);
    });

    test("3. @cap/num-integer — exact integer square root across full u64 range", () => {
        expect_eq(isqrtU64(0 as u64), 0 as u64);
        expect_eq(isqrtU64(1 as u64), 1 as u64);
        expect_eq(isqrtU64(15 as u64), 3 as u64);
        expect_eq(isqrtU64(16 as u64), 4 as u64);
        expect_eq(isqrtU64(1000000 as u64), 1000 as u64);
        // u64::MAX -> 4294967295 (2^32 - 1) without overflow
        expect_eq(isqrtU64(0xFFFFFFFFFFFFFFFF as u64), 4294967295 as u64);
    });

    test("4. @cap/num-integer — exact integer cube root across full u64 range", () => {
        expect_eq(icbrtU64(0 as u64), 0 as u64);
        expect_eq(icbrtU64(1 as u64), 1 as u64);
        expect_eq(icbrtU64(26 as u64), 2 as u64);
        expect_eq(icbrtU64(27 as u64), 3 as u64);
        expect_eq(icbrtU64(1000000000 as u64), 1000 as u64);
        expect_eq(icbrtU64(0xFFFFFFFFFFFFFFFF as u64), 2642245 as u64);
    });

    test("5. @cap/num-integer — binomialU64 & isPrimeU64", () => {
        expect_eq(binomialU64(5 as u64, 0 as u64), 1 as u64);
        expect_eq(binomialU64(5 as u64, 2 as u64), 10 as u64);
        expect_eq(binomialU64(10 as u64, 3 as u64), 120 as u64);
        expect_eq(binomialU64(52 as u64, 5 as u64), 2598960 as u64);
        expect_eq(binomialU64(5 as u64, 6 as u64), 0 as u64);

        expect_eq(isPrimeU64(0 as u64), false);
        expect_eq(isPrimeU64(1 as u64), false);
        expect_eq(isPrimeU64(2 as u64), true);
        expect_eq(isPrimeU64(3 as u64), true);
        expect_eq(isPrimeU64(4 as u64), false);
        expect_eq(isPrimeU64(97 as u64), true);
        expect_eq(isPrimeU64(1000000007 as u64), true);
        expect_eq(isPrimeU64(1000000009 as u64), true);
        expect_eq(isPrimeU64(1000000005 as u64), false);
    });

    test("6. @cap/num-integer — overflow-free averageFloor", () => {
        expect_eq(averageFloor(3, 4), 3 as i64);
        expect_eq(averageFloor(-3, -4), -4 as i64);
        expect_eq(averageFloor(-5, 2), -2 as i64);
        const max64: i64 = 9223372036854775807;
        expect_eq(averageFloor(max64, max64), max64);
        expect_eq(averageFloor(max64, max64 - 2), max64 - 1);
    });
}
