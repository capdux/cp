// cap/test/testCapOrderedFloat.cp
// Comprehensive unit tests for @cap/ordered-float (Rust ordered-float parity)

import { test, expect_eq } from "cap:test";
import {
    isNanF64,
    isInfF64,
    isFiniteF64,
    orderedCmpF64,
    orderedEqF64,
    orderedMinF64,
    orderedMaxF64,
    clampF64,
    NotNanF64,
    sortF64,
    isSortedF64,
    binarySearchF64,
} from "../../packages/@cap/ordered-float/src/index.cp";

const std = @import("std");

fn main() {
    test("1. @cap/ordered-float — float classification (NaN, Inf, finite)", () => {
        const nan: f64 = std.math.nan(f64);
        const pos_inf: f64 = std.math.inf(f64);
        const neg_inf: f64 = -std.math.inf(f64);

        expect_eq(isNanF64(nan), true);
        expect_eq(isInfF64(nan), false);
        expect_eq(isFiniteF64(nan), false);

        expect_eq(isInfF64(pos_inf), true);
        expect_eq(isInfF64(neg_inf), true);
        expect_eq(isFiniteF64(pos_inf), false);

        expect_eq(isFiniteF64(42.5), true);
        expect_eq(isFiniteF64(0.0), true);
    });

    test("2. @cap/ordered-float — total order: -Inf < finite < +Inf < NaN & -0.0 == +0.0", () => {
        const nan1: f64 = std.math.nan(f64);
        const nan2: f64 = std.math.nan(f64);
        const pos_inf: f64 = std.math.inf(f64);
        const neg_inf: f64 = -std.math.inf(f64);

        expect_eq(orderedCmpF64(neg_inf, -1000.0), -1);
        expect_eq(orderedCmpF64(-1000.0, 0.0), -1);
        expect_eq(orderedCmpF64(0.0, pos_inf), -1);
        expect_eq(orderedCmpF64(pos_inf, nan1), -1);
        expect_eq(orderedCmpF64(nan1, pos_inf), 1);

        expect_eq(orderedEqF64(nan1, nan2), true);
        expect_eq(orderedCmpF64(nan1, nan2), 0);
        expect_eq(orderedEqF64(-0.0, 0.0), true);
        expect_eq(orderedEqF64(pos_inf, pos_inf), true);
        expect_eq(orderedEqF64(neg_inf, pos_inf), false);
    });

    test("3. @cap/ordered-float — orderedMinF64, orderedMaxF64 & clampF64", () => {
        const nan: f64 = std.math.nan(f64);
        expect_eq(orderedMinF64(3.0, nan), 3.0);
        expect_eq(isNanF64(orderedMaxF64(3.0, nan)), true);

        expect_eq(clampF64(-5.0, 0.0, 10.0), 0.0);
        expect_eq(clampF64(5.0, 0.0, 10.0), 5.0);
        expect_eq(clampF64(20.0, 0.0, 10.0), 10.0);
        // NaN > hi -> clamped to hi
        expect_eq(clampF64(nan, 0.0, 10.0), 10.0);
    });

    test("4. @cap/ordered-float — NotNanF64 checked wrapper", () => {
        const nan: f64 = std.math.nan(f64);
        expect_eq(NotNanF64.new(nan) == null, true);

        const a = NotNanF64.new(-2.5).?;
        const b = NotNanF64.new(3.5).?;
        expect_eq(a.value(), -2.5);
        expect_eq(a.cmp(b), -1);
        expect_eq(b.cmp(a), 1);
        expect_eq(a.cmp(a), 0);
    });

    test("5. @cap/ordered-float — in-place sortF64 with NaN & +-Inf (small + >16 heapsort)", () => {
        const nan: f64 = std.math.nan(f64);
        const pos_inf: f64 = std.math.inf(f64);
        const neg_inf: f64 = -std.math.inf(f64);

        let arr: [6]f64 = [nan, 3.0, pos_inf, -1.5, neg_inf, nan];
        expect_eq(isSortedF64(arr[0..6]), false);
        sortF64(arr[0..6]);
        expect_eq(isSortedF64(arr[0..6]), true);
        expect_eq(arr[0], neg_inf);
        expect_eq(arr[1], -1.5);
        expect_eq(arr[2], 3.0);
        expect_eq(arr[3], pos_inf);
        expect_eq(isNanF64(arr[4]), true);
        expect_eq(isNanF64(arr[5]), true);

        // 18-element array to exercise heapsort branch (>16)
        let big: [18]f64 = [
            9.0, nan, 1.0, neg_inf, 8.0, 2.0,
            pos_inf, 7.0, 3.0, 6.0, 4.0, 5.0,
            0.0, -2.0, -1.0, 10.0, 11.0, nan,
        ];
        sortF64(big[0..18]);
        expect_eq(isSortedF64(big[0..18]), true);
        expect_eq(big[0], neg_inf);
        expect_eq(big[15], pos_inf);
        expect_eq(isNanF64(big[16]), true);
        expect_eq(isNanF64(big[17]), true);
    });

    test("6. @cap/ordered-float — binarySearchF64 including Inf & NaN", () => {
        const nan: f64 = std.math.nan(f64);
        const pos_inf: f64 = std.math.inf(f64);
        const neg_inf: f64 = -std.math.inf(f64);

        let arr: [6]f64 = [neg_inf, -2.0, 0.0, 4.5, pos_inf, nan];
        expect_eq(binarySearchF64(arr[0..6], neg_inf).?, 0 as usize);
        expect_eq(binarySearchF64(arr[0..6], -0.0).?, 2 as usize);
        expect_eq(binarySearchF64(arr[0..6], 4.5).?, 3 as usize);
        expect_eq(binarySearchF64(arr[0..6], pos_inf).?, 4 as usize);
        expect_eq(binarySearchF64(arr[0..6], nan).?, 5 as usize);
        expect_eq(binarySearchF64(arr[0..6], 99.0) == null, true);
    });
}
