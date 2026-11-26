// cap/test/testCapKalman1d.cp
// Comprehensive unit tests for @cap/kalman1d

import { test, expect_eq } from "cap:test";
import {
    Kalman1D,
} from "../../packages/@cap/kalman1d/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/kalman1d — Constant signal convergence", () => {
        let kf = Kalman1D.new(0.01 as f64, 1.0 as f64);
        let i: usize = 0;
        let est: f64 = 0.0;
        while (i < 20) {
            est = kf.filter(42.0 as f64, 0.0 as f64);
            i += 1;
        }
        expect_eq(near(est, 42.0 as f64, 0.000001 as f64), true);
        expect_eq(near(kf.lastMeasurement(), 42.0 as f64, 0.000001 as f64), true);
    });

    test("2. @cap/kalman1d — Noisy oscillating signal variance significantly reduced", () => {
        let kf = Kalman1D.new(0.01 as f64, 4.0 as f64);
        let rawErrSq: f64 = 0.0;
        let filtErrSq: f64 = 0.0;
        const truth: f64 = 100.0;

        let i: usize = 0;
        while (i < 40) {
            let noise: f64 = 3.0;
            if ((i % 2) == 1) {
                noise = -3.0;
            }
            const z: f64 = truth + noise;
            const est: f64 = kf.filter(z, 0.0 as f64);
            if (i >= 10) {
                const re: f64 = z - truth;
                const fe: f64 = est - truth;
                rawErrSq = rawErrSq + (re * re);
                filtErrSq = filtErrSq + (fe * fe);
            }
            i += 1;
        }
        // Filtered squared error should be a tiny fraction of raw noise squared error.
        expect_eq(filtErrSq < (rawErrSq * (0.1 as f64)), true);
    });

    test("3. @cap/kalman1d — First call initializes state precisely to z / c", () => {
        let kf = Kalman1D.withModel(0.01 as f64, 2.0 as f64, 1.0 as f64, 0.0 as f64, 2.0 as f64);
        expect_eq(kf.initialized, false);
        const first: f64 = kf.filter(20.0 as f64, 0.0 as f64);
        expect_eq(kf.initialized, true);
        expect_eq(near(first, 10.0 as f64, 0.000001 as f64), true);
        expect_eq(near(kf.cov, 0.5 as f64, 0.000001 as f64), true);
    });

    test("4. @cap/kalman1d — predict() does not mutate internal state", () => {
        let kf = Kalman1D.withModel(0.01 as f64, 1.0 as f64, 1.0 as f64, 2.0 as f64, 1.0 as f64);
        * = kf.filter(10.0 as f64, 0.0 as f64);
        const prevX: f64 = kf.x;
        const prevCov: f64 = kf.cov;

        const pred: f64 = kf.predict(3.0 as f64);
        expect_eq(near(pred, 16.0 as f64, 0.000001 as f64), true);
        expect_eq(near(kf.x, prevX, 0.000001 as f64), true);
        expect_eq(near(kf.cov, prevCov, 0.000001 as f64), true);
    });

    test("5. @cap/kalman1d — Linear system tracking with control input b*u", () => {
        // System: x_k = x_{k-1} + 1.0 * u_k, with u = 2.0 per step -> x = 0, 2, 4, 6, ...
        let kf = Kalman1D.withModel(0.001 as f64, 0.5 as f64, 1.0 as f64, 1.0 as f64, 1.0 as f64);
        let step: usize = 0;
        let est: f64 = 0.0;
        while (step < 15) {
            const trueX: f64 = (step as f64) * (2.0 as f64);
            let jitter: f64 = 0.2;
            if ((step % 2) == 1) {
                jitter = -0.2;
            }
            est = kf.filter(trueX + jitter, 2.0 as f64);
            step += 1;
        }
        // At step 14, trueX = 28.0
        expect_eq(near(est, 28.0 as f64, 0.25 as f64), true);
    });

    test("6. @cap/kalman1d — reset() clears state, covariance & initialized flag", () => {
        let kf = Kalman1D.new(0.05 as f64, 1.0 as f64);
        * = kf.filter(50.0 as f64, 0.0 as f64);
        * = kf.filter(52.0 as f64, 0.0 as f64);
        expect_eq(kf.initialized, true);

        kf.reset();
        expect_eq(kf.initialized, false);
        expect_eq(near(kf.x, 0.0 as f64, 0.000001 as f64), true);
        expect_eq(near(kf.cov, 0.0 as f64, 0.000001 as f64), true);

        // Next filter call re-initializes cleanly.
        const fresh: f64 = kf.filter(-7.5 as f64, 0.0 as f64);
        expect_eq(near(fresh, -7.5 as f64, 0.000001 as f64), true);
    });
}
