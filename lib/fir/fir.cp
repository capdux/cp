// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc DSP FIR Windowed-Sinc, Hann/Hamming/Blackman Windows & RBJ Biquad IIR)

import std from "std";

const PI: f64 = 3.14159265358979323846;
const TWO_PI: f64 = 6.28318530717958647692;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

/// Generates a symmetric Hann window into `out`. Returns `false` if `out.len == 0`.
pub fn windowHann(out: []f64): bool {
    if (out.len == 0) {
        return false;
    }
    if (out.len == 1) {
        out[0] = 1.0;
        return true;
    }
    const denom = (out.len - 1) as f64;
    let i: usize = 0;
    while (i < out.len) {
        const phase = TWO_PI * (i as f64) / denom;
        out[i] = 0.5 * (1.0 - std.math.cos(phase));
        i += 1;
    }
    return true;
}

/// Generates a symmetric Hamming window into `out`. Returns `false` if `out.len == 0`.
pub fn windowHamming(out: []f64): bool {
    if (out.len == 0) {
        return false;
    }
    if (out.len == 1) {
        out[0] = 1.0;
        return true;
    }
    const denom = (out.len - 1) as f64;
    let i: usize = 0;
    while (i < out.len) {
        const phase = TWO_PI * (i as f64) / denom;
        out[i] = 0.54 - 0.46 * std.math.cos(phase);
        i += 1;
    }
    return true;
}

/// Generates a symmetric Blackman window into `out`. Returns `false` if `out.len == 0`.
pub fn windowBlackman(out: []f64): bool {
    if (out.len == 0) {
        return false;
    }
    if (out.len == 1) {
        out[0] = 1.0;
        return true;
    }
    const denom = (out.len - 1) as f64;
    let i: usize = 0;
    while (i < out.len) {
        const phase = TWO_PI * (i as f64) / denom;
        out[i] = 0.42 - 0.5 * std.math.cos(phase) + 0.08 * std.math.cos(2.0 * phase);
        i += 1;
    }
    return true;
}

/// Designs a Hamming-windowed Sinc lowpass FIR filter into `coeffs`.
/// - `cutoff_norm` is normalized cutoff frequency `fc / fs` in `(0.0, 0.5)`.
/// - Supports both odd and even tap counts and normalizes DC gain (`sum(coeffs) == 1.0`).
/// Returns `false` if `coeffs.len == 0` or `cutoff_norm` is out of range.
pub fn designLowpassFir(cutoff_norm: f64, coeffs: []f64): bool {
    if (coeffs.len == 0 || !isFiniteF64(cutoff_norm) || cutoff_norm <= 0.0 || cutoff_norm >= 0.5) {
        return false;
    }
    if (coeffs.len == 1) {
        coeffs[0] = 1.0;
        return true;
    }

    const denom = (coeffs.len - 1) as f64;
    const mid = denom * 0.5;
    let sum: f64 = 0.0;

    let i: usize = 0;
    while (i < coeffs.len) {
        const n = (i as f64) - mid;
        let sinc_val: f64 = 0.0;
        if (n == 0.0) {
            sinc_val = 2.0 * cutoff_norm;
        } else {
            sinc_val = std.math.sin(TWO_PI * cutoff_norm * n) / (PI * n);
        }
        const w = 0.54 - 0.46 * std.math.cos(TWO_PI * (i as f64) / denom);
        const c = sinc_val * w;
        coeffs[i] = c;
        sum = sum + c;
        i += 1;
    }

    if (sum == 0.0 || !isFiniteF64(sum)) {
        return false;
    }
    let j: usize = 0;
    while (j < coeffs.len) {
        coeffs[j] = coeffs[j] / sum;
        j += 1;
    }
    return true;
}

/// Zero-allocation streaming FIR filter backed by caller-owned `coeffs` and circular `delay` line.
pub struct FirFilter {
    coeffs: []const f64;
    delay: []f64;
    pos: usize;

    /// Initializes a `FirFilter`. Requires `coeffs.len >= 1` and `delay.len >= coeffs.len`.
    pub fn init(coeffs: []const f64, delay: []f64): FirFilter? {
        if (coeffs.len == 0 || delay.len < coeffs.len) {
            return null;
        }
        let i: usize = 0;
        while (i < coeffs.len) {
            delay[i] = 0.0;
            i += 1;
        }
        return FirFilter {
            coeffs: coeffs,
            delay: delay[0..coeffs.len],
            pos: 0,
        };
    }

    /// Resets the internal circular delay line to zero.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.delay.len) {
            _.delay[i] = 0.0;
            i += 1;
        }
        _.pos = 0;
    }

    /// Processes a single input sample and returns the filtered output sample.
    fn process(*_, sample: f64): f64 {
        const n = _.coeffs.len;
        _.delay[_.pos] = sample;

        let acc: f64 = 0.0;
        let idx = _.pos;
        let k: usize = 0;
        while (k < n) {
            acc = acc + _.coeffs[k] * _.delay[idx];
            if (idx == 0) {
                idx = n - 1;
            } else {
                idx -= 1;
            }
            k += 1;
        }

        _.pos += 1;
        if (_.pos >= n) {
            _.pos = 0;
        }
        return acc;
    }

    /// Processes a slice of input samples into `output`. Returns `false` if `output.len < input.len`.
    fn processSlice(*_, input: []const f64, output: []f64): bool {
        if (output.len < input.len) {
            return false;
        }
        let i: usize = 0;
        while (i < input.len) {
            output[i] = _.process(input[i]);
            i += 1;
        }
        return true;
    }
}

fn validateBiquadParams(fs: f64, fc: f64, q: f64): bool {
    if (!isFiniteF64(fs) || !isFiniteF64(fc) || !isFiniteF64(q)) {
        return false;
    }
    if (fs <= 0.0 || fc <= 0.0 || fc >= fs * 0.5 || q <= 0.0) {
        return false;
    }
    return true;
}

/// RBJ Audio EQ Cookbook second-order IIR filter (Direct Form II Transposed).
pub struct Biquad {
    b0: f64;
    b1: f64;
    b2: f64;
    a1: f64;
    a2: f64;
    z1: f64;
    z2: f64;

    /// Designs a 2nd-order RBJ lowpass biquad filter.
    pub fn lowpass(fs: f64, fc: f64, q: f64): Biquad? {
        if (!validateBiquadParams(fs, fc, q)) {
            return null;
        }
        const w0 = TWO_PI * fc / fs;
        const cos_w0 = std.math.cos(w0);
        const sin_w0 = std.math.sin(w0);
        const alpha = sin_w0 / (2.0 * q);

        const raw_b1 = 1.0 - cos_w0;
        const raw_b0 = raw_b1 * 0.5;
        const a0 = 1.0 + alpha;
        return Biquad {
            b0: raw_b0 / a0,
            b1: raw_b1 / a0,
            b2: raw_b0 / a0,
            a1: (0.0 - 2.0 * cos_w0) / a0,
            a2: (1.0 - alpha) / a0,
            z1: 0.0,
            z2: 0.0,
        };
    }

    /// Designs a 2nd-order RBJ highpass biquad filter.
    pub fn highpass(fs: f64, fc: f64, q: f64): Biquad? {
        if (!validateBiquadParams(fs, fc, q)) {
            return null;
        }
        const w0 = TWO_PI * fc / fs;
        const cos_w0 = std.math.cos(w0);
        const sin_w0 = std.math.sin(w0);
        const alpha = sin_w0 / (2.0 * q);

        const raw_b0 = (1.0 + cos_w0) * 0.5;
        const raw_b1 = 0.0 - (1.0 + cos_w0);
        const a0 = 1.0 + alpha;
        return Biquad {
            b0: raw_b0 / a0,
            b1: raw_b1 / a0,
            b2: raw_b0 / a0,
            a1: (0.0 - 2.0 * cos_w0) / a0,
            a2: (1.0 - alpha) / a0,
            z1: 0.0,
            z2: 0.0,
        };
    }

    /// Designs a 2nd-order RBJ constant-0dB-peak bandpass biquad filter.
    pub fn bandpass(fs: f64, fc: f64, q: f64): Biquad? {
        if (!validateBiquadParams(fs, fc, q)) {
            return null;
        }
        const w0 = TWO_PI * fc / fs;
        const cos_w0 = std.math.cos(w0);
        const sin_w0 = std.math.sin(w0);
        const alpha = sin_w0 / (2.0 * q);

        const a0 = 1.0 + alpha;
        return Biquad {
            b0: alpha / a0,
            b1: 0.0,
            b2: (0.0 - alpha) / a0,
            a1: (0.0 - 2.0 * cos_w0) / a0,
            a2: (1.0 - alpha) / a0,
            z1: 0.0,
            z2: 0.0,
        };
    }

    /// Designs a 2nd-order RBJ notch (band-stop) biquad filter.
    pub fn notch(fs: f64, fc: f64, q: f64): Biquad? {
        if (!validateBiquadParams(fs, fc, q)) {
            return null;
        }
        const w0 = TWO_PI * fc / fs;
        const cos_w0 = std.math.cos(w0);
        const sin_w0 = std.math.sin(w0);
        const alpha = sin_w0 / (2.0 * q);

        const a0 = 1.0 + alpha;
        const neg2cos = 0.0 - 2.0 * cos_w0;
        return Biquad {
            b0: 1.0 / a0,
            b1: neg2cos / a0,
            b2: 1.0 / a0,
            a1: neg2cos / a0,
            a2: (1.0 - alpha) / a0,
            z1: 0.0,
            z2: 0.0,
        };
    }

    /// Resets filter delay state `(z1, z2)` to zero.
    fn reset(*_) {
        _.z1 = 0.0;
        _.z2 = 0.0;
    }

    /// Processes a single sample using Direct Form II Transposed.
    fn process(*_, x: f64): f64 {
        const y = _.b0 * x + _.z1;
        _.z1 = _.b1 * x - _.a1 * y + _.z2;
        _.z2 = _.b2 * x - _.a2 * y;
        return y;
    }

    /// Processes a buffer of samples in-place.
    fn processInPlace(*_, buf: []f64) {
        let i: usize = 0;
        while (i < buf.len) {
            buf[i] = _.process(buf[i]);
            i += 1;
        }
    }

    /// Analytically evaluates the frequency response magnitude `|H(e^{j*2*pi*freq/fs})|`.
    fn magnitudeAt(fs: f64, freq: f64): f64 {
        if (!isFiniteF64(fs) || !isFiniteF64(freq) || fs <= 0.0) {
            return 0.0;
        }
        const w = TWO_PI * freq / fs;
        const c1 = std.math.cos(w);
        const s1 = std.math.sin(w);
        const c2 = std.math.cos(2.0 * w);
        const s2 = std.math.sin(2.0 * w);

        const num_re = _.b0 + _.b1 * c1 + _.b2 * c2;
        const num_im = 0.0 - (_.b1 * s1 + _.b2 * s2);
        const den_re = 1.0 + _.a1 * c1 + _.a2 * c2;
        const den_im = 0.0 - (_.a1 * s1 + _.a2 * s2);

        const num_sq = num_re * num_re + num_im * num_im;
        const den_sq = den_re * den_re + den_im * den_im;
        if (den_sq <= 0.0) {
            return 0.0;
        }
        return std.math.sqrt(num_sq / den_sq);
    }
}
