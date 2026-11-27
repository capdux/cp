// cap/test/testCapKalman.cp
// Comprehensive unit tests for @cap/kalman

import { test, expect_eq } from "cap:test";
import {
    Kalman1D,
    Kalman2D,
} from "../../packages/kalman-filters/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/kalman — Kalman1D converges rapidly on constant signal with alternating noise", () => {
        let kf = Kalman1D.init(0.0, 10.0, 0.001, 1.0);
        let i: usize = 0;
        while (i < 40) {
            let noise: f64 = 0.5;
            if ((i % (2 as usize)) == (1 as usize)) {
                noise = -0.5;
            }
            * = kf.step(25.0 + noise);
            i += 1;
        }
        expect_eq(approxEq(kf.estimate(), 25.0, 0.15), true);
    });

    test("2. @cap/kalman — High measurement noise R vs low R gain behavior", () => {
        let lowR = Kalman1D.init(0.0, 1.0, 0.01, 0.01);
        let highR = Kalman1D.init(0.0, 1.0, 0.01, 100.0);

        const estLow = lowR.step(10.0);
        const estHigh = highR.step(10.0);

        // Low R trusts measurement (gain close to 1); High R trusts prior (gain close to 0)
        expect_eq(lowR.gain() > 0.9, true);
        expect_eq(highR.gain() < 0.02, true);
        expect_eq(estLow > 9.0, true);
        expect_eq(estHigh < 0.5, true);
    });

    test("3. @cap/kalman — Kalman1D error covariance decreases monotonically on static signal", () => {
        let kf = Kalman1D.init(0.0, 100.0, 0.0, 1.0);
        let prevVar = kf.variance();
        let i: usize = 0;
        while (i < 15) {
            * = kf.step(5.0);
            const curVar = kf.variance();
            expect_eq(curVar < prevVar, true);
            prevVar = curVar;
            i += 1;
        }
        expect_eq(prevVar < 0.1, true);
    });

    test("4. @cap/kalman — Kalman2D recovers true constant velocity from noisy position readings", () => {
        // True motion: pos(t) = 3.0 * t (true velocity = 3.0 m/s), dt = 1.0 s
        let k2 = Kalman2D.init(0.0, 0.0, 0.001, 0.001, 0.25);
        let t: usize = 1;
        while (t <= 30) {
            k2.predict(1.0);
            let noise: f64 = 0.2;
            if ((t % (2 as usize)) == (0 as usize)) {
                noise = -0.2;
            }
            const measured: f64 = (3.0 * (t as f64)) + noise;
            * = k2.updatePosition(measured);
            t += 1;
        }
        expect_eq(approxEq(k2.velocity(), 3.0, 0.1), true);
        expect_eq(approxEq(k2.position(), 90.0, 0.5), true);
    });

    test("5. @cap/kalman — Zero measurement noise tracks exact measurements immediately", () => {
        let kf = Kalman1D.init(0.0, 1.0, 0.1, 0.0);
        const v1 = kf.step(42.5);
        expect_eq(approxEq(v1, 42.5, 0.000000001), true);
        expect_eq(approxEq(kf.gain(), 1.0, 0.000000001), true);
        expect_eq(approxEq(kf.variance(), 0.0, 0.000000001), true);

        let k2 = Kalman2D.init(0.0, 0.0, 0.01, 0.01, 0.0);
        k2.predict(1.0);
        const p = k2.updatePosition(12.0);
        expect_eq(approxEq(p, 12.0, 0.000000001), true);
    });

    test("6. @cap/kalman — Kalman1D reset restores state and covariance cleanly", () => {
        let kf = Kalman1D.init(0.0, 1.0, 0.01, 0.5);
        * = kf.step(100.0);
        expect_eq(kf.estimate() > 50.0, true);

        kf.reset(-10.0, 5.0);
        expect_eq(approxEq(kf.estimate(), -10.0, 0.000000001), true);
        expect_eq(approxEq(kf.variance(), 5.0, 0.000000001), true);
        expect_eq(approxEq(kf.gain(), 0.0, 0.000000001), true);
    });
}
