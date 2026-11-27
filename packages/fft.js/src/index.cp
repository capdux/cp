// @cap/fft-radix2 — Zero-allocation in-place iterative Cooley-Tukey Radix-2 FFT / IFFT
// MIT License

pub const PI: f64 = 3.14159265358979323846;
const TWO_PI: f64 = 6.28318530717958647692;

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return 0.0 - x;
    }
    return x;
}

pub fn sqrtF64(x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 64) {
        let next: f64 = 0.5 * (g + x / g);
        if (absF64(next - g) <= 0.000000000000001 * next) {
            return next;
        }
        g = next;
        i = i + 1;
    }
    return g;
}

fn reduceAngle(rad: f64): f64 {
    let x: f64 = rad;
    while (x > PI) {
        x = x - TWO_PI;
    }
    while (x < 0.0 - PI) {
        x = x + TWO_PI;
    }
    return x;
}

pub fn sinF64(rad: f64): f64 {
    let x: f64 = reduceAngle(rad);
    let xx: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let k: usize = 1;
    while (k <= 12) {
        let d1: f64 = (k * 2) as f64;
        let d2: f64 = (k * 2 + 1) as f64;
        term = (0.0 - term) * xx / (d1 * d2);
        sum = sum + term;
        k = k + 1;
    }
    return sum;
}

pub fn cosF64(rad: f64): f64 {
    let x: f64 = reduceAngle(rad);
    let xx: f64 = x * x;
    let term: f64 = 1.0;
    let sum: f64 = 1.0;
    let k: usize = 1;
    while (k <= 12) {
        let d1: f64 = (k * 2 - 1) as f64;
        let d2: f64 = (k * 2) as f64;
        term = (0.0 - term) * xx / (d1 * d2);
        sum = sum + term;
        k = k + 1;
    }
    return sum;
}

pub fn isPowerOfTwo(n: usize): bool {
    return n > 0 && (n & (n - 1)) == 0;
}

pub fn fftInPlace(re: []f64, im: []f64): bool {
    let n: usize = re.len;
    if (im.len != n || !isPowerOfTwo(n)) {
        return false;
    }
    if (n <= 1) {
        return true;
    }

    // Bit-reversal permutation
    let j: usize = 0;
    let i: usize = 1;
    while (i < n) {
        let bit: usize = n >> 1;
        while ((j & bit) != 0) {
            j = j ^ bit;
            bit = bit >> 1;
        }
        j = j ^ bit;
        if (i < j) {
            let tr: f64 = re[i];
            re[i] = re[j];
            re[j] = tr;
            let ti: f64 = im[i];
            im[i] = im[j];
            im[j] = ti;
        }
        i = i + 1;
    }

    // Iterative Cooley-Tukey butterfly stages
    let stage_len: usize = 2;
    while (stage_len <= n) {
        let half: usize = stage_len >> 1;
        let ang: f64 = (0.0 - TWO_PI) / (stage_len as f64);
        let wlen_re: f64 = cosF64(ang);
        let wlen_im: f64 = sinF64(ang);

        let start: usize = 0;
        while (start < n) {
            let w_re: f64 = 1.0;
            let w_im: f64 = 0.0;
            let k: usize = 0;
            while (k < half) {
                let idx_a: usize = start + k;
                let idx_b: usize = idx_a + half;
                let a_re: f64 = re[idx_a];
                let a_im: f64 = im[idx_a];
                let b_re: f64 = re[idx_b] * w_re - im[idx_b] * w_im;
                let b_im: f64 = re[idx_b] * w_im + im[idx_b] * w_re;
                re[idx_a] = a_re + b_re;
                im[idx_a] = a_im + b_im;
                re[idx_b] = a_re - b_re;
                im[idx_b] = a_im - b_im;

                let next_w_re: f64 = w_re * wlen_re - w_im * wlen_im;
                let next_w_im: f64 = w_re * wlen_im + w_im * wlen_re;
                w_re = next_w_re;
                w_im = next_w_im;
                k = k + 1;
            }
            start = start + stage_len;
        }
        stage_len = stage_len << 1;
    }
    return true;
}

pub fn ifftInPlace(re: []f64, im: []f64): bool {
    let n: usize = re.len;
    if (im.len != n || !isPowerOfTwo(n)) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        im[i] = 0.0 - im[i];
        i = i + 1;
    }
    let ok: bool = fftInPlace(re, im);
    if (!ok) {
        return false;
    }
    let inv_n: f64 = 1.0 / (n as f64);
    let k: usize = 0;
    while (k < n) {
        re[k] = re[k] * inv_n;
        im[k] = (0.0 - im[k]) * inv_n;
        k = k + 1;
    }
    return true;
}

pub fn magnitudeSpectrum(re: []const f64, im: []const f64, out_mag: []f64): usize {
    let count: usize = re.len;
    if (im.len < count) {
        count = im.len;
    }
    if (out_mag.len < count) {
        count = out_mag.len;
    }
    let i: usize = 0;
    while (i < count) {
        out_mag[i] = sqrtF64(re[i] * re[i] + im[i] * im[i]);
        i = i + 1;
    }
    return count;
}

pub fn dominantBin(re: []const f64, im: []const f64): usize {
    let count: usize = re.len;
    if (im.len < count) {
        count = im.len;
    }
    if (count == 0) {
        return 0;
    }
    let best_idx: usize = 0;
    let best_sq: f64 = re[0] * re[0] + im[0] * im[0];
    let i: usize = 1;
    while (i < count) {
        let sq: f64 = re[i] * re[i] + im[i] * im[i];
        if (sq > best_sq) {
            best_sq = sq;
            best_idx = i;
        }
        i = i + 1;
    }
    return best_idx;
}
