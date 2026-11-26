// @cap/d3-array — Zero-allocation array statistics, quantiles, bisection & binning in pure CP
// Clean-room MIT implementation of npm `d3-array` (#345, 153M/wk).

pub struct ExtentF64 {
    pub valid: bool,
    pub min: f64,
    pub max: f64,
}

fn d3aIsNan(v: f64): bool {
    return v != v;
}

/// Returns the sum of all non-NaN values in `data`.
pub fn sumF64(data: []const f64): f64 {
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < data.len) {
        if (!d3aIsNan(data[i])) {
            acc += data[i];
        }
        i += 1;
    }
    return acc;
}

/// Returns the arithmetic mean of non-NaN values, or `null` if none.
pub fn meanF64(data: []const f64): f64? {
    let acc: f64 = 0.0;
    let n: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        if (!d3aIsNan(data[i])) {
            acc += data[i];
            n += 1;
        }
        i += 1;
    }
    if (n == 0) { return null; }
    return acc / (n as f64);
}

/// Returns `[min, max]` of non-NaN values in a single pass.
pub fn extentF64(data: []const f64): ExtentF64 {
    let found = false;
    let lo: f64 = 0.0;
    let hi: f64 = 0.0;
    let i: usize = 0;
    while (i < data.len) {
        const v = data[i];
        if (!d3aIsNan(v)) {
            if (!found) {
                lo = v;
                hi = v;
                found = true;
            } else {
                if (v < lo) { lo = v; }
                if (v > hi) { hi = v; }
            }
        }
        i += 1;
    }
    return ExtentF64 { valid: found, min: lo, max: hi };
}

/// Unbiased sample variance (Welford's numerically stable single-pass algorithm, Bessel's N-1).
pub fn varianceF64(data: []const f64): f64? {
    let n: usize = 0;
    let mean: f64 = 0.0;
    let m2: f64 = 0.0;
    let i: usize = 0;
    while (i < data.len) {
        const v = data[i];
        if (!d3aIsNan(v)) {
            n += 1;
            const delta = v - mean;
            mean += delta / (n as f64);
            m2 += delta * (v - mean);
        }
        i += 1;
    }
    if (n < 2) { return null; }
    return m2 / ((n - 1) as f64);
}

/// Unbiased sample standard deviation (`sqrt(variance)`).
pub fn deviationF64(data: []const f64): f64? {
    if (v := varianceF64(data)) {
        return @sqrt(v);
    }
    return null;
}

/// R-7 linear interpolation quantile on an already-sorted ascending slice `sorted` (`p` in `[0, 1]`).
pub fn quantileSorted(sorted: []const f64, p: f64): f64? {
    const n = sorted.len;
    if (n == 0 || d3aIsNan(p)) { return null; }
    if (p <= 0.0 || n == 1) { return sorted[0]; }
    if (p >= 1.0) { return sorted[n - 1]; }
    const idx = ((n - 1) as f64) * p;
    const lo: usize = idx as usize;
    const hi: usize = lo + 1;
    const frac = idx - (lo as f64);
    return sorted[lo] + (sorted[hi] - sorted[lo]) * frac;
}

/// Median on an already-sorted ascending slice (`quantileSorted(sorted, 0.5)`).
pub fn medianSorted(sorted: []const f64): f64? {
    return quantileSorted(sorted, 0.5);
}

/// Returns insertion index for `x` in ascending `sorted` to the LEFT of any existing equal entries.
pub fn bisectLeft(sorted: []const f64, x: f64): usize {
    let lo: usize = 0;
    let hi: usize = sorted.len;
    while (lo < hi) {
        const mid = lo + ((hi - lo) >> 1);
        if (sorted[mid] < x) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/// Returns insertion index for `x` in ascending `sorted` to the RIGHT of any existing equal entries.
pub fn bisectRight(sorted: []const f64, x: f64): usize {
    let lo: usize = 0;
    let hi: usize = sorted.len;
    while (lo < hi) {
        const mid = lo + ((hi - lo) >> 1);
        if (sorted[mid] <= x) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/// Writes cumulative sums of `data` into `out`, returning count written.
pub fn cumsumF64(data: []const f64, out: []f64): usize {
    let acc: f64 = 0.0;
    let n = data.len;
    if (out.len < n) { n = out.len; }
    let i: usize = 0;
    while (i < n) {
        if (!d3aIsNan(data[i])) {
            acc += data[i];
        }
        out[i] = acc;
        i += 1;
    }
    return n;
}

/// Counts how many values from `data` fall into each of `out_counts.len` equal-width bins over `[domain_min, domain_max]`.
pub fn binCounts(data: []const f64, domain_min: f64, domain_max: f64, out_counts: []usize): usize {
    const nb = out_counts.len;
    if (nb == 0 || domain_max <= domain_min) { return 0; }
    let k: usize = 0;
    while (k < nb) {
        out_counts[k] = 0;
        k += 1;
    }
    const span = domain_max - domain_min;
    let total: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        const v = data[i];
        if (!d3aIsNan(v) && v >= domain_min && v <= domain_max) {
            let b: usize = (((v - domain_min) / span) * (nb as f64)) as usize;
            if (b >= nb) { b = nb - 1; }
            out_counts[b] += 1;
            total += 1;
        }
        i += 1;
    }
    return total;
}

export {
    ExtentF64,
    sumF64,
    meanF64,
    extentF64,
    varianceF64,
    deviationF64,
    quantileSorted,
    medianSorted,
    bisectLeft,
    bisectRight,
    cumsumF64,
    binCounts,
};
