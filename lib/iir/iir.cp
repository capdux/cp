// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Classical IIR Filter Design: Butterworth, Chebyshev-I Bilinear SOS Cascade & Zero-Phase filtfilt)

const PI: f64 = 3.14159265358979323846;
const TWO_PI: f64 = 6.28318530717958647692;
const INV_LN10: f64 = 0.43429448190325182765;
const LN10: f64 = 2.30258509299404568402;

/// Normalized Second-Order Section (biquad) with `a0 = 1.0`:
/// `H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)`.
/// First-order sections set `b2 = 0.0` and `a2 = 0.0`.
pub struct SosSection {
    b0: f64,
    b1: f64,
    b2: f64,
    a1: f64,
    a2: f64,

    /// Identity pass-through section.
    static fn identity(): SosSection {
        return SosSection {
            b0: 1.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
        };
    }
}

/// Transposed Direct Form II delay-line state for a single SOS section.
pub struct SosState {
    z1: f64,
    z2: f64,

    /// Zero-initialized section state.
    static fn zero(): SosState {
        return SosState {
            z1: 0.0,
            z2: 0.0,
        };
    }
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn checkFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        if (!isFiniteF64(data[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn pow10(x: f64): f64 {
    return @exp(x * LN10);
}

fn validateCutoff(order: usize, cutoff_hz: f64, sample_rate: f64, out_len: usize): usize? {
    if (order == 0) {
        return null;
    }
    if (!isFiniteF64(cutoff_hz) || !isFiniteF64(sample_rate) || sample_rate <= 0.0) {
        return null;
    }
    if (cutoff_hz <= 0.0 || cutoff_hz >= (0.5 * sample_rate)) {
        return null;
    }
    const n_sections: usize = (order + 1) / 2;
    if (out_len < n_sections) {
        return null;
    }
    return n_sections;
}

fn bilinearLowpassFirstOrder(sigma: f64, k_warp: f64): SosSection {
    const ks: f64 = k_warp * sigma;
    const a0: f64 = 1.0 - ks;
    const b0: f64 = (0.0 - ks) / a0;
    const a1: f64 = (-1.0 - ks) / a0;
    return SosSection {
        b0: b0,
        b1: b0,
        b2: 0.0,
        a1: a1,
        a2: 0.0,
    };
}

fn bilinearLowpassSecondOrder(sigma: f64, omega: f64, k_warp: f64): SosSection {
    const r2: f64 = (sigma * sigma) + (omega * omega);
    const k2_r2: f64 = k_warp * k_warp * r2;
    const two_ks: f64 = 2.0 * k_warp * sigma;
    const a0: f64 = 1.0 - two_ks + k2_r2;
    const b0: f64 = k2_r2 / a0;
    return SosSection {
        b0: b0,
        b1: 2.0 * b0,
        b2: b0,
        a1: (2.0 * (k2_r2 - 1.0)) / a0,
        a2: (1.0 + two_ks + k2_r2) / a0,
    };
}

fn bilinearHighpassFirstOrder(k_warp: f64): SosSection {
    const a0: f64 = 1.0 + k_warp;
    const b0: f64 = 1.0 / a0;
    return SosSection {
        b0: b0,
        b1: 0.0 - b0,
        b2: 0.0,
        a1: (k_warp - 1.0) / a0,
        a2: 0.0,
    };
}

fn bilinearHighpassSecondOrder(sigma: f64, k_warp: f64): SosSection {
    const k2: f64 = k_warp * k_warp;
    const two_ks: f64 = 2.0 * k_warp * sigma;
    const a0: f64 = 1.0 - two_ks + k2;
    const b0: f64 = 1.0 / a0;
    return SosSection {
        b0: b0,
        b1: -2.0 * b0,
        b2: b0,
        a1: (2.0 * (k2 - 1.0)) / a0,
        a2: (1.0 + two_ks + k2) / a0,
    };
}

/// Designs an `order`-th order Butterworth lowpass IIR filter as a cascade of `ceil(order/2)` SOS sections
/// via pre-warped bilinear transform. DC gain is normalized to `1.0` (`0 dB`) and gain at `cutoff_hz` is `-3.0103 dB`.
pub fn designButterworthLowpass(
    order: usize,
    cutoff_hz: f64,
    sample_rate: f64,
    sections_out: *[]SosSection,
): usize? {
    const n_sections: usize = validateCutoff(order, cutoff_hz, sample_rate, sections_out.*.len) ?? return null;
    const k_warp: f64 = @tan(PI * cutoff_hz / sample_rate);
    let out: []SosSection = sections_out.*;

    const n_pairs: usize = order / 2;
    let sec_idx: usize = 0;
    let k: usize = 0;
    while (k < n_pairs) {
        const theta: f64 = (PI * ((2 * k + 1) as f64)) / ((2 * order) as f64);
        const sigma: f64 = 0.0 - @sin(theta);
        const omega: f64 = @cos(theta);
        out[sec_idx] = bilinearLowpassSecondOrder(sigma, omega, k_warp);
        sec_idx += 1;
        k += 1;
    }

    if ((order % 2) == 1) {
        out[sec_idx] = bilinearLowpassFirstOrder(-1.0, k_warp);
    }

    return n_sections;
}

/// Designs an `order`-th order Butterworth highpass IIR filter as a cascade of `ceil(order/2)` SOS sections
/// via pre-warped bilinear transform. Nyquist gain is normalized to `1.0` (`0 dB`) and gain at `cutoff_hz` is `-3.0103 dB`.
pub fn designButterworthHighpass(
    order: usize,
    cutoff_hz: f64,
    sample_rate: f64,
    sections_out: *[]SosSection,
): usize? {
    const n_sections: usize = validateCutoff(order, cutoff_hz, sample_rate, sections_out.*.len) ?? return null;
    const k_warp: f64 = @tan(PI * cutoff_hz / sample_rate);
    let out: []SosSection = sections_out.*;

    const n_pairs: usize = order / 2;
    let sec_idx: usize = 0;
    let k: usize = 0;
    while (k < n_pairs) {
        const theta: f64 = (PI * ((2 * k + 1) as f64)) / ((2 * order) as f64);
        const sigma: f64 = 0.0 - @sin(theta);
        out[sec_idx] = bilinearHighpassSecondOrder(sigma, k_warp);
        sec_idx += 1;
        k += 1;
    }

    if ((order % 2) == 1) {
        out[sec_idx] = bilinearHighpassFirstOrder(k_warp);
    }

    return n_sections;
}

/// Designs an `order`-th order Chebyshev Type-I equiripple lowpass IIR filter (`ripple_db > 0`)
/// as a cascade of `ceil(order/2)` SOS sections. Even-order DC gain is scaled by `10^(-ripple_db/20)`
/// so gain at `cutoff_hz` equals `-ripple_db` exactly.
pub fn designChebyshev1Lowpass(
    order: usize,
    ripple_db: f64,
    cutoff_hz: f64,
    sample_rate: f64,
    sections_out: *[]SosSection,
): usize? {
    if (!isFiniteF64(ripple_db) || ripple_db <= 0.0) {
        return null;
    }
    const n_sections: usize = validateCutoff(order, cutoff_hz, sample_rate, sections_out.*.len) ?? return null;
    const k_warp: f64 = @tan(PI * cutoff_hz / sample_rate);

    const eps_sq: f64 = pow10(ripple_db / 10.0) - 1.0;
    if (!isFiniteF64(eps_sq) || eps_sq <= 0.0) {
        return null;
    }
    const eps: f64 = @sqrt(eps_sq);
    const inv_eps: f64 = 1.0 / eps;
    const v0: f64 = @log(inv_eps + @sqrt((inv_eps * inv_eps) + 1.0)) / (order as f64);
    const ev: f64 = @exp(v0);
    const inv_ev: f64 = 1.0 / ev;
    const sinh_v0: f64 = 0.5 * (ev - inv_ev);
    const cosh_v0: f64 = 0.5 * (ev + inv_ev);

    let out: []SosSection = sections_out.*;
    const n_pairs: usize = order / 2;
    let sec_idx: usize = 0;
    let k: usize = 0;
    while (k < n_pairs) {
        const theta: f64 = (PI * ((2 * k + 1) as f64)) / ((2 * order) as f64);
        const sigma: f64 = (0.0 - sinh_v0) * @sin(theta);
        const omega: f64 = cosh_v0 * @cos(theta);
        out[sec_idx] = bilinearLowpassSecondOrder(sigma, omega, k_warp);
        sec_idx += 1;
        k += 1;
    }

    if ((order % 2) == 1) {
        out[sec_idx] = bilinearLowpassFirstOrder(0.0 - sinh_v0, k_warp);
    } else {
        // Even-order Chebyshev-I has DC gain 1 / sqrt(1 + eps^2) = 10^(-ripple_db / 20).
        const dc_scale: f64 = 1.0 / @sqrt(1.0 + eps_sq);
        out[0].b0 *= dc_scale;
        out[0].b1 *= dc_scale;
        out[0].b2 *= dc_scale;
    }

    return n_sections;
}

/// Applies a cascade of SOS sections in Transposed Direct Form II to `input`, writing to `output`.
pub fn sosFilter(
    sections: []const SosSection,
    states: *[]SosState,
    input: []const f64,
    output: *[]f64,
): bool {
    const n_sec: usize = sections.len;
    const n: usize = input.len;
    if (n_sec == 0 || states.*.len < n_sec || output.*.len < n) {
        return false;
    }
    if (!checkFinitePrefix(input, n)) {
        return false;
    }

    let st: []SosState = states.*;
    let out: []f64 = output.*;

    let i: usize = 0;
    while (i < n) {
        let x: f64 = input[i];
        let s: usize = 0;
        while (s < n_sec) {
            const sec: SosSection = sections[s];
            const y: f64 = (sec.b0 * x) + st[s].z1;
            st[s].z1 = (sec.b1 * x) - (sec.a1 * y) + st[s].z2;
            st[s].z2 = (sec.b2 * x) - (sec.a2 * y);
            x = y;
            s += 1;
        }
        out[i] = x;
        i += 1;
    }
    return true;
}

fn reverseF64InPlace(buf: []f64, n: usize) {
    if (n < 2) {
        return;
    }
    let lo: usize = 0;
    let hi: usize = n - 1;
    while (lo < hi) {
        const tmp: f64 = buf[lo];
        buf[lo] = buf[hi];
        buf[hi] = tmp;
        lo += 1;
        hi -= 1;
    }
}

/// Zero-phase forward-backward SOS filtering (`filtfilt`) eliminating phase distortion.
/// Caller provides `tmp_buf` (length >= `input.len`) and `states_scratch` (length >= `sections.len`).
pub fn sosFiltFilt(
    sections: []const SosSection,
    input: []const f64,
    output: *[]f64,
    tmp_buf: *[]f64,
    states_scratch: *[]SosState,
): bool {
    const n_sec: usize = sections.len;
    const n: usize = input.len;
    if (
        n_sec == 0 ||
        states_scratch.*.len < n_sec ||
        output.*.len < n ||
        tmp_buf.*.len < n
    ) {
        return false;
    }
    if (!checkFinitePrefix(input, n)) {
        return false;
    }
    if (n == 0) {
        return true;
    }

    // Forward pass: input -> tmp_buf
    let s0: usize = 0;
    while (s0 < n_sec) {
        states_scratch.*[s0] = SosState.zero();
        s0 += 1;
    }
    if (!sosFilter(sections, states_scratch, input, tmp_buf)) {
        return false;
    }

    // Reverse tmp_buf in-place, run backward pass: tmp_buf -> output, then reverse output.
    reverseF64InPlace(tmp_buf.*, n);
    let s1: usize = 0;
    while (s1 < n_sec) {
        states_scratch.*[s1] = SosState.zero();
        s1 += 1;
    }
    if (!sosFilter(sections, states_scratch, tmp_buf.*[0..n], output)) {
        return false;
    }
    reverseF64InPlace(output.*, n);
    return true;
}

/// Evaluates the exact magnitude frequency response of an SOS cascade at `freq_hz` in decibels (dB).
pub fn sosMagnitudeDb(sections: []const SosSection, sample_rate: f64, freq_hz: f64): f64? {
    if (sections.len == 0 || !isFiniteF64(sample_rate) || sample_rate <= 0.0) {
        return null;
    }
    if (!isFiniteF64(freq_hz) || freq_hz < 0.0 || freq_hz > (0.5 * sample_rate)) {
        return null;
    }

    const w: f64 = TWO_PI * freq_hz / sample_rate;
    const c1: f64 = @cos(w);
    const s1: f64 = 0.0 - @sin(w);
    const c2: f64 = @cos(2.0 * w);
    const s2: f64 = 0.0 - @sin(2.0 * w);

    let total_db: f64 = 0.0;
    let i: usize = 0;
    while (i < sections.len) {
        const sec: SosSection = sections[i];
        const num_re: f64 = sec.b0 + (sec.b1 * c1) + (sec.b2 * c2);
        const num_im: f64 = (sec.b1 * s1) + (sec.b2 * s2);
        const den_re: f64 = 1.0 + (sec.a1 * c1) + (sec.a2 * c2);
        const den_im: f64 = (sec.a1 * s1) + (sec.a2 * s2);

        const num_sq: f64 = (num_re * num_re) + (num_im * num_im);
        const den_sq: f64 = (den_re * den_re) + (den_im * den_im);
        if (den_sq <= 1.0e-300) {
            return null;
        }
        const ratio: f64 = num_sq / den_sq;
        if (ratio <= 1.0e-30) {
            return -300.0;
        }
        total_db += 10.0 * (@log(ratio) * INV_LN10);
        i += 1;
    }
    return total_db;
}
