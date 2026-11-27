import { test, expect_eq } from "cap:test";
import {
    linearScale,
    powScale,
    sqrtScale,
    logScale,
    quantizeScale,
} from "../../packages/d3-scale/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    if (d < 0.0) return (0.0 - d) <= eps;
    return d <= eps;
}

fn main() {
    test("1. @cap/d3-scale — LinearScale apply, invert & clamp", () => {
        let s = linearScale(0.0, 100.0, 0.0, 500.0);
        expect_eq(approxEq(s.apply(25.0), 125.0, 1.0e-9), true);
        expect_eq(approxEq(s.apply(150.0), 750.0, 1.0e-9), true);
        expect_eq(approxEq(s.invert(250.0), 50.0, 1.0e-9), true);

        s.clamp(true);
        expect_eq(approxEq(s.apply(-20.0), 0.0, 1.0e-9), true);
        expect_eq(approxEq(s.apply(150.0), 500.0, 1.0e-9), true);
        expect_eq(approxEq(s.invert(999.0), 100.0, 1.0e-9), true);
    });

    test("2. @cap/d3-scale — LinearScale ticksCount & ticks generation", () => {
        const s = linearScale(0.0, 10.0, 0.0, 100.0);
        const cnt = s.ticksCount(5);
        expect_eq(cnt, 6 as usize);

        let buf: [16]f64 = undefined;
        const n = s.ticks(5, buf[0..16]);
        expect_eq(n, 6 as usize);
        expect_eq(approxEq(buf[0], 0.0, 1.0e-9), true);
        expect_eq(approxEq(buf[1], 2.0, 1.0e-9), true);
        expect_eq(approxEq(buf[5], 10.0, 1.0e-9), true);
    });

    test("3. @cap/d3-scale — PowScale & SqrtScale with negative domain", () => {
        const p = powScale(-10.0, 10.0, -100.0, 100.0, 2.0);
        expect_eq(approxEq(p.apply(5.0), 25.0, 1.0e-9), true);
        expect_eq(approxEq(p.apply(-5.0), -25.0, 1.0e-9), true);
        expect_eq(approxEq(p.invert(-25.0), -5.0, 1.0e-9), true);

        const sq = sqrtScale(0.0, 100.0, 0.0, 10.0);
        expect_eq(approxEq(sq.apply(25.0), 5.0, 1.0e-9), true);
        expect_eq(approxEq(sq.invert(5.0), 25.0, 1.0e-9), true);
    });

    test("4. @cap/d3-scale — LogScale base 10 & custom base 2", () => {
        const l10 = logScale(1.0, 1000.0, 0.0, 300.0, 10.0);
        expect_eq(approxEq(l10.apply(10.0), 100.0, 1.0e-9), true);
        expect_eq(approxEq(l10.apply(100.0), 200.0, 1.0e-9), true);
        expect_eq(approxEq(l10.invert(200.0), 100.0, 1.0e-9), true);

        const l2 = logScale(1.0, 16.0, 0.0, 40.0, 2.0);
        expect_eq(approxEq(l2.apply(4.0), 20.0, 1.0e-9), true);
        expect_eq(approxEq(l2.invert(30.0), 8.0, 1.0e-9), true);
    });

    test("5. @cap/d3-scale — LogScale negative domain & clamping", () => {
        let ln = logScale(-100.0, -1.0, 0.0, 200.0, 10.0);
        expect_eq(approxEq(ln.apply(-10.0), 100.0, 1.0e-9), true);
        expect_eq(approxEq(ln.invert(100.0), -10.0, 1.0e-9), true);

        ln.clamp(true);
        expect_eq(approxEq(ln.apply(-1000.0), 0.0, 1.0e-9), true);
    });

    test("6. @cap/d3-scale — QuantizeScale bucket mapping & invertExtent", () => {
        const q = quantizeScale(0.0, 100.0, 4);
        const palette: [4]f64 = [10.0, 20.0, 30.0, 40.0];

        expect_eq(q.applyIndex(-5.0), 0 as usize);
        expect_eq(q.applyIndex(0.0), 0 as usize);
        expect_eq(q.applyIndex(24.9), 0 as usize);
        expect_eq(q.applyIndex(25.0), 1 as usize);
        expect_eq(q.applyIndex(50.0), 2 as usize);
        expect_eq(q.applyIndex(99.9), 3 as usize);
        expect_eq(q.applyIndex(150.0), 3 as usize);

        expect_eq(approxEq(q.applyValue(60.0, palette[0..4]), 30.0, 1.0e-9), true);
        expect_eq(approxEq(q.invertExtentMin(1), 25.0, 1.0e-9), true);
        expect_eq(approxEq(q.invertExtentMax(1), 50.0, 1.0e-9), true);
    });
}
