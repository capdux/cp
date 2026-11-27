// cap/test/testCapFft.cp
// Comprehensive unit tests for @cap/fft

import { test, expect_eq } from "cap:test";
import {
    Complex,
    isPowerOfTwo,
    fftInPlace,
    ifftInPlace,
    magnitude,
    applyHannWindow,
    applyHammingWindow,
} from "../../packages/rustfft/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn sinSimple(x_in: f64) -> f64 {
    const pi: f64 = 3.14159265358979323846;
    const twoPi: f64 = 6.28318530717958647692;
    const halfPi: f64 = 1.57079632679489661923;
    let x = x_in;
    const turns = ((x / twoPi) as i64) as f64;
    x -= turns * twoPi;
    while (x > pi) { x -= twoPi; }
    while (x < -pi) { x += twoPi; }
    if (x > halfPi) { x = pi - x; } else if (x < -halfPi) { x = -pi - x; }
    const x2 = x * x;
    let term = x;
    let sum = x;
    let k: usize = 1;
    while (k <= 10) {
        term = (-term * x2) / (((2 * k) as f64) * (((2 * k) + 1) as f64));
        sum += term;
        k += 1;
    }
    return sum;
}

fn main() {
    test("1. @cap/fft — isPowerOfTwo, non-power-of-two rejection & n=1 boundary", () => {
        expect_eq(isPowerOfTwo(0 as usize), false);
        expect_eq(isPowerOfTwo(1 as usize), true);
        expect_eq(isPowerOfTwo(2 as usize), true);
        expect_eq(isPowerOfTwo(3 as usize), false);
        expect_eq(isPowerOfTwo(32 as usize), true);
        expect_eq(isPowerOfTwo(30 as usize), false);

        let bad: [3]Complex = [
            Complex{ re: 1.0, im: 0.0 },
            Complex{ re: 2.0, im: 0.0 },
            Complex{ re: 3.0, im: 0.0 },
        ];
        expect_eq(fftInPlace(bad[0..3]), false);
        expect_eq(ifftInPlace(bad[0..3]), false);

        let one: [1]Complex = [Complex{ re: 7.5, im: -2.5 }];
        expect_eq(fftInPlace(one[0..1]), true);
        expect_eq(approxEq(one[0].re, 7.5, 0.000000001), true);
        expect_eq(approxEq(one[0].im, -2.5, 0.000000001), true);
    });

    test("2. @cap/fft — DC & impulse exact spectra", () => {
        // Impulse [1, 0, 0, 0] -> all bins = 1 + 0i
        let imp: [4]Complex = [
            Complex{ re: 1.0, im: 0.0 },
            Complex{ re: 0.0, im: 0.0 },
            Complex{ re: 0.0, im: 0.0 },
            Complex{ re: 0.0, im: 0.0 },
        ];
        expect_eq(fftInPlace(imp[0..4]), true);
        let i: usize = 0;
        while (i < 4) {
            expect_eq(approxEq(imp[i].re, 1.0, 0.000000001), true);
            expect_eq(approxEq(imp[i].im, 0.0, 0.000000001), true);
            i += 1;
        }

        // DC [2, 2, 2, 2] -> bin 0 = 8, others = 0
        let dc: [4]Complex = [
            Complex{ re: 2.0, im: 0.0 },
            Complex{ re: 2.0, im: 0.0 },
            Complex{ re: 2.0, im: 0.0 },
            Complex{ re: 2.0, im: 0.0 },
        ];
        expect_eq(fftInPlace(dc[0..4]), true);
        expect_eq(approxEq(magnitude(dc[0]), 8.0, 0.000000001), true);
        expect_eq(approxEq(magnitude(dc[1]), 0.0, 0.000000001), true);
        expect_eq(approxEq(magnitude(dc[2]), 0.0, 0.000000001), true);
        expect_eq(approxEq(magnitude(dc[3]), 0.0, 0.000000001), true);
    });

    test("3. @cap/fft — Pure sine wave k=3 on N=32 peaks cleanly at bins 3 & 29", () => {
        const twoPi: f64 = 6.28318530717958647692;
        let sig: [32]Complex = undefined;
        let n: usize = 0;
        while (n < 32) {
            const val = sinSimple((twoPi * 3.0 * (n as f64)) / 32.0);
            sig[n] = Complex{ re: val, im: 0.0 };
            n += 1;
        }
        expect_eq(fftInPlace(sig[0..32]), true);
        // For sin(2*pi*3*n/32), |X[3]| = |X[29]| = N/2 = 16.0, all others ~0
        expect_eq(approxEq(magnitude(sig[3]), 16.0, 0.000001), true);
        expect_eq(approxEq(magnitude(sig[29]), 16.0, 0.000001), true);
        expect_eq(approxEq(magnitude(sig[0]), 0.0, 0.000001), true);
        expect_eq(approxEq(magnitude(sig[4]), 0.0, 0.000001), true);
        expect_eq(approxEq(magnitude(sig[16]), 0.0, 0.000001), true);
    });

    test("4. @cap/fft — Parseval's energy conservation theorem", () => {
        let buf: [16]Complex = undefined;
        let timeEnergy: f64 = 0.0;
        let i: usize = 0;
        while (i < 16) {
            const re = ((i as f64) * 0.7) - 5.0;
            const im = ((i as f64) * -0.3) + 2.0;
            buf[i] = Complex{ re: re, im: im };
            timeEnergy += (re * re) + (im * im);
            i += 1;
        }
        expect_eq(fftInPlace(buf[0..16]), true);
        let freqEnergy: f64 = 0.0;
        let k: usize = 0;
        while (k < 16) {
            const m = magnitude(buf[k]);
            freqEnergy += m * m;
            k += 1;
        }
        expect_eq(approxEq(timeEnergy, freqEnergy / 16.0, 0.000001), true);
    });

    test("5. @cap/fft — Lossless fftInPlace -> ifftInPlace round-trip (< 1e-9 error)", () => {
        let orig: [16]Complex = undefined;
        let work: [16]Complex = undefined;
        let i: usize = 0;
        while (i < 16) {
            const c = Complex{ re: ((i as f64) * 1.25) - 4.0, im: ((i as f64) * -0.5) + 3.0 };
            orig[i] = c;
            work[i] = c;
            i += 1;
        }
        expect_eq(fftInPlace(work[0..16]), true);
        expect_eq(ifftInPlace(work[0..16]), true);
        let k: usize = 0;
        while (k < 16) {
            expect_eq(approxEq(work[k].re, orig[k].re, 0.000000001), true);
            expect_eq(approxEq(work[k].im, orig[k].im, 0.000000001), true);
            k += 1;
        }
    });

    test("6. @cap/fft — Hann & Hamming window symmetry and endpoint values", () => {
        let hann: [9]f64 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0];
        applyHannWindow(hann[0..9]);
        // Endpoints = 0.0, center (index 4) = 1.0, symmetric
        expect_eq(approxEq(hann[0], 0.0, 0.000000001), true);
        expect_eq(approxEq(hann[8], 0.0, 0.000000001), true);
        expect_eq(approxEq(hann[4], 1.0, 0.000000001), true);
        expect_eq(approxEq(hann[2], hann[6], 0.000000001), true);

        let ham: [9]f64 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0];
        applyHammingWindow(ham[0..9]);
        // Endpoints = 0.08, center = 1.0, symmetric
        expect_eq(approxEq(ham[0], 0.08, 0.000000001), true);
        expect_eq(approxEq(ham[8], 0.08, 0.000000001), true);
        expect_eq(approxEq(ham[4], 1.0, 0.000000001), true);
        expect_eq(approxEq(ham[1], ham[7], 0.000000001), true);
    });
}
