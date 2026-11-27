// cap/test/testCapRyuFloat.cp
// Comprehensive unit tests for @cap/ryu-float (Rust ryu v1 parity)

import { test, expect_eq } from "cap:test";
import {
    formatF64,
    formatF64Finite,
    formatF32,
    formatFixedF64,
    formatExpF64,
    isFiniteF64,
    isNanF64,
    isInfF64
} from "../../packages/ryu/src/index.cp";

fn ryuTestSliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ryu-float — Standard integers & decimals (0.0, -0.0, 1.0, -42.5, 3.14159, pi)", () => {
        let buf: [64]u8 = undefined;

        let n0 = formatF64(0.0, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n0], "0.0"), true);

        let neg_zero: f64 = -0.0;
        let n1 = formatF64(neg_zero, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n1], "-0.0"), true);

        let n2 = formatF64(1.0, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n2], "1.0"), true);

        let n3 = formatF64(-42.5, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n3], "-42.5"), true);

        let n4 = formatF64(3.14159, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n4], "3.14159"), true);

        let n5 = formatF64(3.141592653589793, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n5], "3.141592653589793"), true);

        let n6 = formatF64(0.1, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n6], "0.1"), true);
    });

    test("2. @cap/ryu-float — Special values (NaN, inf, -inf) & formatF64Finite", () => {
        let buf: [64]u8 = undefined;
        let nan_v: f64 = std.math.nan(f64);
        let inf_v: f64 = std.math.inf(f64);
        let ninf_v: f64 = -std.math.inf(f64);

        expect_eq(isNanF64(nan_v), true);
        expect_eq(isFiniteF64(nan_v), false);
        expect_eq(isInfF64(inf_v), true);
        expect_eq(isInfF64(ninf_v), true);
        expect_eq(isFiniteF64(inf_v), false);
        expect_eq(isFiniteF64(42.0), true);

        let n1 = formatF64(nan_v, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n1], "NaN"), true);

        let n2 = formatF64(inf_v, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n2], "inf"), true);

        let n3 = formatF64(ninf_v, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n3], "-inf"), true);

        // formatF64Finite rejects NaN/Inf with 0, accepts finite
        expect_eq(formatF64Finite(nan_v, buf[0..64]), 0 as usize);
        expect_eq(formatF64Finite(inf_v, buf[0..64]), 0 as usize);
        expect_eq(formatF64Finite(ninf_v, buf[0..64]), 0 as usize);
        let nf = formatF64Finite(123.75, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..nf], "123.75"), true);
    });

    test("3. @cap/ryu-float — formatF32 single-precision shortest formatting", () => {
        let buf: [64]u8 = undefined;

        let f1: f32 = 3.14;
        let n1 = formatF32(f1, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n1], "3.14"), true);

        let f2: f32 = 0.1;
        let n2 = formatF32(f2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n2], "0.1"), true);

        let f3: f32 = -100.0;
        let n3 = formatF32(f3, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n3], "-100.0"), true);

        let f4: f32 = 0.0;
        let n4 = formatF32(f4, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n4], "0.0"), true);
    });

    test("4. @cap/ryu-float — formatFixedF64 0/2/4 decimal places & carry rounding (9.995 -> 10.00)", () => {
        let buf: [64]u8 = undefined;

        let n0 = formatFixedF64(3.14159, 0, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n0], "3"), true);

        let n1 = formatFixedF64(3.14159, 2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n1], "3.14"), true);

        let n2 = formatFixedF64(3.14159, 4, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n2], "3.1416"), true);

        // Carry across decimal point: 9.995 -> "10.00"
        let n3 = formatFixedF64(9.995, 2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n3], "10.00"), true);

        // Negative rounding & zero padding
        let n4 = formatFixedF64(-0.5, 2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n4], "-0.50"), true);

        let n5 = formatFixedF64(0.0, 2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n5], "0.00"), true);
    });

    test("5. @cap/ryu-float — formatExpF64 positive & negative scientific exponents", () => {
        let buf: [64]u8 = undefined;

        let n1 = formatExpF64(12345.0, 2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n1], "1.23e4"), true);

        let n2 = formatExpF64(0.0015, 1, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n2], "1.5e-3"), true);

        let n3 = formatExpF64(-9.995, 2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n3], "-1.00e1"), true);

        let n4 = formatExpF64(0.0, 2, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n4], "0.00e0"), true);
    });

    test("6. @cap/ryu-float — Extreme magnitudes (1e20, 1e-7, 1e308, 1e-308) & small buffer safety", () => {
        let buf: [64]u8 = undefined;

        let n1 = formatF64(1.0e20, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n1], "1e20"), true);

        let n2 = formatF64(1.0e-7, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n2], "1e-7"), true);

        let n3 = formatF64(1.0e308, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n3], "1e308"), true);

        let n4 = formatF64(1.0e-308, buf[0..64]);
        expect_eq(ryuTestSliceEq(buf[0..n4], "1e-308"), true);

        // Small buffer returns 0 without overflowing
        let tiny: [4]u8 = [0xAA, 0xAA, 0xAA, 0xAA];
        expect_eq(formatF64(3.14159, tiny[0..3]), 0 as usize);
        expect_eq(tiny[3], 0xAA as u8);
        expect_eq(formatF64(0.0, tiny[0..2]), 0 as usize);
        expect_eq(formatFixedF64(123.45, 2, tiny[0..4]), 0 as usize);
        expect_eq(formatExpF64(12345.0, 2, tiny[0..4]), 0 as usize);
    });
}
