// cap/test/testCapFixedQ16.cp
// Comprehensive unit tests for @cap/fixed (Fixed32 Q32.32 deterministic fixed-point)

import { test, expect_eq } from "cap:test";
import { Fixed32 } from "../../packages/fixed/src/index.cp";

fn fixApprox(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) { d = -d; }
    return d <= eps;
}

fn main() {
    test("1. @cap/fixed — Fixed32 arithmetic, floor/ceil/round & clamp", () => {
        const a = Fixed32.fromRatio(3, 2); // 1.5
        const b = Fixed32.fromInt(2);
        expect_eq(a.toInt(), 1 as i32);
        expect_eq(a.floor(), 1 as i32);
        expect_eq(a.ceil(), 2 as i32);
        expect_eq(a.round(), 2 as i32);

        const prod = a.mul(b);
        expect_eq(prod.toInt(), 3 as i32);
        const quot = prod.div(a);
        expect_eq(fixApprox(quot.toF64(), 2.0, 0.000001), true);

        const clamped: Fixed32 = Fixed32.fromInt(10).clamp(Fixed32.fromInt(0), Fixed32.fromInt(5));
        expect_eq(clamped.toInt(), 5 as i32);
    });

    test("2. @cap/fixed — Fixed32 sqrt & lerp", () => {
        const s: Fixed32 = Fixed32.fromInt(9).sqrt();
        expect_eq(fixApprox(s.toF64(), 3.0, 0.00001), true);

        const mid: Fixed32 = Fixed32.fromInt(0).lerp(Fixed32.fromInt(10), Fixed32.fromRatio(1, 2));
        expect_eq(fixApprox(mid.toF64(), 5.0, 0.000001), true);
    });
}
