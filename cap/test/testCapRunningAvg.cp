import { test, expect_eq } from "cap:test";
import {
    Ema, Sma, Cma, Wma,
    smaSlice, wmaSlice,
    ma, sma, ema, wma, dma, cma, smma
} from "../../packages/moving-averages/src/index.cp";

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

    test("ma/sma, ema, wma, dma, cma, smma series functions", () => {
        const data: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
        let out: [5]f64 = [0.0, 0.0, 0.0, 0.0, 0.0];

        expect_eq(ma(data[0..5], 3, out[0..5]), 5);
        expect_eq(out[0] != out[0], true);
        expect_eq(out[1] != out[1], true);
        expect_eq(approx(out[2], 2.0), true);
        expect_eq(approx(out[3], 3.0), true);
        expect_eq(approx(out[4], 4.0), true);

        expect_eq(sma(data[0..5], 2, out[0..5]), 5);
        expect_eq(approx(out[1], 1.5), true);
        expect_eq(approx(out[4], 4.5), true);

        expect_eq(ema(data[0..5], 3, out[0..5]), 5);
        // alpha = 2/(3+1) = 0.5: [1.0, 1.5, 2.25, 3.125, 4.0625]
        expect_eq(approx(out[0], 1.0), true);
        expect_eq(approx(out[1], 1.5), true);
        expect_eq(approx(out[2], 2.25), true);
        expect_eq(approx(out[3], 3.125), true);
        expect_eq(approx(out[4], 4.0625), true);

        expect_eq(dma(data[0..5], 0.5, out[0..5]), 5);
        expect_eq(approx(out[4], 4.0625), true);

        expect_eq(wma(data[0..5], 3, out[0..5]), 5);
        expect_eq(out[0] != out[0], true);
        // (1*1 + 2*2 + 3*3)/6 = 14/6 = 2.333333...
        expect_eq(approx(out[2], 14.0 / 6.0), true);
        // (3*1 + 4*2 + 5*3)/6 = 26/6 = 4.333333...
        expect_eq(approx(out[4], 26.0 / 6.0), true);

        expect_eq(cma(data[0..5], out[0..5]), 5);
        expect_eq(approx(out[0], 1.0), true);
        expect_eq(approx(out[2], 2.0), true);
        expect_eq(approx(out[4], 3.0), true);

        expect_eq(smma(data[0..5], 2, out[0..5]), 5);
        expect_eq(approx(out[0], 1.0), true);
        expect_eq(approx(out[1], 1.5), true);
    });

    test("Cma and Wma streaming structs", () => {
        let c = Cma.new();
        expect_eq(approx(c.update(10.0), 10.0), true);
        expect_eq(approx(c.update(20.0), 15.0), true);
        expect_eq(approx(c.update(30.0), 20.0), true);
        expect_eq(approx(c.average(), 20.0), true);
        c.reset();
        expect_eq(c.count, 0);

        let wbuf: [3]f64 = [0.0, 0.0, 0.0];
        let w = Wma.new();
        expect_eq(approx(w.update(wbuf[0..3], 1.0), 1.0), true);
        expect_eq(approx(w.update(wbuf[0..3], 2.0), 5.0 / 3.0), true);
        expect_eq(approx(w.update(wbuf[0..3], 3.0), 14.0 / 6.0), true);
        expect_eq(approx(w.update(wbuf[0..3], 4.0), 20.0 / 6.0), true);
        w.reset();
        expect_eq(w.count, 0);
    });
}
