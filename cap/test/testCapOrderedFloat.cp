import { test, expect_eq } from "cap:test";
import {
    OrderedF64, NotNanF64, cmpOrderedF64, eqOrderedF64, hashOrderedF64,
    isNanF64, isInfiniteF64, isFiniteF64, sortOrderedF64
} from "../../packages/@cap/ordered-float/src/index.cp";

fn main() {
    test("1. @cap/ordered-float — Total order: -Inf < -1.0 < 0.0 < 1.0 < +Inf < NaN", () => {
        let zero: f64 = 0.0;
        let pos_inf: f64 = 1.0 / zero;
        let neg_inf: f64 = -1.0 / zero;
        let nan_val: f64 = zero / zero;
        expect_eq(isNanF64(nan_val), true);
        expect_eq(isInfiniteF64(pos_inf), true);
        expect_eq(isFiniteF64(42.5), true);
        expect_eq(cmpOrderedF64(neg_inf, -1.0), -1);
        expect_eq(cmpOrderedF64(-1.0, 0.0), -1);
        expect_eq(cmpOrderedF64(1.0, pos_inf), -1);
        expect_eq(cmpOrderedF64(pos_inf, nan_val), -1);
        expect_eq(cmpOrderedF64(nan_val, nan_val), 0);
        expect_eq(eqOrderedF64(nan_val, nan_val), true);
    });

    test("2. @cap/ordered-float — -0.0 and +0.0 compare equal and share identical hash", () => {
        let p0 = OrderedF64.new(0.0);
        let n0 = OrderedF64.new(-0.0);
        expect_eq(p0.eq(n0), true);
        expect_eq(p0.cmp(n0), 0);
        expect_eq(p0.hash(), n0.hash());
    });

    test("3. @cap/ordered-float — All NaNs produce identical canonical hash", () => {
        let zero: f64 = 0.0;
        let nan1 = zero / zero;
        let nan2 = -(zero / zero);
        expect_eq(hashOrderedF64(nan1), hashOrderedF64(nan2));
    });

    test("4. @cap/ordered-float — OrderedF64 min, max & clamp", () => {
        let a = OrderedF64.new(3.5);
        let b = OrderedF64.new(7.25);
        let lo = OrderedF64.new(0.0);
        let hi = OrderedF64.new(5.0);
        expect_eq(a.min(b).intoInner(), 3.5);
        expect_eq(a.max(b).intoInner(), 7.25);
        expect_eq(b.clamp(lo, hi).intoInner(), 5.0);
        expect_eq(OrderedF64.new(-2.0).clamp(lo, hi).intoInner(), 0.0);
    });

    test("5. @cap/ordered-float — NotNanF64 rejects NaN and accepts finite/Inf", () => {
        let zero: f64 = 0.0;
        let nan_val = zero / zero;
        expect_eq(NotNanF64.new(nan_val) == null, true);
        let ok = NotNanF64.new(2.5) ?? NotNanF64 { val: 0.0 };
        expect_eq(ok.intoInner(), 2.5);
        expect_eq(ok.cmp(NotNanF64 { val: 3.0 }), -1);
    });

    test("6. @cap/ordered-float — sortOrderedF64 sorts finite, +/-Inf and NaN deterministically", () => {
        let zero: f64 = 0.0;
        let arr: [5]f64 = [zero / zero, 2.0, -1.0 / zero, -5.0, 1.0 / zero];
        sortOrderedF64(arr[0..5]);
        expect_eq(isInfiniteF64(arr[0]) && arr[0] < 0.0, true);
        expect_eq(arr[1], -5.0);
        expect_eq(arr[2], 2.0);
        expect_eq(isInfiniteF64(arr[3]) && arr[3] > 0.0, true);
        expect_eq(isNanF64(arr[4]), true);
    });
}
