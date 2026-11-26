// @cap/fft — Zero-allocation in-place radix-2 Cooley-Tukey FFT / IFFT & windowing for CP
// Clean-room MIT implementation inspired by Rust `rustfft` and `microfft`.

const PI: f64 = 3.14159265358979323846;
const HALF_PI: f64 = 1.57079632679489661923;
const TWO_PI: f64 = 6.28318530717958647692;

pub struct Complex {
    pub re: f64,
    pub im: f64,
}

fn sqrtF64(x: f64) -> f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 64) {
        const next = 0.5 * (g + (x / g));
        let diff = next - g;
        if (diff < 0.0) {
            diff = -diff;
        }
        g = next;
        if (diff <= 0.000000000000001 * g) {
            break;
        }
        i += 1;
    }
    return g;
}

fn sinF64(x_in: f64) -> f64 {
    let x = x_in;
    const turns = ((x / TWO_PI) as i64) as f64;
    x -= turns * TWO_PI;
    while (x > PI) {
        x -= TWO_PI;
    }
    while (x < -PI) {
        x += TWO_PI;
    }
    if (x > HALF_PI) {
        x = PI - x;
    } else if (x < -HALF_PI) {
        x = -PI - x;
    }
    const x2 = x * x;
    let term = x;
    let sum = x;
    let k: usize = 1;
    while (k <= 10) {
        const d1 = (2 * k) as f64;
        const d2 = ((2 * k) + 1) as f64;
        term = (-term * x2) / (d1 * d2);
        sum += term;
        k += 1;
    }
    return sum;
}

fn cosF64(x: f64) -> f64 {
    return sinF64(x + HALF_PI);
}

pub fn isPowerOfTwo(n: usize) -> bool {
    if (n == (0 as usize)) {
        return false;
    }
    return (n & (n - (1 as usize))) == (0 as usize);
}

pub fn magnitude(c: Complex) -> f64 {
    return sqrtF64((c.re * c.re) + (c.im * c.im));
}

pub fn fftInPlace(buf: []Complex) -> bool {
    const n = buf.len;
    if (!isPowerOfTwo(n)) {
        return false;
    }
    if (n <= (1 as usize)) {
        return true;
    }

    // Bit-reversal permutation
    let j: usize = 0;
    let i: usize = 1;
    while (i < n) {
        let bit = n / (2 as usize);
        while ((j & bit) != (0 as usize)) {
            j = j ^ bit;
            bit = bit / (2 as usize);
        }
        j = j ^ bit;
        if (i < j) {
            const tmp = buf[i];
            buf[i] = buf[j];
            buf[j] = tmp;
        }
        i += 1;
    }

    // Iterative radix-2 Cooley-Tukey butterfly stages
    let stageLen: usize = 2;
    while (stageLen <= n) {
        const half = stageLen / (2 as usize);
        const angle = -TWO_PI / (stageLen as f64);
        const wStepRe = cosF64(angle);
        const wStepIm = sinF64(angle);

        let blockStart: usize = 0;
        while (blockStart < n) {
            let wRe: f64 = 1.0;
            let wIm: f64 = 0.0;
            let k: usize = 0;
            while (k < half) {
                const idxU = blockStart + k;
                const idxV = idxU + half;
                const u = buf[idxU];
                const v = buf[idxV];

                const tRe = (v.re * wRe) - (v.im * wIm);
                const tIm = (v.re * wIm) + (v.im * wRe);

                buf[idxU] = Complex{ re: u.re + tRe, im: u.im + tIm };
                buf[idxV] = Complex{ re: u.re - tRe, im: u.im - tIm };

                const nextWRe = (wRe * wStepRe) - (wIm * wStepIm);
                const nextWIm = (wRe * wStepIm) + (wIm * wStepRe);
                wRe = nextWRe;
                wIm = nextWIm;
                k += 1;
            }
            blockStart += stageLen;
        }
        stageLen = stageLen * (2 as usize);
    }

    return true;
}

pub fn ifftInPlace(buf: []Complex) -> bool {
    const n = buf.len;
    if (!isPowerOfTwo(n)) {
        return false;
    }
    if (n <= (1 as usize)) {
        return true;
    }
    let i: usize = 0;
    while (i < n) {
        buf[i] = Complex{ re: buf[i].re, im: -buf[i].im };
        i += 1;
    }
    const ok = fftInPlace(buf);
    if (!ok) {
        return false;
    }
    const invN = 1.0 / (n as f64);
    let j: usize = 0;
    while (j < n) {
        buf[j] = Complex{ re: buf[j].re * invN, im: -buf[j].im * invN };
        j += 1;
    }
    return true;
}

pub fn applyHannWindow(samples: []f64) {
    if (samples.len <= (1 as usize)) {
        return;
    }
    const denom = (samples.len - (1 as usize)) as f64;
    let i: usize = 0;
    while (i < samples.len) {
        const w = 0.5 * (1.0 - cosF64((TWO_PI * (i as f64)) / denom));
        samples[i] = samples[i] * w;
        i += 1;
    }
}

pub fn applyHammingWindow(samples: []f64) {
    if (samples.len <= (1 as usize)) {
        return;
    }
    const denom = (samples.len - (1 as usize)) as f64;
    let i: usize = 0;
    while (i < samples.len) {
        const w = 0.54 - (0.46 * cosF64((TWO_PI * (i as f64)) / denom));
        samples[i] = samples[i] * w;
        i += 1;
    }
}

export {
    Complex,
    isPowerOfTwo,
    fftInPlace,
    ifftInPlace,
    magnitude,
    applyHannWindow,
    applyHammingWindow,
};
