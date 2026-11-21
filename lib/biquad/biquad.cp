// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RBJ Audio EQ Cookbook Biquad IIR Filters & Goertzel Tone Detector)

const PI: f64 = 3.14159265358979323846;
const TWO_PI: f64 = 6.28318530717958647692;
const LN10: f64 = 2.30258509299404568402;
const MAG_FLOOR: f64 = 1.0e-15;

/// Standard RBJ Audio EQ Cookbook second-order IIR filter topologies.
pub enum FilterType {
    LowPass,
    HighPass,
    BandPass,
    Notch,
    AllPass,
    PeakingEq,
    LowShelf,
    HighShelf,
}

/// Normalized biquad transfer-function coefficients (`a0 = 1.0`):
/// `H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)`.
pub struct BiquadCoeffs {
    b0: f64,
    b1: f64,
    b2: f64,
    a1: f64,
    a2: f64,
}

/// Transposed Direct Form II biquad filter with value-type state (`z1`, `z2`).
pub struct BiquadFilter {
    coeffs: BiquadCoeffs,
    z1: f64,
    z2: f64,

    fn init(coeffs: BiquadCoeffs): BiquadFilter {
        return {
            coeffs: coeffs,
            z1: 0.0,
            z2: 0.0,
        };
    }

    fn reset(*_) {
        _.z1 = 0.0;
        _.z2 = 0.0;
    }

    fn processSample(*_, x: f64): f64 {
        const y = _.coeffs.b0 * x + _.z1;
        _.z1 = _.coeffs.b1 * x - _.coeffs.a1 * y + _.z2;
        _.z2 = _.coeffs.b2 * x - _.coeffs.a2 * y;
        return y;
    }

    /// Processes `input` into `output` sample-by-sample (safe for in-place aliasing).
    fn processBlock(*_, input: []const f64, output: *[]f64): bool {
        const n = input.len;
        if (output.*.len < n) {
            return false;
        }
        let i: usize = 0;
        while (i < n) {
            output.*[i] = _.processSample(input[i]);
            i += 1;
        }
        return true;
    }
}

/// Analytic frequency response evaluated at a specific frequency on the unit circle.
pub struct FreqResponse {
    magnitude: f64,
    magnitude_db: f64,
    phase_rad: f64,
}

/// Normalized single-frequency energy and sine amplitude estimate from Goertzel algorithm.
pub struct GoertzelResult {
    power: f64,
    magnitude: f64,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

/// Designs normalized RBJ Audio EQ Cookbook biquad coefficients (`a0 = 1.0`).
/// Validates finite inputs, `sample_rate > 0`, `0 < freq < sample_rate / 2`, and `q > 0`.
pub fn designBiquad(
    kind: FilterType,
    sample_rate: f64,
    freq: f64,
    q: f64,
    gain_db: f64,
): BiquadCoeffs? {
    if (!isFinite(sample_rate) || !isFinite(freq) || !isFinite(q) || !isFinite(gain_db)) {
        return null;
    }
    if (sample_rate <= 0.0 || freq <= 0.0 || freq >= sample_rate * 0.5 || q <= 0.0) {
        return null;
    }

    const w0 = TWO_PI * freq / sample_rate;
    const c = @cos(w0);
    const s = @sin(w0);
    const alpha = s / (2.0 * q);

    let b0: f64 = 0.0;
    let b1: f64 = 0.0;
    let b2: f64 = 0.0;
    let a0: f64 = 1.0;
    let a1: f64 = 0.0;
    let a2: f64 = 0.0;

    match (kind) {
        FilterType.LowPass => {
            b0 = (1.0 - c) * 0.5;
            b1 = 1.0 - c;
            b2 = (1.0 - c) * 0.5;
            a0 = 1.0 + alpha;
            a1 = -2.0 * c;
            a2 = 1.0 - alpha;
        },
        FilterType.HighPass => {
            b0 = (1.0 + c) * 0.5;
            b1 = -(1.0 + c);
            b2 = (1.0 + c) * 0.5;
            a0 = 1.0 + alpha;
            a1 = -2.0 * c;
            a2 = 1.0 - alpha;
        },
        FilterType.BandPass => {
            b0 = alpha;
            b1 = 0.0;
            b2 = 0.0 - alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * c;
            a2 = 1.0 - alpha;
        },
        FilterType.Notch => {
            b0 = 1.0;
            b1 = -2.0 * c;
            b2 = 1.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * c;
            a2 = 1.0 - alpha;
        },
        FilterType.AllPass => {
            b0 = 1.0 - alpha;
            b1 = -2.0 * c;
            b2 = 1.0 + alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * c;
            a2 = 1.0 - alpha;
        },
        FilterType.PeakingEq => {
            const amp = @exp(gain_db * (LN10 / 40.0));
            b0 = 1.0 + alpha * amp;
            b1 = -2.0 * c;
            b2 = 1.0 - alpha * amp;
            a0 = 1.0 + alpha / amp;
            a1 = -2.0 * c;
            a2 = 1.0 - alpha / amp;
        },
        FilterType.LowShelf => {
            const amp = @exp(gain_db * (LN10 / 40.0));
            const beta = 2.0 * @sqrt(amp) * alpha;
            const ap1 = amp + 1.0;
            const am1 = amp - 1.0;
            b0 = amp * (ap1 - am1 * c + beta);
            b1 = 2.0 * amp * (am1 - ap1 * c);
            b2 = amp * (ap1 - am1 * c - beta);
            a0 = ap1 + am1 * c + beta;
            a1 = -2.0 * (am1 + ap1 * c);
            a2 = ap1 + am1 * c - beta;
        },
        FilterType.HighShelf => {
            const amp = @exp(gain_db * (LN10 / 40.0));
            const beta = 2.0 * @sqrt(amp) * alpha;
            const ap1 = amp + 1.0;
            const am1 = amp - 1.0;
            b0 = amp * (ap1 + am1 * c + beta);
            b1 = -2.0 * amp * (am1 + ap1 * c);
            b2 = amp * (ap1 + am1 * c - beta);
            a0 = ap1 - am1 * c + beta;
            a1 = 2.0 * (am1 - ap1 * c);
            a2 = ap1 - am1 * c - beta;
        },
    }

    if (!isFinite(a0) || absF64(a0) < 1.0e-15) {
        return null;
    }
    const inv_a0 = 1.0 / a0;
    const coeffs: BiquadCoeffs = {
        b0: b0 * inv_a0,
        b1: b1 * inv_a0,
        b2: b2 * inv_a0,
        a1: a1 * inv_a0,
        a2: a2 * inv_a0,
    };
    if (!isFinite(coeffs.b0) || !isFinite(coeffs.b1) || !isFinite(coeffs.b2) || !isFinite(coeffs.a1) || !isFinite(coeffs.a2)) {
        return null;
    }
    return coeffs;
}

/// Processes a block through a cascade of second-order sections (SOS), supporting in-place aliasing.
pub fn processSosBlock(
    stages: *[]BiquadFilter,
    input: []const f64,
    output: *[]f64,
): bool {
    const n = input.len;
    if (output.*.len < n) {
        return false;
    }
    const num_stages = stages.*.len;
    let i: usize = 0;
    while (i < n) {
        let y = input[i];
        let s: usize = 0;
        while (s < num_stages) {
            y = stages.*[s].processSample(y);
            s += 1;
        }
        output.*[i] = y;
        i += 1;
    }
    return true;
}

/// Evaluates analytic frequency response `H(e^{j*w})` at `freq` in `[0, sample_rate / 2]`.
pub fn frequencyResponse(
    coeffs: BiquadCoeffs,
    sample_rate: f64,
    freq: f64,
): FreqResponse? {
    if (!isFinite(sample_rate) || !isFinite(freq) || sample_rate <= 0.0 || freq < 0.0 || freq > sample_rate * 0.5) {
        return null;
    }
    if (!isFinite(coeffs.b0) || !isFinite(coeffs.b1) || !isFinite(coeffs.b2) || !isFinite(coeffs.a1) || !isFinite(coeffs.a2)) {
        return null;
    }

    const w = TWO_PI * freq / sample_rate;
    const c1 = @cos(w);
    const s1 = 0.0 - @sin(w);
    const c2 = @cos(2.0 * w);
    const s2 = 0.0 - @sin(2.0 * w);

    const nr = coeffs.b0 + coeffs.b1 * c1 + coeffs.b2 * c2;
    const ni = coeffs.b1 * s1 + coeffs.b2 * s2;
    const dr = 1.0 + coeffs.a1 * c1 + coeffs.a2 * c2;
    const di = coeffs.a1 * s1 + coeffs.a2 * s2;

    const den2 = dr * dr + di * di;
    if (!isFinite(den2) || den2 < 1.0e-30) {
        return null;
    }

    const hr = (nr * dr + ni * di) / den2;
    const hi = (ni * dr - nr * di) / den2;
    const mag = @sqrt(hr * hr + hi * hi);
    const safe_mag = if (mag > MAG_FLOOR) mag else MAG_FLOOR;
    const mag_db = (20.0 / LN10) * @log(safe_mag);
    const phase = std.math.atan2(hi, hr);

    return {
        magnitude: mag,
        magnitude_db: mag_db,
        phase_rad: phase,
    };
}

/// O(N) zero-allocation Goertzel single-frequency tone detector.
/// Returns normalized power `|X(w)|^2 / N^2` and estimated single-sided sine amplitude
/// (`2 * |X(w)| / N`, or `|X(w)| / N` at DC and Nyquist).
pub fn goertzelDetect(
    samples: []const f64,
    sample_rate: f64,
    target_freq: f64,
): GoertzelResult? {
    const n = samples.len;
    if (n == 0 || !isFinite(sample_rate) || !isFinite(target_freq)) {
        return null;
    }
    if (sample_rate <= 0.0 || target_freq < 0.0 || target_freq > sample_rate * 0.5) {
        return null;
    }

    const w = TWO_PI * target_freq / sample_rate;
    const cos_w = @cos(w);
    const sin_w = @sin(w);
    const coeff = 2.0 * cos_w;

    let s1: f64 = 0.0;
    let s2: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const x = samples[i];
        if (!isFinite(x)) {
            return null;
        }
        const s0 = x + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
        i += 1;
    }

    const real = s1 - s2 * cos_w;
    const imag = s2 * sin_w;
    const raw_power = real * real + imag * imag;
    const nf = n as f64;
    const norm_power = raw_power / (nf * nf);
    const raw_mag = @sqrt(raw_power) / nf;
    const is_boundary_freq = (target_freq == 0.0) || (absF64(target_freq - sample_rate * 0.5) < 1.0e-12);
    const mag = if (is_boundary_freq) raw_mag else 2.0 * raw_mag;

    return {
        power: norm_power,
        magnitude: mag,
    };
}
