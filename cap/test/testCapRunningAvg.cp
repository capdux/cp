import { test, expect_eq } from "cap:test";
import { Ema, Sma, smaSlice, wmaSlice } from "../../packages/moving-averages/src/index.cp";

fn approx(a: f64, b: f64): bool {
    let d = a - b;
    if (d < 0.0) { d = -d; }
    return d < 0.000001;
}

fn main() {
    test("EMA basic convergence & alpha", () => {
        let e = Ema.new(0.5);
        expect_eq(approx(e.update(10.0), 10.0), true);
        expect_eq(approx(e.update(20.0), 15.0), true);
        expect_eq(approx(e.update(20.0), 17.5), true);
    });

    test("EMA fromPeriod & reset", () => {
        let e = Ema.fromPeriod(3);
        expect_eq(approx(e.alpha, 0.5), true);
        * = e.update(100.0);
        e.reset();
        expect_eq(e.initialized, false);
        expect_eq(approx(e.update(4.0), 4.0), true);
    });

    test("SMA ring buffer sliding window", () => {
        let buf: [3]f64 = [0.0, 0.0, 0.0];
        let s = Sma.new();
        expect_eq(approx(s.update(buf[0..3], 10.0), 10.0), true);
        expect_eq(approx(s.update(buf[0..3], 20.0), 15.0), true);
        expect_eq(approx(s.update(buf[0..3], 30.0), 20.0), true);
        expect_eq(approx(s.update(buf[0..3], 40.0), 30.0), true);
        expect_eq(approx(s.average(), 30.0), true);
    });

    test("SMA reset & empty window safety", () => {
        let buf: [2]f64 = [0.0, 0.0];
        let s = Sma.new();
        * = s.update(buf[0..2], 5.0);
        s.reset();
        expect_eq(approx(s.average(), 0.0), true);
        expect_eq(approx(s.update(buf[0..0], 10.0), 0.0), true);
    });

    test("smaSlice & wmaSlice calculation", () => {
        let xs: [4]f64 = [10.0, 20.0, 30.0, 40.0];
        expect_eq(approx(smaSlice(xs[0..4]), 25.0), true);
        // WMA = (10*1 + 20*2 + 30*3 + 40*4) / (1+2+3+4) = 300 / 10 = 30
        expect_eq(approx(wmaSlice(xs[0..4]), 30.0), true);
    });

    test("empty slice safety", () => {
        let xs: [1]f64 = [1.0];
        expect_eq(approx(smaSlice(xs[0..0]), 0.0), true);
        expect_eq(approx(wmaSlice(xs[0..0]), 0.0), true);
    });
}
