import { test, expect_eq } from "cap:test";
import {
    PI,
    cosF64,
    isPowerOfTwo,
    fftInPlace,
    ifftInPlace,
    magnitudeSpectrum,
    dominantBin,
} from "../../packages/@cap/fft-radix2/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) {
        d = 0.0 - d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/fft-radix2 — isPowerOfTwo & invalid length rejection", () => {
        expect_eq(isPowerOfTwo(0), false);
        expect_eq(isPowerOfTwo(1), true);
        expect_eq(isPowerOfTwo(2), true);
        expect_eq(isPowerOfTwo(3), false);
        expect_eq(isPowerOfTwo(8), true);
        expect_eq(isPowerOfTwo(12), false);
        expect_eq(isPowerOfTwo(1024), true);

        let r3: [3]f64 = [1.0, 2.0, 3.0];
        let i3: [3]f64 = [0.0, 0.0, 0.0];
        expect_eq(fftInPlace(r3[0..3], i3[0..3]), false);

        let r4: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        expect_eq(fftInPlace(r4[0..4], i3[0..3]), false);
    });

    test("2. @cap/fft-radix2 — constant DC signal concentrates energy at bin 0", () => {
        let eps: f64 = 0.0001;
        let re: [8]f64 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0];
        let im: [8]f64 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
        expect_eq(fftInPlace(re[0..8], im[0..8]), true);

        let mag: [8]f64 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
        let written: usize = magnitudeSpectrum(re[0..8], im[0..8], mag[0..8]);
        expect_eq(written, 8 as usize);
        expect_eq(near(mag[0], 8.0, eps), true);
        expect_eq(dominantBin(re[0..8], im[0..8]), 0 as usize);

        let k: usize = 1;
        while (k < 8) {
            expect_eq(near(mag[k], 0.0, eps), true);
            k = k + 1;
        }
    });

    test("3. @cap/fft-radix2 — pure cosine hits exact frequency bin", () => {
        let eps: f64 = 0.0001;
        // N = 16, cosine at frequency bin k = 3: cos(2*pi*3*n/16)
        let re: [16]f64 = [
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        ];
        let im: [16]f64 = [
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        ];
        let n_idx: usize = 0;
        while (n_idx < 16) {
            let phase: f64 = 2.0 * PI * 3.0 * (n_idx as f64) / 16.0;
            re[n_idx] = cosF64(phase);
            n_idx = n_idx + 1;
        }
        expect_eq(fftInPlace(re[0..16], im[0..16]), true);

        let mag: [16]f64 = [
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        ];
        * = magnitudeSpectrum(re[0..16], im[0..16], mag[0..16]);
        // Positive freq bin 3 and mirror bin 13 both have magnitude N/2 = 8.0
        expect_eq(near(mag[3], 8.0, eps), true);
        expect_eq(near(mag[13], 8.0, eps), true);
        expect_eq(near(mag[0], 0.0, eps), true);
        expect_eq(dominantBin(re[0..8], im[0..8]), 3 as usize);
    });

    test("4. @cap/fft-radix2 — FFT + IFFT round-trip reconstructs original signal", () => {
        let eps: f64 = 0.0001;
        let orig_re: [8]f64 = [1.5, -2.0, 0.25, 4.0, -1.25, 3.5, 0.0, -0.75];
        let orig_im: [8]f64 = [0.5, 1.0, -0.5, 0.25, 2.0, -1.5, 0.75, -0.25];
        let re: [8]f64 = [1.5, -2.0, 0.25, 4.0, -1.25, 3.5, 0.0, -0.75];
        let im: [8]f64 = [0.5, 1.0, -0.5, 0.25, 2.0, -1.5, 0.75, -0.25];

        expect_eq(fftInPlace(re[0..8], im[0..8]), true);
        expect_eq(ifftInPlace(re[0..8], im[0..8]), true);

        let k: usize = 0;
        while (k < 8) {
            expect_eq(near(re[k], orig_re[k], eps), true);
            expect_eq(near(im[k], orig_im[k], eps), true);
            k = k + 1;
        }
    });

    test("5. @cap/fft-radix2 — Parseval energy conservation sum(|x|^2) == sum(|X|^2)/N", () => {
        let eps: f64 = 0.0001;
        let re: [8]f64 = [2.0, -1.0, 3.0, 0.5, -2.5, 1.5, 4.0, -0.5];
        let im: [8]f64 = [1.0, 0.5, -1.5, 2.0, 0.0, -1.0, 0.25, 1.25];

        let time_energy: f64 = 0.0;
        let i: usize = 0;
        while (i < 8) {
            time_energy = time_energy + re[i] * re[i] + im[i] * im[i];
            i = i + 1;
        }

        expect_eq(fftInPlace(re[0..8], im[0..8]), true);

        let freq_energy: f64 = 0.0;
        let k: usize = 0;
        while (k < 8) {
            freq_energy = freq_energy + re[k] * re[k] + im[k] * im[k];
            k = k + 1;
        }
        expect_eq(near(time_energy, freq_energy / 8.0, eps), true);
    });

    test("6. @cap/fft-radix2 — N=1 single point & all-zero signal edge cases", () => {
        let eps: f64 = 0.0001;
        let re1: [1]f64 = [42.5];
        let im1: [1]f64 = [-3.25];
        expect_eq(fftInPlace(re1[0..1], im1[0..1]), true);
        expect_eq(near(re1[0], 42.5, eps), true);
        expect_eq(near(im1[0], -3.25, eps), true);
        expect_eq(ifftInPlace(re1[0..1], im1[0..1]), true);
        expect_eq(near(re1[0], 42.5, eps), true);

        let rz: [4]f64 = [0.0, 0.0, 0.0, 0.0];
        let iz: [4]f64 = [0.0, 0.0, 0.0, 0.0];
        expect_eq(fftInPlace(rz[0..4], iz[0..4]), true);
        expect_eq(dominantBin(rz[0..4], iz[0..4]), 0 as usize);
        expect_eq(near(rz[0], 0.0, eps), true);
        expect_eq(near(rz[3], 0.0, eps), true);
    });
}
