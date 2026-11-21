// Source: cp/cap/test/test_biquad_production_deep.cp (Production deep tests for cap:biquad)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FilterType,
    BiquadFilter,
    designBiquad,
    processSosBlock,
    frequencyResponse,
    goertzelDetect,
} from "cap:biquad";

const TWO_PI: f64 = 6.28318530717958647692;
const LN10: f64 = 2.30258509299404568402;
const BUTTERWORTH_Q: f64 = 0.70710678118654752440;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn peakSteadyState(samples: []const f64, start: usize): f64 {
    let peak: f64 = 0.0;
    let i = start;
    while (i < samples.len) {
        const a = absF64(samples[i]);
        if (a > peak) {
            peak = a;
        }
        i += 1;
    }
    return peak;
}

test("1. LowPass & HighPass steady-state DC & high-frequency attenuation + reset/in-place", () => {
    const fs: f64 = 8000.0;
    const fc: f64 = 500.0;

    let lp_coeffs = designBiquad(FilterType.LowPass, fs, fc, BUTTERWORTH_Q, 0.0);
    let hp_coeffs = designBiquad(FilterType.HighPass, fs, fc, BUTTERWORTH_Q, 0.0);
    expect_eq(lp_coeffs != null, true);
    expect_eq(hp_coeffs != null, true);

    if (lpc := lp_coeffs) {
        if (hpc := hp_coeffs) {
            let lp = BiquadFilter.init(lpc);
            let hp = BiquadFilter.init(hpc);

            // DC step response (512 samples)
            let dc_in: [512]f64 = [1.0; 512];
            let lp_dc_buf: [512]f64 = [0.0; 512];
            let hp_dc_buf: [512]f64 = [0.0; 512];
            let lp_dc: []f64 = lp_dc_buf[0..];
            let hp_dc: []f64 = hp_dc_buf[0..];

            expect_eq(lp.processBlock(dc_in[0..], *lp_dc), true);
            expect_eq(hp.processBlock(dc_in[0..], *hp_dc), true);
            expect_eq(absF64(lp_dc[511] - 1.0) < 1.0e-6, true);
            expect_eq(absF64(hp_dc[511]) < 1.0e-5, true);

            // Reset & test 3000 Hz sine wave in-place
            lp.reset();
            hp.reset();
            let lp_hf_buf: [512]f64 = [0.0; 512];
            let hp_hf_buf: [512]f64 = [0.0; 512];
            let i: usize = 0;
            while (i < 512) {
                const s = @sin(TWO_PI * 3000.0 * (i as f64) / fs);
                lp_hf_buf[i] = s;
                hp_hf_buf[i] = s;
                i += 1;
            }
            let lp_hf: []f64 = lp_hf_buf[0..];
            let hp_hf: []f64 = hp_hf_buf[0..];
            expect_eq(lp.processBlock(lp_hf, *lp_hf), true);
            expect_eq(hp.processBlock(hp_hf, *hp_hf), true);

            const lp_peak = peakSteadyState(lp_hf, 256);
            const hp_peak = peakSteadyState(hp_hf, 256);
            const lp_att_db = (20.0 / LN10) * @log(lp_peak);
            expect_eq(lp_att_db < -30.0, true);
            expect_eq(absF64(hp_peak - 1.0) < 0.02, true);
        }
    }
});

test("2. Notch filter attenuates 1kHz tone by > 40 dB while passing 200Hz cleanly", () => {
    const fs: f64 = 8000.0;
    if (nc := designBiquad(FilterType.Notch, fs, 1000.0, 8.0, 0.0)) {
        let notch = BiquadFilter.init(nc);

        // 2048 samples of 200Hz desired + 1000Hz interferer (both exact integer cycles in 1024-sample steady tail)
        let sig_buf: [2048]f64 = [0.0; 2048];
        let out_buf: [2048]f64 = [0.0; 2048];
        let i: usize = 0;
        while (i < 2048) {
            const t = (i as f64) / fs;
            sig_buf[i] = @sin(TWO_PI * 200.0 * t) + @sin(TWO_PI * 1000.0 * t);
            i += 1;
        }
        let out_s: []f64 = out_buf[0..];
        expect_eq(notch.processBlock(sig_buf[0..], *out_s), true);

        // Inspect steady-state second half (indices 1024..2048, length 1024; 200Hz & 1000Hz exact integer bins? 200*1024/8000 = 25.6 -> use 800-sample tail for exact integer bins!)
        const tail = out_s[1248..2048];
        if (g200 := goertzelDetect(tail, fs, 200.0)) {
            if (g1000 := goertzelDetect(tail, fs, 1000.0)) {
                expect_eq(absF64(g200.magnitude - 1.0) < 0.02, true);
                expect_eq(g1000.magnitude < 0.01, true); // > 40 dB attenuation
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // Analytic response at notch center must be deeply attenuated (< -100 dB)
        if (resp := frequencyResponse(nc, fs, 1000.0)) {
            expect_eq(resp.magnitude_db < -100.0, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. PeakingEq, LowShelf & HighShelf hit exact configured gain_db at target frequencies", () => {
    const fs: f64 = 48000.0;

    // PeakingEq +6 dB at 1000 Hz
    if (pk := designBiquad(FilterType.PeakingEq, fs, 1000.0, 2.0, 6.0)) {
        if (r_center := frequencyResponse(pk, fs, 1000.0)) {
            expect_eq(absF64(r_center.magnitude_db - 6.0) < 1.0e-9, true);
        } else {
            expect_eq(true, false);
        }
        if (r_dc := frequencyResponse(pk, fs, 0.0)) {
            expect_eq(absF64(r_dc.magnitude_db) < 1.0e-9, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // LowShelf +9 dB at 300 Hz -> DC must be +9 dB, Nyquist ~ 0 dB
    if (ls := designBiquad(FilterType.LowShelf, fs, 300.0, BUTTERWORTH_Q, 9.0)) {
        if (r_dc := frequencyResponse(ls, fs, 0.0)) {
            expect_eq(absF64(r_dc.magnitude_db - 9.0) < 1.0e-9, true);
        } else {
            expect_eq(true, false);
        }
        if (r_nyq := frequencyResponse(ls, fs, 24000.0)) {
            expect_eq(absF64(r_nyq.magnitude_db) < 1.0e-6, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // HighShelf -6 dB at 4000 Hz -> DC must be 0 dB, Nyquist -6 dB
    if (hs := designBiquad(FilterType.HighShelf, fs, 4000.0, BUTTERWORTH_Q, -6.0)) {
        if (r_dc := frequencyResponse(hs, fs, 0.0)) {
            expect_eq(absF64(r_dc.magnitude_db) < 1.0e-9, true);
        } else {
            expect_eq(true, false);
        }
        if (r_nyq := frequencyResponse(hs, fs, 24000.0)) {
            expect_eq(absF64(r_nyq.magnitude_db + 6.0) < 1.0e-6, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. AllPass maintains unity magnitude (== 1.0) across the entire spectrum", () => {
    const fs: f64 = 8000.0;
    if (ap := designBiquad(FilterType.AllPass, fs, 1200.0, 1.5, 0.0)) {
        const freqs: [8]f64 = [0.0, 100.0, 500.0, 1200.0, 2000.0, 3000.0, 3900.0, 4000.0];
        let i: usize = 0;
        while (i < 8) {
            if (r := frequencyResponse(ap, fs, freqs[i])) {
                expect_eq(absF64(r.magnitude - 1.0) < 1.0e-12, true);
                expect_eq(absF64(r.magnitude_db) < 1.0e-11, true);
            } else {
                expect_eq(true, false);
            }
            i += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Cascaded 4th-order SOS LowPass achieves steeper stopband roll-off than 2nd-order stage", () => {
    const fs: f64 = 8000.0;
    const fc: f64 = 500.0;
    if (c := designBiquad(FilterType.LowPass, fs, fc, BUTTERWORTH_Q, 0.0)) {
        let single = BiquadFilter.init(c);
        let stages_arr: [2]BiquadFilter = [BiquadFilter.init(c), BiquadFilter.init(c)];
        let stages: []BiquadFilter = stages_arr[0..];

        let sig: [512]f64 = [0.0; 512];
        let out_2nd_buf: [512]f64 = [0.0; 512];
        let out_4th_buf: [512]f64 = [0.0; 512];
        let i: usize = 0;
        while (i < 512) {
            sig[i] = @sin(TWO_PI * 1500.0 * (i as f64) / fs);
            i += 1;
        }
        let out_2nd: []f64 = out_2nd_buf[0..];
        let out_4th: []f64 = out_4th_buf[0..];

        expect_eq(single.processBlock(sig[0..], *out_2nd), true);
        expect_eq(processSosBlock(*stages, sig[0..], *out_4th), true);

        const peak_2nd = peakSteadyState(out_2nd, 256);
        const peak_4th = peakSteadyState(out_4th, 256);
        // 4th-order amplitude should be roughly (peak_2nd)^2, strictly much smaller
        expect_eq(peak_4th < peak_2nd * 0.25, true);
        expect_eq(absF64(peak_4th - peak_2nd * peak_2nd) < 0.01, true);
    } else {
        expect_eq(true, false);
    }
});

test("6. goertzelDetect accurately extracts amplitudes in multi-tone composite signal", () => {
    const fs: f64 = 8000.0;
    // N = 800 => bin spacing = 10 Hz, so 700 Hz, 1210 Hz, 2000 Hz, 1500 Hz are exact integer bins
    let sig: [800]f64 = [0.0; 800];
    let i: usize = 0;
    while (i < 800) {
        const t = (i as f64) / fs;
        sig[i] = 0.8 * @sin(TWO_PI * 700.0 * t) + 1.2 * @sin(TWO_PI * 1210.0 * t) + 0.5 * @cos(TWO_PI * 2000.0 * t);
        i += 1;
    }

    if (g700 := goertzelDetect(sig[0..], fs, 700.0)) {
        expect_eq(absF64(g700.magnitude - 0.8) < 1.0e-11, true);
        expect_eq(g700.power > 0.15, true);
    } else {
        expect_eq(true, false);
    }
    if (g1210 := goertzelDetect(sig[0..], fs, 1210.0)) {
        expect_eq(absF64(g1210.magnitude - 1.2) < 1.0e-11, true);
    } else {
        expect_eq(true, false);
    }
    if (g2000 := goertzelDetect(sig[0..], fs, 2000.0)) {
        expect_eq(absF64(g2000.magnitude - 0.5) < 1.0e-11, true);
    } else {
        expect_eq(true, false);
    }
    // Absent frequency bin must have ~0 amplitude
    if (g1500 := goertzelDetect(sig[0..], fs, 1500.0)) {
        expect_eq(g1500.magnitude < 1.0e-11, true);
    } else {
        expect_eq(true, false);
    }
});

test("7. Boundary defense: Nyquist/out-of-range freq, q <= 0, fs <= 0, short buffer & NaN", () => {
    const nan = std.math.nan(f64);
    const inf = std.math.inf(f64);

    // designBiquad invalid params
    expect_eq(designBiquad(FilterType.LowPass, 8000.0, 4000.0, 0.707, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, 8000.0, 5000.0, 0.707, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, 8000.0, 0.0, 0.707, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, 8000.0, -100.0, 0.707, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, 8000.0, 500.0, 0.0, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, 8000.0, 500.0, -1.0, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, 0.0, 500.0, 0.707, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, -8000.0, 500.0, 0.707, 0.0) == null, true);
    expect_eq(designBiquad(FilterType.PeakingEq, 8000.0, 500.0, 1.0, nan) == null, true);
    expect_eq(designBiquad(FilterType.LowPass, 8000.0, nan, 0.707, 0.0) == null, true);

    // Short output buffer in processBlock & processSosBlock
    if (c := designBiquad(FilterType.BandPass, 8000.0, 1000.0, 1.0, 0.0)) {
        let flt = BiquadFilter.init(c);
        let stages_arr: [1]BiquadFilter = [BiquadFilter.init(c)];
        let stages: []BiquadFilter = stages_arr[0..];
        const inp: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        let short_buf: [2]f64 = [0.0; 2];
        let short_out: []f64 = short_buf[0..];
        expect_eq(flt.processBlock(inp[0..], *short_out), false);
        expect_eq(processSosBlock(*stages, inp[0..], *short_out), false);

        // frequencyResponse invalid range
        expect_eq(frequencyResponse(c, 8000.0, -1.0) == null, true);
        expect_eq(frequencyResponse(c, 8000.0, 4001.0) == null, true);
        expect_eq(frequencyResponse(c, 0.0, 100.0) == null, true);
    } else {
        expect_eq(true, false);
    }

    // goertzelDetect invalid inputs
    const sig_ok: [4]f64 = [1.0, 0.0, -1.0, 0.0];
    const sig_nan: [4]f64 = [1.0, nan, -1.0, 0.0];
    const sig_inf: [4]f64 = [1.0, inf, -1.0, 0.0];
    expect_eq(goertzelDetect(sig_ok[0..0], 8000.0, 1000.0) == null, true);
    expect_eq(goertzelDetect(sig_ok[0..], 0.0, 1000.0) == null, true);
    expect_eq(goertzelDetect(sig_ok[0..], 8000.0, 4001.0) == null, true);
    expect_eq(goertzelDetect(sig_ok[0..], 8000.0, -10.0) == null, true);
    expect_eq(goertzelDetect(sig_nan[0..], 8000.0, 1000.0) == null, true);
    expect_eq(goertzelDetect(sig_inf[0..], 8000.0, 1000.0) == null, true);
});
