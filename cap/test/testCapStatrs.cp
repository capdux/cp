import { test, expect_eq } from "cap:test";
import {
    LinearFit,
    mean,
    variance,
    popVariance,
    stdDev,
    covariance,
    pearsonCorr,
    erf,
    normalPdf,
    normalCdf,
    exponentialPdf,
    exponentialCdf,
    poissonPmf,
    binomialCoeff,
    linearRegression
} from "../../packages/statrs/src/index.cp";

fn statTestNear(a: f64, b: f64, tol: f64): bool {
    let diff: f64 = if (a > b) { a - b } else { b - a };
    return diff <= tol;
}

fn main() {
    test("1. @cap/statrs — mean, variance, popVariance & stdDev", () => {
        let xs: [5]f64 = [2.0, 4.0, 4.0, 4.0, 6.0];
        let m: f64 = mean(xs[0..5]) ?? 0.0;
        expect_eq(statTestNear(m, 4.0, 1.0e-12), true);

        let pv: f64 = popVariance(xs[0..5]) ?? 0.0;
        expect_eq(statTestNear(pv, 1.6, 1.0e-12), true);

        let sv: f64 = variance(xs[0..5]) ?? 0.0;
        expect_eq(statTestNear(sv, 2.0, 1.0e-12), true);

        let sd: f64 = stdDev(xs[0..5]) ?? 0.0;
        expect_eq(statTestNear(sd, 1.4142135623730951, 1.0e-12), true);

        let single: [1]f64 = [5.0];
        expect_eq(variance(single[0..1]) == null, true);
        expect_eq(mean(xs[0..0]) == null, true);
    });

    test("2. @cap/statrs — covariance & pearsonCorr", () => {
        let xs: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        let ys: [4]f64 = [2.0, 4.0, 6.0, 8.0];
        let neg_ys: [4]f64 = [8.0, 6.0, 4.0, 2.0];

        let cov: f64 = covariance(xs[0..4], ys[0..4]) ?? 0.0;
        expect_eq(statTestNear(cov, 3.3333333333333335, 1.0e-12), true);

        let r_pos: f64 = pearsonCorr(xs[0..4], ys[0..4]) ?? 0.0;
        expect_eq(statTestNear(r_pos, 1.0, 1.0e-12), true);

        let r_neg: f64 = pearsonCorr(xs[0..4], neg_ys[0..4]) ?? 0.0;
        expect_eq(statTestNear(r_neg, -1.0, 1.0e-12), true);

        let flat: [4]f64 = [3.0, 3.0, 3.0, 3.0];
        expect_eq(pearsonCorr(xs[0..4], flat[0..4]) == null, true);
    });

    test("3. @cap/statrs — erf, normalPdf & normalCdf (A&S 7.1.26 precision)", () => {
        expect_eq(statTestNear(erf(0.0), 0.0, 1.0e-7), true);
        expect_eq(statTestNear(erf(1.0), 0.84270079, 1.5e-7), true);
        expect_eq(statTestNear(erf(-1.0), -0.84270079, 1.5e-7), true);

        expect_eq(statTestNear(normalPdf(0.0, 0.0, 1.0), 0.3989422804014327, 1.0e-12), true);
        expect_eq(statTestNear(normalCdf(0.0, 0.0, 1.0), 0.5, 1.0e-7), true);
        expect_eq(statTestNear(normalCdf(1.96, 0.0, 1.0), 0.9750021, 1.0e-5), true);
        expect_eq(normalPdf(0.0, 0.0, 0.0), 0.0);
    });

    test("4. @cap/statrs — exponentialPdf & exponentialCdf", () => {
        expect_eq(statTestNear(exponentialPdf(0.0, 2.0), 2.0, 1.0e-12), true);
        expect_eq(statTestNear(exponentialPdf(1.0, 1.0), 0.36787944117144233, 1.0e-12), true);
        expect_eq(statTestNear(exponentialCdf(0.0, 2.0), 0.0, 1.0e-12), true);
        expect_eq(statTestNear(exponentialCdf(1.0, 1.0), 0.6321205588285577, 1.0e-12), true);
        expect_eq(exponentialPdf(-1.0, 2.0), 0.0);
        expect_eq(exponentialCdf(-1.0, 2.0), 0.0);
    });

    test("5. @cap/statrs — poissonPmf & binomialCoeff", () => {
        // Poisson(lambda=2): P(0)=e^-2, P(2)=2*e^-2
        expect_eq(statTestNear(poissonPmf(0, 2.0), 0.1353352832366127, 1.0e-12), true);
        expect_eq(statTestNear(poissonPmf(2, 2.0), 0.2706705664732254, 1.0e-12), true);
        expect_eq(poissonPmf(0, 0.0), 1.0);
        expect_eq(poissonPmf(1, 0.0), 0.0);

        expect_eq(binomialCoeff(5, 2), 10);
        expect_eq(binomialCoeff(10, 0), 1);
        expect_eq(binomialCoeff(10, 10), 1);
        expect_eq(binomialCoeff(5, 6), 0);
        expect_eq(binomialCoeff(30, 15), 155117520);
    });

    test("6. @cap/statrs — linearRegression exact & noisy fit", () => {
        let xs: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        let ys: [4]f64 = [5.0, 7.0, 9.0, 11.0]; // y = 2x + 3
        let fit: LinearFit = linearRegression(xs[0..4], ys[0..4]);
        expect_eq(fit.valid, true);
        expect_eq(statTestNear(fit.slope, 2.0, 1.0e-12), true);
        expect_eq(statTestNear(fit.intercept, 3.0, 1.0e-12), true);
        expect_eq(statTestNear(fit.r_squared, 1.0, 1.0e-12), true);

        let const_x: [3]f64 = [2.0, 2.0, 2.0];
        let some_y: [3]f64 = [1.0, 2.0, 3.0];
        expect_eq(linearRegression(const_x[0..3], some_y[0..3]).valid, false);
    });
}
